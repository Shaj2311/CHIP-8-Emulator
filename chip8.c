#include <stdint.h>
typedef struct
{
	//GPRs
	uint8_t V0;
	uint8_t V1;
	uint8_t V2;
	uint8_t V3;
	uint8_t V4;
	uint8_t V5;
	uint8_t V6;
	uint8_t V7;
	uint8_t V8;
	uint8_t V9;
	uint8_t VA;
	uint8_t VB;
	uint8_t VC;
	uint8_t VD;
	uint8_t VE;
	uint8_t VF;

	uint16_t idx; //index reg

	uint8_t stack[64];
	uint8_t sptr; //stack ptr

	//timers
	uint8_t delay;
	uint8_t sound;

	uint8_t frameBuffer[(64 * 32) / 8]; //display
	uint16_t pc; //program counter
	uint8_t mem[4096]; //RAM
} Chip8;
