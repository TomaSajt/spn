import json
from os import path
from django.contrib.auth.decorators import login_required
from django.core.exceptions import PermissionDenied
from django.forms import ModelForm
from django.http import JsonResponse, HttpResponseBadRequest
from django.shortcuts import render, redirect, get_object_or_404
from django.template.loader import render_to_string
from django.views.decorators.http import require_POST
from core.models import SnakeVersion, ServerCommand, get_user_profile, ProgrammingLanguage


class CreateSnakeForm(ModelForm):
    class Meta:
        model = SnakeVersion
        fields = ['code', 'comment']


@login_required
def snake_list(request):
    snakes = SnakeVersion.objects.filter(user=request.user).order_by('-created')
    response = {'versions': [
        {
            'id': s.id,
            'version': s.version,
            'title': s.comment or '',
            'programming_language': s.programming_language.readable_name or '(unknown)',
            'date': s.created.strftime("%d.%m.%Y %H:%M:%S")
        } for s in snakes
    ]}
    return JsonResponse(response)


@login_required
def snake_create(request, programming_language=None):
    # if programming language is not given, use the first in the database
    if programming_language == None:
        programming_language = ProgrammingLanguage.objects.all()[0]

    ext = programming_language.file_extension
    code_filename = path.join("ide", "example_code", f"initial-bot.{ext}")

    snake = SnakeVersion(user=request.user)
    snake.code = open(code_filename, "r").read()
    snake.programming_language = programming_language
    return snake_edit(request, snake)


@login_required
def snake_edit_latest(request):
    try:
        return snake_edit(request, SnakeVersion.get_latest_for_user(request.user))
    except SnakeVersion.DoesNotExist:
        return snake_create(request)


@login_required
def snake_edit_version(request, snake_id):
    snake = get_object_or_404(SnakeVersion, pk=snake_id)
    if snake.user != request.user:
        raise PermissionDenied
    return snake_edit(request, snake)


@login_required
def snake_edit_by_language(request, lang_slug):
    try:
        proglang = ProgrammingLanguage.objects.get(slug=lang_slug)
    except ProgrammingLanguage.DoesNotExist:
        return HttpResponseBadRequest('Programming Language is unknown')

    print(proglang)

    try:
        return snake_edit(request, SnakeVersion.get_latest_for_user(request.user, programming_language=proglang))
    except SnakeVersion.DoesNotExist:
        return snake_create(request, proglang)


@login_required
@require_POST
def snake_save(request):
    json_req = json.loads(request.body.decode('utf-8'))

    action = json_req.get('action')
    if action not in ['run', 'save']:
        return HttpResponseBadRequest('unknown or undefined action')

    code = json_req.get('code')
    if code is None:
        return HttpResponseBadRequest('code not defined')

    comment = json_req.get('comment')

    programming_language = json_req.get('programming_language')
    if programming_language is None:
        return HttpResponseBadRequest('programming_language not defined')

    try:
        proglang = ProgrammingLanguage.objects.get(pk=programming_language)
    except ProgrammingLanguage.DoesNotExist:
        return HttpResponseBadRequest('programming_language is invalid')

    snake_args = {
            "code": code,
            "comment": comment,
            "programming_language": proglang
        }

    try:
        snake = SnakeVersion.objects.get(pk=json_req.get('parent'), user=request.user)
        snake = snake.create_new_if_changed(**snake_args)
    except SnakeVersion.DoesNotExist:
        snake = SnakeVersion(user=request.user, parent=None, **snake_args)
        snake.save()

    if action == "run":
        snake.activate()
        send_kill_command(snake.user)

    return JsonResponse({'success': True, 'snake_id': snake.id, 'version': snake.version, 'comment': snake.comment})


def send_kill_command(user):
    ServerCommand(user=user, command='kill').save()


def snake_edit(request, snake):
    form = CreateSnakeForm(request.POST or None)
    if form.is_valid():
        posted_code = form.cleaned_data.get('code')
        posted_comment = form.cleaned_data.get('comment', '')
        snake = snake.create_new_if_changed(code=posted_code, comment=posted_comment)
        snake.activate()
        send_kill_command(snake.user)
        return redirect('snake_edit', snake_id=snake.id)

    proglangs = ProgrammingLanguage.objects.all()

    print(proglangs)

    return render(request, 'ide/ide.html', {'form': form, 'snake': snake, 'profile': get_user_profile(request.user), 'programming_languages': proglangs})


@login_required
@require_POST
def snake_delete(request, snake_id=-1):
    snake = get_object_or_404(SnakeVersion, pk=snake_id)
    if snake.user != request.user:
        raise PermissionDenied
    snake.delete()
    return redirect('snake')


@login_required
@require_POST
def snake_activate(request, snake_id=-1):
    try:
        snake = SnakeVersion.objects.filter(user=request.user).get(pk=snake_id)
    except SnakeVersion.DoesNotExist:
        return JsonResponse({'message': 'Cannot activate snake: Not found.'}, status=404)

    if snake.user != request.user:
        return JsonResponse({'message': 'Cannot activate snake: forbidden'}, status=403)

    snake.activate()
    return JsonResponse({'message': 'Snake {} was activated'.format(snake.version)})


@login_required
@require_POST
def snake_disable(request):
    profile = get_user_profile(request.user)
    if profile.active_snake is not None:
        response = {'message': 'disabled snake {}'.format(profile.active_snake.version)}
        profile.active_snake = None
        profile.save()
        send_kill_command(request.user)
    else:
        response = {'message': 'no snake was and is enabled.'}

    if request.is_ajax():
        return JsonResponse(response)
    else:
        return redirect('snake')


@login_required
@require_POST
def snake_restart(request):
    profile = get_user_profile(request.user)
    if profile.active_snake is not None:
        send_kill_command(request.user)
        response = {'message': 'requesting restart of snake version {}'.format(profile.active_snake.version)}
    else:
        response = {'message': 'requesting kill of any running snake version (no activate snake version)'}

    return JsonResponse(response)


@login_required
def buildlogs(request):
    profile = get_user_profile(request.user)
    snake = profile.active_snake # SnakeVersion(user=request.user)
    return render(request, 'ide/buildlogs.html', {'snake': snake, 'profile': profile})
