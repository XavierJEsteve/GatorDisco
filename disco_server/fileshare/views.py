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

from django.core import serializers
# from .serialize import ConfigSerializer


# Create your views here.
''' TODO  
        * Change 'Uploaded Files' text color to yellow
        * Change text color for each DB entry listed in the table
        * CRUD from the main screen
                '''
def index(request,action=-1,id=-1):
 
        config_rows = ConfigModel.objects.all()   
        audio_rows = AudioModel.objects.all()

        if request.method == 'POST':
                # AUDIO FILE UPLOAD
                audioform = AudioForm(request.POST, request.FILES)


                if audioform.is_valid():
                        audioform.save()
                try:
                        # Save audio file to media folder
                        uploaded_file = request.FILES['file'] # Dictionary key is based on HTML form <input name=*****> \
                        # fs = FileSystemStorage()
                        # fs.save(uploaded_file.name, uploaded_file)
                        print(uploaded_file.name, uploaded_file)
                        # Save audio file to db
                        A = AudioModel(name=uploaded_file.name,file=uploaded_file)
                        A.save()

                        return redirect('index')
                        
                except MultiValueDictKeyError:
                        print("Couldn't load an audio file\n")

                # elif: synthform.is_valid():
                #         pass
        else:
                audioform = AudioForm()
                configform = ConfigForm()

        context = {
                'audioform'     : audioform,
                'audio_rows'    : audio_rows,
                'config_rows'   : config_rows,
                'configform'    : configform
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
                        # Save audio file to db
                        return redirect('index')
                        
                except MultiValueDictKeyError:
                        print("Bad audio file")
        else:
                audioform = AudioForm()

        return render(request, 'upload_audio.html',{
                'audioform': audioform
        })

def upload_config(request):
        if request.method == 'POST':
                configform = ConfigForm(request.POST, request.FILES)
                if configform.is_valid():
                        configform.save()
                try:
                        # Save audio file to media folder
                        uploaded_file = request.FILES['file'] # Dictionary key is based on HTML form <input name=*****> \
                        fs = FileSystemStorage()
                        fs.save(uploaded_file.name, uploaded_file)
                        print(uploaded_file.name, uploaded_file)
                        # Save config file to db
                        return redirect('index')
                        
                except MultiValueDictKeyError:
                        print("Bad json file")
        else:
                audioform = AudioForm()

        return render(request, 'upload_audio.html',{
                'audioform': audioform
        })

def delete_audio(request, audio_id=None):
        audio = AudioModel.objects.get(pk=audio_id)
        audio.delete()
        return redirect('index')

def delete_config(request, config_id=None):
        config = SynthModel.objects.get(pk=synth_id)
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

def upload_config(request):
        ''' Area for uploading audio files'''

        if request.method == 'POST':
                synthform = SynthForm(request.POST, request.FILES)
                if synthform.is_valid():
                        synthform.save()
                try:
                        # Save audio file to media folder
                        uploaded_file = request.FILES['file'] # Dictionary key is based on HTML form <input name=*****> \
                        fs = FileSystemStorage()
                        fs.save(uploaded_file.name, uploaded_file)
                        print(uploaded_file.name, uploaded_file)
                        # Save config file to db
                        return redirect('index')
                        
                except MultiValueDictKeyError:
                        print("Bad config file")
        else:
                synthform = SynthForm()

        return render(request, 'upload_audio.html',{
                'audioform': audiofor0090m
        })