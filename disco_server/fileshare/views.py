from django.shortcuts import render
from django.http import HttpResponse
from django.core.files.storage import FileSystemStorage
from django.utils.datastructures import MultiValueDictKeyError
from .forms import SynthForm
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


        else:
                synthform = SynthForm()

        return render(request, 'upload_config.html', {
                'synthform': synthform 
        })

def upload_audio(request):

        if request.method == 'POST':

                try:
                        uploaded_file = request.FILES['file'] # Dictionary key is based on HTML form <input name=*****> \
                        fs = FileSystemStorage()
                        fs.save(uploaded_file.name, uploaded_file)

                except MultiValueDictKeyError:
                        print("Bad audio file")

        return render(request, 'upload_audio.html',)
        