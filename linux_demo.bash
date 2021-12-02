#!/bin/bash

# Install python dependencies and start server
pip3 install -r ./disco_server/requirments.py
python3 ./disco_server/manage.py runserver 0.0.0.0:8000

# Start raylib application
./disco_render/main
