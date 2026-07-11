#include <stdint.h>
typedef struct
{
	//GPRs
	uint8_t V[16]; //V0 - VF

	uint16_t idx; //index reg

	uint8_t stack[64];
	uint8_t sptr; //stack ptr

	//timers
	uint8_t delay;
	uint8_t sound;

	uint8_t frameBuffer[(64 * 32) / 8]; //display
	uint16_t pc; //program counter
	uint8_t mem[4096]; //RAM

	uint8_t keypad[16]; //16 keys
} Chip8;
