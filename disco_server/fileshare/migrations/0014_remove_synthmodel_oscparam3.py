# Generated by Django 3.2.9 on 2022-03-04 17:54

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('fileshare', '0013_synthmodel_oscparam3'),
    ]

    operations = [
        migrations.RemoveField(
            model_name='synthmodel',
            name='oscParam3',
        ),
    ]