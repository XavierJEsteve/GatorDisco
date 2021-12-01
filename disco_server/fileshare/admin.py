from django.contrib import admin

# Register your models here.
from .models import SynthModel, AudioModel

admin.site.register(SynthModel)
admin.site.register(AudioModel)
