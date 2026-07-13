#ifndef CHIP8_H
#define CHIP8_H
#include <stdint.h>
#define CHIP8_FRAME_WIDTH 64
#define CHIP8_FRAME_HEIGHT 32

typedef struct
{
	//GPRs
	uint8_t vregs[16]; //V0 - VF

	uint16_t idx; //index reg

	uint16_t stack[16];
	uint8_t sp; //stack ptr

	//timers
	uint8_t delay;
	uint8_t sound;

	uint8_t frameBuffer[CHIP8_FRAME_WIDTH * CHIP8_FRAME_HEIGHT]; //display
	uint16_t pc; //program counter
	uint8_t mem[4096]; //RAM

	uint8_t keypad[16]; //16 keys
} Chip8;

extern uint8_t fontset[80];
extern Chip8 chip8;

#endif
