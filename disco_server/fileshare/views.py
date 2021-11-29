from django.shortcuts import render
from django.http import HttpResponse
from django.core.files.storage import FileSystemStorage
from django.utils.datastructures import MultiValueDictKeyError

# Forms
# from .forms import upload

# Models

# Create your views here.
def upload(request):
        if request.method == 'POST':
                try:
                        uploaded_file = request.FILES['audio_file'] # Dictionary key is based on HTML form <input name=*****> \
                        fs = FileSystemStorage()
                        fs.save(uploaded_file.name, uploaded_file)
                except MultiValueDictKeyError:
                        print("Bad file mate")
        return render(request, 'upload.html')

