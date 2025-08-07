import json
from django.conf import settings
from django.http import JsonResponse, HttpResponseBadRequest, HttpResponse
from django.forms import ModelForm
from django.core.exceptions import PermissionDenied, SuspiciousOperation
from django.shortcuts import render, redirect, get_object_or_404
from django.contrib.auth.decorators import login_required
from django.views.decorators.http import require_http_methods
from core.models import ApiKey, SnakeVersion, ServerCommand, get_user_profile, LiveStats, ProgrammingLanguage

class bot_api(object):
    def __init__(self, f):
        self.f = f

    def __call__(self, request, *args, **kwargs):
        try:
            key = request.META.get('HTTP_AUTHORIZATION', None) or request.GET.get('token', None) or None

            if key is not None:
                request.user = ApiKey.objects.get(key=key).user

            if request.user and not request.user.is_anonymous:
                return self.f(request, *args, **kwargs)
            else:
                raise ApiKey.DoesNotExist

        except ApiKey.DoesNotExist:
            raise PermissionDenied('Unauthorized: not logged in or no / invalid api key given')


def version_dict(v):
    return {
        'id': v.id,
        'parent': None if v.parent is None else v.parent.id,
        'version': v.version,
        'created': v.created,
        'comment': v.comment,
        'compile_state': v.compile_state,
        'error': v.server_error_message
    }


def decode_build_log(build_log):
    try:
        return json.loads(build_log)
    except json.JSONDecodeError:
        return  [{'o': build_log}]
    except TypeError:
        return []

def full_version_dict(v):
    d = version_dict(v)
    d['code'] = v.code
    d['build_log'] = decode_build_log(v.build_log)
    return d

@require_http_methods(['GET', 'POST', 'PUT'])
@bot_api
def version(request):
    if request.method in ['PUT', 'POST']:
        return put_version(request)
    else:
        return JsonResponse({'versions': [version_dict(v) for v in SnakeVersion.objects.filter(user=request.user)]})


@require_http_methods(['GET'])
@bot_api
def get_version(request, version_id):
    v = get_object_or_404(SnakeVersion, user=request.user, id=version_id)
    return JsonResponse(full_version_dict(v))


@require_http_methods(['GET'])
@bot_api
def get_compile_state(request):
    profile = get_user_profile(request.user)
    snake = profile.active_snake
    build_queue_size = len(SnakeVersion.objects.filter(compile_state='not_compiled'))

    data = {'build_queue_size': build_queue_size}
    if snake:
        data['compile_state'] = str(snake.compile_state)
        data['build_log'] = decode_build_log(snake.build_log)

    return JsonResponse(data)


@require_http_methods(['GET','POST','PUT','DELETE'])
@bot_api
def persistent_data(request):
    profile = get_user_profile(request.user)
    if request.method in ['POST', 'PUT']:
        if len(request.body) > settings.PERSISTENT_MEMORY_SIZE:
            return HttpResponseBadRequest('max size for persistent memory blob is {0} bytes.'.format(settings.PERSISTENT_MEMORY_SIZE))
        profile.persistent_data = request.body
        profile.save()
    elif request.method == 'DELETE':
        profile.persistent_data = bytearray([0]*settings.PERSISTENT_MEMORY_SIZE)
        profile.save()
    resp = HttpResponse(profile.persistent_data, content_type='application/octet-stream')
    resp['Content-Disposition'] = 'attachment; filename=persistent_data.dat'
    return resp


@require_http_methods(['POST', 'PUT'])
@bot_api
def put_version(request):
    data = json.loads(request.body)
    if not isinstance(data, dict):
        return HttpResponseBadRequest('need to send a json dict as request body')

    v = SnakeVersion()
    v.user = request.user
    v.parent = data.get('parent', None)
    v.comment = data.get('comment', None)
    v.code = data.get('code', None)
    if v.code is None:
        return HttpResponseBadRequest('need to provide lua script in code field');
    v.save()
    return get_version(request, version_id=v.id)


