from django.db import models
from django.db.models import Model


class AudioModel(Model):
    name = models.CharField(null=True, max_length=16)
    file = models.FileField()

class SynthModel(Model):
    name        = models.CharField(null=True, max_length=16)
    octave      = models.DecimalField(max_digits=3, decimal_places=2)
    pulseWidth  = models.DecimalField(max_digits=3, decimal_places=2)
    pwmFreq     = models.DecimalField(max_digits=3, decimal_places=2)
    pwmValue    = models.DecimalField(max_digits=3, decimal_places=2)
    attack      = models.DecimalField(max_digits=3, decimal_places=2)
    decay       = models.DecimalField(max_digits=3, decimal_places=2)
    sustain     = models.DecimalField(max_digits=3, decimal_places=2)
    release     = models.DecimalField(max_digits=3, decimal_places=2)

    def __str__(self):
        return self.name