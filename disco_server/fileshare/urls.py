from django.urls import path

from . import views

urlpatterns = [
        path('audio/', views.upload_audio, name='upload_audio'),
        path('', views.index, name='index')
]