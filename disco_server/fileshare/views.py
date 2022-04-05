import os
from os import name
from django.shortcuts import redirect, render
from django.http import HttpResponse
from django.core.files.storage import FileSystemStorage
from django.utils.datastructures import MultiValueDictKeyError
from .forms import AudioForm, SynthForm
from .models import AudioModel, SynthModel

# Primarily for dbOperations
from django.conf import settings
from django.conf.urls.static import static
from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.urls import path


# Create your views here.
''' TODO  
        * Change 'Uploaded Files' text color to yellow
        * Change text color for each DB entry listed in the table
        * CRUD from the main screen
                '''
def index(request,action=-1,id=-1):
        audio_files = AudioModel.objects.all()
        synth_files = SynthModel.objects.all()
        
        configs, photos = config_list()

        context = {
                'audio_files'   : audio_files,
                'synth_files'   : synth_files,
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
                'audioform': audiofor0090m
        })

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

def delete_config(request, synth_id=None):
        config = SynthModel.objects.get(pk=synth_id)
        config.delete()
        return redirect('index')

def download_config(request, synth_id=None):
        return None
