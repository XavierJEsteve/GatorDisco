import os
from os import name
from django.shortcuts import redirect, render
from django.http import HttpResponse
from django.core.files.storage import FileSystemStorage
from django.utils.datastructures import MultiValueDictKeyError
from .forms import AudioForm, ConfigForm
from .models import AudioModel, ConfigModel

# Primarily for dbOperations
from django.conf import settings
from django.conf.urls.static import static
from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.urls import path
from django.db.models import Max

from django.core import serializers
from rest_framework import serializers as sz

# Create your views here.
''' TODO  
        * Change 'Uploaded Files' text color to yellow
        * Change text color for each DB entry listed in the table
        * CRUD from the main screen
                '''
# class ConfigSerializer(sz.Serializer):
#         name        = models.CharField(max_length=16)
#         octave      = models.IntegerField(default=0)
#         oscParam1   = models.IntegerField(default=0)
#         oscParam2   = models.IntegerField(default=0)
#         lfoSpeed    = models.IntegerField(default=0)
#         lfoval      = models.IntegerField(default=0)
#         Attack      = models.IntegerField(default=0)
#         Decay       = models.IntegerField(default=0)
#         Sustain     = models.IntegerField(default=0)
#         Release     = models.IntegerField(default=0)
#         Effect1     = models.IntegerField(default=0)
#         Effect2     = models.IntegerField(default=0)
#         oscType     = models.IntegerField(default=0)
#         effectType  = models.IntegerField(default=0)
#         lfoTarget   = models.IntegerField(default=0)

#         def create(self, validated_data):
#                 return config

#         def update(self,instance,validated_data)
#                 return instance

def index(request,action=-1,id=-1):
 
        config_rows = ConfigModel.objects.all()   
        audio_rows = AudioModel.objects.all()
        audioform = AudioForm()
        configform = ConfigForm()
        
        context = {
                'audioform'     : audioform,
                'audio_rows'    : audio_rows,
                'config_rows'   : config_rows,
                # 'configform'    : configform
        }
        return render(request, 'index.html', context)

def upload_audio(request):
        ''' Area for uploading audio files'''

        if request.method == 'POST':
                audioform = AudioForm(request.POST, request.FILES)
                if audioform.is_valid():
                        audioform.save()
                try:
                        # Save audio file to media folder
                        uploaded_file = request.FILES['file'] # Dictionary key is based on HTML form <input name=*****> \
                        fs = FileSystemStorage()
                        fs.save(uploaded_file.name, uploaded_file)
                        print(uploaded_file.name, uploaded_file)
                        A = AudioModel(name=uploaded_file.name,file=uploaded_file)
                        A.save()                       
                        return redirect('index')
                        
                except MultiValueDictKeyError:
                        print("Bad audio file")
        else:
                audioform = AudioForm()

        return redirect('index')

def upload_config(request):

        if request.method == 'POST':
               
                try:
                        print(request.FILES.keys())
                        uploaded_file = request.FILES['myfile']
                        data = uploaded_file.read()
                        # fs = FileSystemStorage()
                        # fs.save(uploaded_file.name, uploaded_file)
                        print(uploaded_file.name, uploaded_file)
                        # print(data)
                        for obj in serializers.deserialize("json",data):
                                configs = ConfigModel.objects.all()
                                max = configs.aggregate(Max('id'))

                                obj.object.pk = max['id__max'] + 1
                                obj.object.save()

                        return redirect('index')
                        
                except MultiValueDictKeyError:
                        print("Bad json file")
        else:
                configform = ConfigForm()

        return redirect('index')

def delete_audio(request, audio_id=None):
        audio = AudioModel.objects.get(pk=audio_id)
        audio.file.delete(save=False)
        return redirect('index')

def delete_config(request, config_id=None):
        config = ConfigModel.objects.get(pk=config_id)
        config.delete()
        return redirect('index')

def download_config(request, config_id=None):
        config = ConfigModel.objects.filter(pk=config_id)
        JSONSerializer = serializers.get_serializer("json")
        json_serializer = JSONSerializer()
        json_serializer.serialize(config)
        data = json_serializer.getvalue()

        response = HttpResponse(data,content_type='application/json')
        response['Content-Disposition'] = ('attatchment; filename=gatorSynth.json')
        return response

def download_audio(request, audio_id=None):
        audio = AudioModel.objects.get(pk=audio_id)
        audio_file = audio.file
        file_path = os.path.join(settings.MEDIA_ROOT, audio.file.path)
        print(file_path)
        if os.path.exists(file_path):
                with open(file_path, 'rb') as fh:
                        response = HttpResponse(fh.read(), content_type="audio/x-wav")
                        response['Content-Disposition'] = 'inline; filename=' + os.path.basename(file_path)
                        return response