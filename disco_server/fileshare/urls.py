from django.urls import path

from . import views

urlpatterns = [
        path('index/', views.index, name='index'),
        path('audio/', views.upload_audio, name='upload_audio'),
        path('config/', views.upload_config, name='upload_config')
]