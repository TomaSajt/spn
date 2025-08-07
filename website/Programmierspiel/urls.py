from django.contrib import admin
from django.urls import include, path
from django.contrib.auth import views as auth_views
from django.views.generic import TemplateView
from django.views.generic.base import RedirectView
from core import views as core_views

urlpatterns = [
    path('', TemplateView.as_view(template_name='core/index.html'), name='home'),
    path('privacy/', TemplateView.as_view(template_name='core/privacy.html'), name='privacy'),
    path('admin/', admin.site.urls),
    path('signup/', core_views.signup, name='signup'),
    path('password/', core_views.change_password, name='change_password'),
    path('profile/', core_views.profile, name='profile'),
    path('login/', auth_views.LoginView.as_view(template_name='login.html'), name='login'),
    path('logout/', auth_views.LogoutView.as_view(next_page='login'), name='logout'),
    path('watch/', TemplateView.as_view(template_name='visualization/watch.html'), name='watch'),
    path('tv/', TemplateView.as_view(template_name='visualization/tv.html'), name='tv'),
    path('accounts/profile/', RedirectView.as_view(url='/', permanent=False)),
    path('snake/', include('ide.urls')),
    path('highscore/', include('highscore.urls')),
    path('docs/', include('docs.urls')),
    path('api/v1/', include('api.urls')),
]
