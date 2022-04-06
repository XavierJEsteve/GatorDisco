    1 
    8 /*
    9  * $Id: patest_read_record.c 757 2004-02-13 07:48:10Z rossbencina $
   10  *
   11  * This program uses the PortAudio Portable Audio Library.
   12  * For more information see: http://www.portaudio.com
   13  * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
   14  *
   15  * Permission is hereby granted, free of charge, to any person obtaining
   16  * a copy of this software and associated documentation files
   17  * (the "Software"), to deal in the Software without restriction,
   18  * including without limitation the rights to use, copy, modify, merge,
   19  * publish, distribute, sublicense, and/or sell copies of the Software,
   20  * and to permit persons to whom the Software is furnished to do so,
   21  * subject to the following conditions:
   22  *
   23  * The above copyright notice and this permission notice shall be
   24  * included in all copies or substantial portions of the Software.
   25  *
   26  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   27  * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   28  * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   29  * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
   30  * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
   31  * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   32  * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   33  */
   34 
   35 /*
   36  * The text above constitutes the entire PortAudio license; however,
   37  * the PortAudio community also makes the following non-binding requests:
   38  *
   39  * Any person wishing to distribute modifications to the Software is
   40  * requested to send the modifications to the original developer so that
   41  * they can be incorporated into the canonical version. It is also
   42  * requested that these non-binding requests be included along with the
   43  * license above.
   44  */
   45 
   46 #include <stdio.h>
   47 #include <stdlib.h>
   48 #include <string.h>
   49 #include "portaudio.h"
   50 
   51 /* #define SAMPLE_RATE  (17932) // Test failure to open with this value. */
   52 #define SAMPLE_RATE       (44100)
   53 #define FRAMES_PER_BUFFER   (512)
   54 #define NUM_SECONDS          (10)
   55 /* #define DITHER_FLAG     (paDitherOff)  */
   56 #define DITHER_FLAG           (0)
   57 
   58 /* Select sample format. */
   59 #if 1
   60 #define PA_SAMPLE_TYPE  paFloat32
   61 #define SAMPLE_SIZE (4)
   62 #define SAMPLE_SILENCE  (0.0f)
   63 #define PRINTF_S_FORMAT "%.8f"
   64 #elif 0
   65 #define PA_SAMPLE_TYPE  paInt16
   66 #define SAMPLE_SIZE (2)
   67 #define SAMPLE_SILENCE  (0)
   68 #define PRINTF_S_FORMAT "%d"
   69 #elif 0
   70 #define PA_SAMPLE_TYPE  paInt24
   71 #define SAMPLE_SIZE (3)
   72 #define SAMPLE_SILENCE  (0)
   73 #define PRINTF_S_FORMAT "%d"
   74 #elif 0
   75 #define PA_SAMPLE_TYPE  paInt8
   76 #define SAMPLE_SIZE (1)
   77 #define SAMPLE_SILENCE  (0)
   78 #define PRINTF_S_FORMAT "%d"
   79 #else
   80 #define PA_SAMPLE_TYPE  paUInt8
   81 #define SAMPLE_SIZE (1)
   82 #define SAMPLE_SILENCE  (128)
   83 #define PRINTF_S_FORMAT "%d"
   84 #endif
   85 
   86 /*******************************************************************/
   87 int main(void);
   88 int main(void)
   89 {
   90     PaStreamParameters inputParameters, outputParameters;
   91     PaStream *stream = NULL;
   92     PaError err;
   93     const PaDeviceInfo* inputInfo;
   94     const PaDeviceInfo* outputInfo;
   95     char *sampleBlock = NULL;
   96     int i;
   97     int numBytes;
   98     int numChannels;
   99 
  100     printf("patest_read_write_wire.c\n"); fflush(stdout);
  101     printf("sizeof(int) = %lu\n", sizeof(int)); fflush(stdout);
  102     printf("sizeof(long) = %lu\n", sizeof(long)); fflush(stdout);
  103 
  104     err = Pa_Initialize();
  105     if( err != paNoError ) goto error2;
  106 
  107     inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
  108     printf( "Input device # %d.\n", inputParameters.device );
  109     inputInfo = Pa_GetDeviceInfo( inputParameters.device );
  110     printf( "    Name: %s\n", inputInfo->name );
  111     printf( "      LL: %g s\n", inputInfo->defaultLowInputLatency );
  112     printf( "      HL: %g s\n", inputInfo->defaultHighInputLatency );
  113 
  114     outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
  115     printf( "Output device # %d.\n", outputParameters.device );
  116     outputInfo = Pa_GetDeviceInfo( outputParameters.device );
  117     printf( "   Name: %s\n", outputInfo->name );
  118     printf( "     LL: %g s\n", outputInfo->defaultLowOutputLatency );
  119     printf( "     HL: %g s\n", outputInfo->defaultHighOutputLatency );
  120 
  121     numChannels = inputInfo->maxInputChannels < outputInfo->maxOutputChannels
  122             ? inputInfo->maxInputChannels : outputInfo->maxOutputChannels;
  123     printf( "Num channels = %d.\n", numChannels );
  124 
  125     inputParameters.channelCount = numChannels;
  126     inputParameters.sampleFormat = PA_SAMPLE_TYPE;
  127     inputParameters.suggestedLatency = inputInfo->defaultHighInputLatency ;
  128     inputParameters.hostApiSpecificStreamInfo = NULL;
  129 
  130     outputParameters.channelCount = numChannels;
  131     outputParameters.sampleFormat = PA_SAMPLE_TYPE;
  132     outputParameters.suggestedLatency = outputInfo->defaultHighOutputLatency;
  133     outputParameters.hostApiSpecificStreamInfo = NULL;
  134 
  135     /* -- setup -- */
  136 
  137     err = Pa_OpenStream(
  138               &stream,
  139               &inputParameters,
  140               &outputParameters,
  141               SAMPLE_RATE,
  142               FRAMES_PER_BUFFER,
  143               paClipOff,      /* we won't output out of range samples so don't bother clipping them */
  144               NULL, /* no callback, use blocking API */
  145               NULL ); /* no callback, so no callback userData */
  146     if( err != paNoError ) goto error2;
  147 
  148     numBytes = FRAMES_PER_BUFFER * numChannels * SAMPLE_SIZE ;
  149     sampleBlock = (char *) malloc( numBytes );
  150     if( sampleBlock == NULL )
  151     {
  152         printf("Could not allocate record array.\n");
  153         goto error1;
  154     }
  155     memset( sampleBlock, SAMPLE_SILENCE, numBytes );
  156 
  157     err = Pa_StartStream( stream );
  158     if( err != paNoError ) goto error1;
  159     printf("Wire on. Will run %d seconds.\n", NUM_SECONDS); fflush(stdout);
  160 
  161     for( i=0; i<(NUM_SECONDS*SAMPLE_RATE)/FRAMES_PER_BUFFER; ++i )
  162     {
  163         // You may get underruns or overruns if the output is not primed by PortAudio.
  164         err = Pa_WriteStream( stream, sampleBlock, FRAMES_PER_BUFFER );
  165         if( err ) goto xrun;
  166         err = Pa_ReadStream( stream, sampleBlock, FRAMES_PER_BUFFER );
  167         if( err ) goto xrun;
  168     }
  169     printf("Wire off.\n"); fflush(stdout);
  170 
  171     err = Pa_StopStream( stream );
  172     if( err != paNoError ) goto error1;
  173 
  174     free( sampleBlock );
  175 
  176     Pa_Terminate();
  177     return 0;
  178 
  179 xrun:
  180     printf("err = %d\n", err); fflush(stdout);
  181     if( stream ) {
  182         Pa_AbortStream( stream );
  183         Pa_CloseStream( stream );
  184     }
  185     free( sampleBlock );
  186     Pa_Terminate();
  187     if( err & paInputOverflow )
  188         fprintf( stderr, "Input Overflow.\n" );
  189     if( err & paOutputUnderflow )
  190         fprintf( stderr, "Output Underflow.\n" );
  191     return -2;
  192 error1:
  193     free( sampleBlock );
  194 error2:
  195     if( stream ) {
  196         Pa_AbortStream( stream );
  197         Pa_CloseStream( stream );
  198     }
  199     Pa_Terminate();
  200     fprintf( stderr, "An error occurred while using the portaudio stream\n" );
  201     fprintf( stderr, "Error number: %d\n", err );
  202     fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
  203     return -1;
  204 }