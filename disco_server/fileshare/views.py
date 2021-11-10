from django.shortcuts import render
from django.http import HttpResponse

# Forms
from .forms import upload

# Models

# Create your views here.
def upload(request):
        if request.method == 'POST':
                uploaded_file = request.FILES['audio_file'] # Dictionary key is based on HTML form <input name=*****> 
                print(uploaded_file.name)
                print(uploaded_file.size)
        return render(request, 'upload.html')

