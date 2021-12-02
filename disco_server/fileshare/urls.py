from django.urls import path

from . import views

urlpatterns = [
        path('audio/', views.upload_audio, name='upload_audio'),
        path('config/', views.upload_config, name='upload_config'),
        path('', views.index, name='index')
]