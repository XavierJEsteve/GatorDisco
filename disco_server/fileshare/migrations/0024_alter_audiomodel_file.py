# Generated by Django 4.0.3 on 2022-04-06 22:19

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('fileshare', '0023_remove_configmodel_image_alter_configmodel_name'),
    ]

    operations = [
        migrations.AlterField(
            model_name='audiomodel',
            name='file',
            field=models.FileField(upload_to='wavs/'),
        ),
    ]
