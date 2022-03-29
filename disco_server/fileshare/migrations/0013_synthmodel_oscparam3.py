# Generated by Django 3.2.9 on 2022-03-04 17:50

import django.core.validators
from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('fileshare', '0012_auto_20220224_1857'),
    ]

    operations = [
        migrations.AddField(
            model_name='synthmodel',
            name='oscParam3',
            field=models.PositiveSmallIntegerField(default=0, validators=[django.core.validators.MaxValueValidator(127), django.core.validators.MinValueValidator(0)]),
        ),
    ]