from django.db import models
from django.db.models.base import Model
from django.forms import ModelForm, fields
from .models import SynthModel

class SynthForm(ModelForm):
    class Meta:
        model = SynthModel
        fields = '__all__'