from rest_framework import serializers 
from .models import ConfigModel

class ConfigSerializer(serializers.ModelSerializer):
    class Meta:
        model = ConfigModel
        fields = [ 'name', 
                    'octave','oscParam1', 'oscParam2',
                     'lfoSpeed','lfoval', 
                    'Attack', 'Decay',' Sustain', 'Release',
                    'Effect1', 'Effect2', 'oscType', 'effectType', 'lfoTarget'
                ]