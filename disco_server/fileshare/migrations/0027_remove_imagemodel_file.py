# Generated by Django 4.0.3 on 2022-04-06 23:04

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('fileshare', '0026_imagemodel'),
    ]

    operations = [
        migrations.RemoveField(
            model_name='imagemodel',
            name='file',
        ),
    ]