@require_http_methods(['GET'])
@bot_api
def get_active_version(request):
    up = get_user_profile(request.user)
    v = up.active_snake
    if v:
        return get_version(request, version_id=v.id)
    else:
        return JsonResponse({})


@require_http_methods(['POST'])
@bot_api
def activate_version(request, version_id):
    v = get_object_or_404(SnakeVersion, user=request.user, id=version_id)
    up = get_user_profile(request.user)
    up.active_snake = v
    up.save()
    return JsonResponse(version_dict(v))


@require_http_methods(['POST'])
@bot_api
def disable_active_version(request):
    up = get_user_profile(request.user)
    up.active_snake = None
    up.save()
    return JsonResponse({'result': 'ok'})


@require_http_methods(['POST'])
@bot_api
def disable_version(request, version_id):
    up = get_user_profile(request.user)
    if up.active_snake is not None and up.active_snake.id == version_id:
        up.active_snake = None
        up.save()
    return JsonResponse({'result': 'ok'})


@require_http_methods(['POST', 'DELETE'])
@bot_api
def kill_bot(request):
    ServerCommand(user=request.user, command='kill').save()
    return JsonResponse({'result': 'ok'})


@require_http_methods(['GET'])
@bot_api
def get_viewer_key(request):
    up = get_user_profile(request.user)
    return JsonResponse({'viewer_key': up.viewer_key})

######################
# API KEY MANAGEMENT #
######################
@require_http_methods(['GET'])
@login_required()
def list_api_keys(request):
    return render(request, 'api/list_api_keys.html', {
        'user': request.user,
        'max_keys': ApiKey.MAX_KEYS_PER_USER
    })


class CreateKeyForm(ModelForm):
    class Meta:
        model = ApiKey
        fields = ['comment']


@require_http_methods(['POST'])
@login_required()
def create_api_key(request):
    if request.user.apikey_set.count() >= ApiKey.MAX_KEYS_PER_USER:
        raise SuspiciousOperation

    form = CreateKeyForm(request.POST or None)
    if form.is_valid():
        key = ApiKey(user=request.user)
        key.comment = form.cleaned_data.get('comment', None)
        key.save()
    return redirect('api_keys_list')


@require_http_methods(['POST', 'DELETE'])
@login_required()
def delete_api_key(request, key_id):
    key = get_object_or_404(ApiKey, user=request.user, id=key_id)
    key.delete()
    return redirect('api_keys_list')

#########
# STATS #
#########

def stats_dict(livestats_val, buildstats_dict):
    if livestats_val:
        livestats = {
                "last_update": livestats_val.last_update,
                "fps": livestats_val.fps,
                "current_frame": livestats_val.current_frame,
                "running_bots": livestats_val.running_bots,
                "start_queue_len": livestats_val.start_queue_len,
                "stop_queue_len": livestats_val.stop_queue_len,
                "living_mass": livestats_val.living_mass,
                "dead_mass": livestats_val.dead_mass
            }
    else:
        livestats = {}

    if not buildstats_dict:
        buildstats_dict = {}

    return {
            "livestats": livestats,
            "buildstats": buildstats_dict
        }

@require_http_methods(['GET'])
#@login_required()
def stats(request):
    try:
        livestats = LiveStats.objects.get(id=1)
    except LiveStats.DoesNotExist:
        livestats = None

    buildstats = {}
    for tup in SnakeVersion.COMPILE_STATE_CHOICES:
        state = tup[0]
        buildstats[state] = SnakeVersion.objects.filter(compile_state=state).count()

    languages = ProgrammingLanguage.objects.all()
    for lang in languages:
        buildstats[lang.slug] = {}
        for tup in SnakeVersion.COMPILE_STATE_CHOICES:
            state = tup[0]
            buildstats[lang.slug][state] = SnakeVersion.objects.filter(compile_state=state, programming_language=lang).count()

    return JsonResponse(stats_dict(livestats, buildstats))
