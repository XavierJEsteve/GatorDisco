#define SPI_MODULE_OSC 0b10000000
#define SPI_MODULE_KEYBOARD 0b10010000
#define SPI_MODULE_LFO 0b10100000
#define SPI_MODULE_ENV 0b10110000
#define SPI_MODULE_FILTER 0b11010000
#define SPI_MODULE_FX 0b11000000

#define SPI_OSCTYPE 0
#define SPI_OSCPARAM1 1
#define SPI_OSCPARAM2 2
#define SPI_OSC_WAVFREQ 3
#define SPI_KEYBOARD_KEY 0
#define SPI_KEYBOARD_OCTAVE 1
#define SPI_LFO_SPEED 0
#define SPI_LFO_VAL 1
#define SPI_LFO_TARGET 2
#define SPI_ENV_ATTACK 0
#define SPI_ENV_DECAY 1
#define SPI_ENV_SUSTAIN 2
#define SPI_ENV_RELEASE 3
#define SPI_GATE 4
#define SPI_FX_SEL 0
#define SPI_FX_PARAM1 1
#define SPI_FX_PARAM2 2