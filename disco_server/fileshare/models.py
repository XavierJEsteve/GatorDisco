from django.db import models
from django.db.models import Model


class AudioModel(Model):

    file_field = models.FileField()