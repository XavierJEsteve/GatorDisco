from django.contrib import admin

# Register your models here.
from .models import ConfigModel, AudioModel

# admin.site.register(SynthModel)
admin.site.register(AudioModel)
admin.site.register(ConfigModel)
