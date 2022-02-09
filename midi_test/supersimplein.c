#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define  MIDI_DEVICE  "/dev/midi2"


int main(void) {
   unsigned char inpacket[4];
	unsigned char firstByte = 0;
	unsigned char secondByte = 0;
   // first open the sequencer device for reading.
   int seqfd = open(MIDI_DEVICE, O_RDONLY);
   if (seqfd == -1) {
      printf("Error: cannot open %s\n", MIDI_DEVICE);
      exit(1);
   } 

   // now just wait around for MIDI bytes to arrive and print them to screen.
   while (1) {
      read(seqfd, &inpacket, sizeof(inpacket));
 	if(firstByte != inpacket[1] || secondByte != inpacket[2]){
      // print the MIDI byte if this input packet contains one
	printf("Bytes:\n");
	printf("%d\n", inpacket[1]);
	printf("%d\n", inpacket[2]);
	firstByte = inpacket[1];
	secondByte = inpacket[2];
}
   }
      
   return 0;
}
