from django.db import models
from django.db.models import Model
from django.core.validators import MaxValueValidator, MinValueValidator

class AudioModel(Model):
    name = models.CharField(null=True, max_length=16)
    file = models.FileField()

    def __str__(self):
        return self.name

class SynthModel(Model):
    name        = models.CharField(null=True, max_length=16)
    waveForm    = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(2),
                    MinValueValidator(0)
                    ])
    octave      = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    oscParam1   = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    oscParam2   = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    oscParam3   = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    attack      = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    decay       = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    sustain     = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    release     = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    cutoff      = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    hp_lp       = models.IntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])

    def __str__(self):
        return self.name