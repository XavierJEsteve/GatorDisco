from django.db import models
from django.db.models import Model


class AudioModel(Model):
    name = models.CharField(null=True, max_length=16)
    file = models.FileField()

    def __str__(self):
        return self.name

class SynthModel(Model):
    name        = models.CharField(null=True, max_length=16)
    octave      = models.IntegerField(default=0)
    pulseWidth  = models.IntegerField(default=0)
    pwmFreq     = models.IntegerField(default=0)
    pwmValue    = models.IntegerField(default=0)
    attack      = models.IntegerField(default=0)
    decay       = models.IntegerField(default=0)
    sustain     = models.IntegerField(default=0)
    release     = models.IntegerField(default=0)

    def __str__(self):
        return self.name