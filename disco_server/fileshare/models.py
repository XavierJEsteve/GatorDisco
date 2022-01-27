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
    oscParam1   = models.IntegerField(default=0)
    oscParam2   = models.IntegerField(default=0)
    oscParam3   = models.IntegerField(default=0)
    attack      = models.IntegerField(default=0)
    decay       = models.IntegerField(default=0)
    sustain     = models.IntegerField(default=0)
    release     = models.IntegerField(default=0)
    cutoff      = models.IntegerField(default=0)
    hp_lp       = models.IntegerField(default=0)

    def __str__(self):
        return self.name