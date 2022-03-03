from django.db import models
from django.core.validators import MaxValueValidator, MinValueValidator

class AudioModel(models.Model):
    name = models.CharField(null=True, max_length=16)
    file = models.FileField()

    def __str__(self):
        return self.name

class SynthModel(models.Model):
    name        = models.CharField(null=True, max_length=16)
    waveForm    = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(3),
                    MinValueValidator(0)
                    ])
    octave      = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    oscParam1   = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    oscParam2   = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    lfo_speed   = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    lfo_val     = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    lfo_target  = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(2),
                    MinValueValidator(0)
                    ])
    attack      = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    decay       = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    sustain     = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    release     = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    fx_sel      = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(2),
                    MinValueValidator(0)
                    ])
    fx_param1   = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])
    fx_param2   = models.PositiveSmallIntegerField(default=0, 
                    validators=[
                    MaxValueValidator(127),
                    MinValueValidator(0)
                    ])

    def __str__(self):
        return self.name