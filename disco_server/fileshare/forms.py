'''
https://developer.mozilla.org/en-US/docs/Learn/Server-side/Django/Forms
'''
from django.db import models
from django.db.models.base import Model
from django import forms
from django.forms import ModelForm, ChoiceField
from .models import SynthModel, AudioModel

class SynthForm(ModelForm):
    
    
    class Meta:
        
        model = SynthModel
        fields = ('__all__')

        widgets = {
            # 'waveForm' : forms.Select(choices=WAVE_CHOICES, attrs={'class': 'form-group','style':'width:190px'}),
        }

        def __init__(self, *args, **kwargs):
            super(SynthForm,self).__init__(*args, **kwargs)
            # for field in iter(self.fields):
            #     self.fields[field].widget.attrs.update({
            #         'class': 'form-group',
            #     })
            #     self.fields[field].widget.attrs['style'] = 'width:100px;'


class AudioForm(ModelForm):
    class Meta:
        model = AudioModel
        fields = '__all__'

    def __init__(self, *args, **kwargs):
            super(AudioForm,self).__init__(*args, **kwargs)
            for field in iter(self.fields):
                self.fields[field].widget.attrs.update({
                    'class': 'form-group',
                })
                # self.fields[field].widget.attrs['style'] = 'width:100px;'