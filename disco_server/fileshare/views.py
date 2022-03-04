from os import name
from django.shortcuts import redirect, render
from django.http import HttpResponse
from django.core.files.storage import FileSystemStorage
from django.utils.datastructures import MultiValueDictKeyError
from .forms import AudioForm, SynthForm
from .models import AudioModel, SynthModel


# Create your views here.
''' TODO  
        * Change 'Uploaded Files' text color to yellow
        * Change text color for each DB entry listed in the table
        * CRUD from the main screen
                '''
def index(request):
        audio_files = AudioModel.objects.all()
        synth_files = SynthModel.objects.all()

        if request.method == 'POST':
                synthform = SynthForm(request.POST)
                if synthform.is_valid():
                        print(synthform)
                        synthform.save()                        
                        # Need to save config settings in a place that the raylib application can load it
                        # config_bytes = [
                        #         synthform.cleaned_data.get("octave"),
                        #         synthform.cleaned_data.get("waveForm"),
                        #         synthform.cleaned_data.get("oscParam1"),
                        #         synthform.cleaned_data.get("oscParam2"),
                        #         synthform.cleaned_data.get("attack"),
                        #         synthform.cleaned_data.get("decay"),                        
                        #         synthform.cleaned_data.get("sustain"),
                        #         synthform.cleaned_data.get("release"),
                        #         synthform.cleaned_data.get("cutoff"),
                        #         synthform.cleaned_data.get("hp_lp")
                        # ]
                        # synthform = SynthForm() # Clear form after submission
                        # with open('synth_settings.bin', 'wb') as cfile:
                        #         cfile.write(bytearray(config_bytes))
                else:
                        print("Synthform is invalid")

        else:

                synthform = SynthForm()
                # print(synthform)
        context = {
                'audio_files'   : audio_files,
                'synth_files'   : synth_files,
                'synthform'     : synthform
        }
        return render(request, 'index.html', context)

###############################################
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
        