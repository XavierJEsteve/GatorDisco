from django.db import models
from django.core.validators import MaxValueValidator, MinValueValidator

class AudioModel(models.Model):
    name = models.CharField(max_length=16)
    file = models.FileField(upload_to='wavs')

    def __str__(self):
        return self.name

class SynthModel(models.Model):
    name = models.CharField(unique=True,max_length=16)
    synFile = models.FileField()
    photo = models.ImageField()

    def __str__(self):
        return self.name

class ConfigModel(models.Model):
    name        = models.CharField(unique=True, max_length=16)
    octave      = models.IntegerField(default=0)
    oscParam1   = models.IntegerField(default=0)
    oscParam2   = models.IntegerField(default=0)
    lfoSpeed    = models.IntegerField(default=0)
    lfoval      = models.IntegerField(default=0)
    Attack      = models.IntegerField(default=0)
    Decay       = models.IntegerField(default=0)
    Sustain     = models.IntegerField(default=0)
    Release     = models.IntegerField(default=0)
    Effect1     = models.IntegerField(default=0)
    Effect2     = models.IntegerField(default=0)
    oscType     = models.IntegerField(default=0)
    effectType  = models.IntegerField(default=0)
    lfoTarget   = models.IntegerField(default=0)
    image       = models.BinaryField(blank=True,editable=True)

    def __str__(self):
        return self.name