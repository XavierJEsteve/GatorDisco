'''
https://developer.mozilla.org/en-US/docs/Learn/Server-side/Django/Forms
'''
from django.db import models
from django.db.models.base import Model
from django.forms import ModelForm, fields
from .models import SynthModel, AudioModel

class SynthForm(ModelForm):
    class Meta:
        model = SynthModel
        fields = '__all__'

class AudioForm(ModelForm):
    class Meta:
        model = AudioModel
        fields = '__all__'
