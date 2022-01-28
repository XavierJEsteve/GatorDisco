%System parameters
fs = 48000;
length_seconds = 3;
sample_duration = 1/fs;
%OSCILLATOR PARAMETERS
phase = 0;
phaseStride = 0;
PWM_phase = 0;
PWM_phaseStride = 0;
PWM_maxFreq = 20;
threshold = 0;
%USER PARAMETERS (0 to 1 for sliders, frequency determined by key press)
slider_PWM_freq = 0.2;
slider_PWM_val = 1;
slider_pulse_width = 0.5;
freq = 600;

%"phase stride is the cyclic frequency of the signal, PWM_phaseStride
%is the frequency of the pulse width modulator. phaseStride is added to a variable called phase
%to keep track of what the output of a sin wave oscillator would be at the current sample/iteration of
%the loop. This info is used to determine the output of the pulse wave, but it could also be used for any 
%oscillator who's output can be determined by a phase (sawtooth, triangle, etc.)
output = zeros(fs*length_seconds,1);
PWM_phaseStride = slider_PWM_freq*PWM_maxFreq * sample_duration;
phaseStride = freq * sample_duration;

%fill output buffer
for i = 1:fs*length_seconds
    
    phase = phase + phaseStride;
    if phase > 1
        phase = phase - 1;
    end
    %sin_value is equal to what the output would be if our oscillator was a
    %pure sin wave. This value is compared with a threshold to create a
    %pulse wave, which is low when sin_value is below the threshold and
    %high otherwise
    sin_value = sin(2 * pi * phase);
    %PWM phase
    PWM_phase = PWM_phase + PWM_phaseStride;
    if PWM_phase > 1
        PWM_phase = PWM_phase - 1;
    end
    PWM_sin_value = sin(2 * pi * PWM_phase)*slider_PWM_val;
    threshold = 0.5*(slider_pulse_width + PWM_sin_value);

    if sin_value > threshold
        output(i) = 0.5;
    else 
        output(i) = -0.5;
    end
end
soundsc(output,fs);
