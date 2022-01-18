from os import name
from django.shortcuts import redirect, render
from django.http import HttpResponse
from django.core.files.storage import FileSystemStorage
from django.utils.datastructures import MultiValueDictKeyError
from .forms import AudioForm, SynthForm
from .models import AudioModel, SynthModel

# Create your views here.

def index(request):
        audio_files = AudioModel.objects.all()
        synth_files = SynthModel.objects.all()
        context = {
                'audio_files'   : audio_files,
                'synth_files'  : synth_files
        }
        return render(request, 'index.html', context)

def upload_config(request):
        ''' Area for uploading config files'''

        if request.method == 'POST':
                synthform = SynthForm(request.POST)
                if synthform.is_valid():
                        synthform.save()
                        
                        # Need to save config settings in a place that the raylib application can load it
                        print(synthform)

        else:
                synthform = SynthForm()

        return render(request, 'upload_config.html', {
                'synthform': synthform 
        })

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
        