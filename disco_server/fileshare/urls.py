from django.urls import path

from . import views

urlpatterns = [
        path('audio/', views.upload_audio, name='upload_audio'),
        path('delete_config/<synth_id>', views.delete_config, name='delete-config'),
        path('', views.index, name='index')
]