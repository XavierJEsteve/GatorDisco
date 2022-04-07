from django.conf import settings
from django.conf.urls.static import static
from django.urls import path
from django.contrib.staticfiles.urls import staticfiles_urlpatterns

from . import views

urlpatterns = [
        path('audio/', views.upload_audio, name='upload_audio'),
        path('delete_config/<config_id>', views.delete_config, name='delete-config'),
        path('delete_audio/<audio_id>', views.delete_audio, name='delete-audio'),
        path('download_config/<config_id>', views.download_config, name='download-config'),

        path('', views.index, name='index')
]

# Serving the media files in development mode
if settings.DEBUG:
    urlpatterns += static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
else:
    urlpatterns += staticfiles_urlpatterns()