from django.urls import path
from . import views as highscore_views

urlpatterns = [
    path('anytime_score', highscore_views.maxmass, name='highscore_maxmass'),
    path('maxage', highscore_views.maxage, name='highscore_maxage'),
    path('consumerate', highscore_views.consumerate, name='highscore_consumerate'),
    path('kills', highscore_views.kills, name='highscore_kills'),
    path('deaths', highscore_views.deaths, name='highscore_deaths'),
    path('', highscore_views.score, name='highscore'),
]
