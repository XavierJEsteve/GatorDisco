libsynth: ../synth.c ../BiquadEQ.c ../effects.c ../Effects/BitCrush.c ../Effects/Echo.c ../Effects/Effect4.c ../Effects/Effect5.c ../Effects/PitchShift.c
	gcc -c ../synth.c
	gcc -c ../BiquadEQ.c
	gcc -c ../effects.c
	gcc -c ../Effects/BitCrush.c
	gcc -c ../Effects/Echo.c
	gcc -c ../Effects/Effect4.c
	gcc -c ../Effects/Effect5.c
	gcc -c ../Effects/PitchShift.c
	ar -crs libsynth.a synth.o BiquadEQ.o effects.o BitCrush.o Echo.o Effect4.o Effect5.o PitchShift.o
	mv libsynth.a ../../RPi/lib
	rm synth.o
	rm BiquadEQ.o
	rm effects.o
	rm BitCrush.o 
	rm Echo.o
	rm Effect4.o
	rm Effect5.o
	rm PitchShift.o
	