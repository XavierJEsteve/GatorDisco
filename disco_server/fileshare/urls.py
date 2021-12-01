from django.urls import path

from . import views

urlpatterns = [
        path('index/', views.index),
        path('audio/', views.upload_audio),
        path('config/', views.upload_config)
]