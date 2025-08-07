from django.shortcuts import render
from django.db.models import F, Max, ExpressionWrapper, FloatField, Count
from core.models import SnakeGame
from django.db import models
from django.conf import settings
from django.utils import timezone

import datetime


def get_relevant_games():
    if settings.HIGHSCORE_DT_FROM and settings.HIGHSCORE_DT_TILL:
        start_dt = settings.HIGHSCORE_DT_FROM
        end_dt = settings.HIGHSCORE_DT_TILL
    else:
        end_dt = timezone.now()
        start_dt = end_dt - settings.HIGHSCORE_DT_RANGE

    return SnakeGame.objects.filter(
        end_date__gte=start_dt,
        end_date__lte=end_dt
    ).exclude(
        user__username__in=settings.HIGHSCORE_BLACKLIST
    )


def sattr(obj, attr, val):
    if isinstance(obj, dict):
        obj[attr] = val
    else:
        setattr(obj, attr, val)


def gattr(obj, attr):
    if type(obj) is dict:
        return obj[attr]
    else:
        return getattr(obj, attr)


def table(request, data, usr, title, rotate):
    i = 0
    for d in data:
        sattr(d, 'position', '{}.'.format(i+1))
        if request.user.is_authenticated and gattr(d, 'user__username') == request.user.username:
            sattr(usr, 'position', gattr(d, 'position'))
        i = i + 1

    context={'highscores': data, 'title': title}

    if request.user.is_authenticated:
        context['usr'] = usr
    
    if request.GET.get('rotate'):
        context['rotate'] = rotate

    return render(request, 'highscore/table.html', context=context)


def score(request):
    data = get_relevant_games().values('user__username').annotate(
            score=Max('final_mass'),
            language=F('snake_version__programming_language__readable_name')).order_by('-score')
    if request.user.is_authenticated:
        usr = get_relevant_games().filter(user=request.user).aggregate(score=Max('final_mass'))
        if usr['score'] == None:
            usr = False
    else:
        usr = False
    return table(request, data, usr, 'Highscore', 'highscore_maxmass')


def maxmass(request):
    data = get_relevant_games().values('user__username').annotate(
            score=Max('maximum_mass'),
            language=F('snake_version__programming_language__readable_name')).order_by('-score')
    if request.user.is_authenticated:
        usr = get_relevant_games().filter(user=request.user).aggregate(score=Max('maximum_mass'))
        if usr['score'] == None:
            usr = False
    else:
        usr = False
    return table(request, data, usr, 'Anytime Highscore', 'highscore_maxage')


def maxage(request):
    data = get_relevant_games().\
        values('user__username').\
        annotate(score=Max(F('end_frame')-F('start_frame')),
                 language=F('snake_version__programming_language__readable_name')).\
        exclude(user__username__in=settings.HIGHSCORE_BLACKLIST).\
        order_by('-score')

    if request.user.is_authenticated:
        usr = get_relevant_games().filter(user=request.user).aggregate(score=Max(F('end_frame')-F('start_frame')))
        if usr['score'] == None:
            usr = False
    else:
        usr = False
    return table(request, data, usr, 'Max Age', 'highscore_consumerate')


def consumerate(request):
    data = get_relevant_games().values('user__username').annotate(
        score=Max(ExpressionWrapper(
            (F('natural_food_consumed') + F('carrison_food_consumed') + F('hunted_food_consumed'))
            / (F('end_frame') - F('start_frame')), output_field=models.FloatField())
        ), language=F('snake_version__programming_language__readable_name')
    ).order_by('-score')

    if request.user.is_authenticated:
        usr = data.filter(user__username=request.user.username)
        if len(usr) == 0:
            usr = False
        else:
            usr = usr[0]
    else:
        usr = False
    return table(request, data, usr, 'Consume Rate', 'highscore_kills')


def kills(request):
    data = get_relevant_games().\
        values('killer__username').\
        annotate(score=Count('killer_id'), language=F('snake_version__programming_language__readable_name')).\
        order_by('-score').\
        annotate(user__username=F('killer__username'))

    if request.user.is_authenticated:
        usr = get_relevant_games().filter(killer=request.user).aggregate(score=Count('killer_id'))
        if usr['score'] == None:
            usr = False
    else:
        usr = False
    return table(request, data, usr, 'Killcount', 'highscore_deaths')


def deaths(request):
    data = get_relevant_games().\
        values('user__username').\
        annotate(score=Count('user_id'), language=F('snake_version__programming_language__readable_name')).\
        order_by('score')

    if request.user.is_authenticated:
        usr = get_relevant_games().filter(user=request.user).aggregate(score=Count('user_id'))
        if usr['score'] == None:
            usr = False
    else:
        usr = False
    return table(request, data, usr, 'Deathcount', 'highscore')

