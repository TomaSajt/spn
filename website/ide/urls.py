from django.urls import path
from . import views

urlpatterns = [
    path('list', views.snake_list, name='snake'),
    path('create', views.snake_create, name='snake_create'),
    path('edit/latest', views.snake_edit_latest, name='snake_edit_latest'),
    path('edit/<int:snake_id>', views.snake_edit_version, name='snake_edit_version'),
    path('edit/lang/<str:lang_slug>', views.snake_edit_by_language, name='snake_edit_by_language'),
    path('edit/save', views.snake_save, name='snake_save'),
    path('activate/<int:snake_id>', views.snake_activate, name='snake_activate'),
    path('delete/<int:snake_id>', views.snake_delete, name='snake_delete'),
    path('disable', views.snake_disable, name='snake_disable'),
    path('restart', views.snake_restart, name='snake_restart'),
    path('buildlogs', views.buildlogs, name='buildlogs'),
]
