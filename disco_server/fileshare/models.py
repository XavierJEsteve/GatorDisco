from django.db import models
from django.core.validators import MaxValueValidator, MinValueValidator

class AudioModel(models.Model):
    name = models.CharField(null=True,max_length=16)
    file = models.FileField(upload_to='wavs')

    def __str__(self):
        return self.name

class SynthModel(models.Model):
    name = models.CharField(null=True,max_length=16)
    synFile = models.FileField(null=True,upload_to='configs')
    photo = models.ImageField(null=True,upload_to='images')

    def __str__(self):
        return self.name, self.synFile