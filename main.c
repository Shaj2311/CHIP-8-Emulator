#include "chip8.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define DBG_ROM_NAME "test.rom"

//Create VM
Chip8 chip8;

void chip8_reset_hardware();
void chip8_load_rom(char *romName);
void chip8_init();
void interpret_opcode(uint16_t opcode);

int main()
{
	//initialize machine and load ROM
	chip8_init();

	//FDE cycle
	for(int i = 0; i < 35; i++)
	{
		//use fancy bit manipulation to read Big Endian to Big Endian
		//(directly fetching a uint16_t value will flip the bytes around as
		// it's expecting them to be in Little Endian)
		uint16_t opcode = (chip8.mem[chip8.pc] << 8) | chip8.mem[chip8.pc + 1];

		printf("PC value: %04x | Read opcode: %04x\n", chip8.pc, opcode);
		interpret_opcode(opcode);

		//move forward by two bytes
		chip8.pc += 2;
	}
}

void chip8_reset_hardware()
{
	//reset everything to zero

	memset(chip8.vregs, 0, sizeof(chip8.vregs));
	chip8.idx = 0;

	memset(chip8.stack, 0, sizeof(chip8.stack));
	chip8.sp = 0;

	chip8.delay = 0;
	chip8.sound = 0;

	memset(chip8.frameBuffer, 0, sizeof(chip8.frameBuffer));
	chip8.pc = 0;
	memset(chip8.mem, 0, sizeof(chip8.mem));

	memset(chip8.keypad, 0, sizeof(chip8.keypad));
}

void chip8_load_rom(char *romName)
{
	printf("Loading ROM %s\n", romName);

	//open file
	FILE *rom = fopen(romName, "rb");
	if(rom == 0)
	{
		perror("fopen");
		exit(1);
	}

	//check file size
	if(fseek(rom, 0, SEEK_END) == -1)
	{
		perror("fseek (check file size)");
		exit(1);
	}
	long romSize = ftell(rom);
	if(romSize == -1)
	{
		perror("ftell");
		exit(1);
	}

	//exit if ROM too big
	if(romSize > (0xFFF - 0x200))
	{
		puts("Error: ROM size exceeds memory");
		exit(1);
	}

	//move file offset back to start before reading
	if(fseek(rom, 0, SEEK_SET) == -1)
	{
		perror("fseek (reset file offset)");
		exit(1);
	}

	//load ROM into memory
	if(fread(chip8.mem + 0x200, 1, (size_t)romSize, rom) < (size_t)romSize)
	{
		if(ferror(rom))
		{
			perror("fread");
			exit(1);
		}
	}

	//close file
	if(fclose(rom) == EOF)
	{
		perror("fclose");
		exit(1);
	}

	puts("ROM loaded successfully");
}

void chip8_init()
{
	//set all hardware to zero
	chip8_reset_hardware();

	//load fontset into memory
	memcpy(chip8.mem + 0x050, fontset, sizeof(fontset));

	//load target program to memory
	chip8_load_rom(DBG_ROM_NAME);

	//set program counter to 0x200
	chip8.pc = 0x200;
}

void interpret_opcode(uint16_t opcode)
{
	//extract information
	uint8_t opcodeFamily = opcode >> 12;
	uint8_t addr = opcode & 0x0FFF;
	uint8_t regx = (opcode & 0x0F00) >> 8;
	uint8_t regy = (opcode & 0x00F0) >> 4;
	uint8_t byteVal = (opcode & 0x00FF);
	uint8_t nibbleVal = opcode & 0x000F;

	//check opcode family
	switch(opcodeFamily)
	{
		case 0:
			puts("We got 0");
			break;
		case 1:
			puts("We got 1");
			break;
		case 2:
			puts("We got 2");
			break;
		case 3:
			puts("We got 3");
			break;
		case 4:
			puts("We got 4");
			break;
		case 5:
			puts("We got 5");
			break;
		case 6:
			puts("We got 6");
			break;
		case 7:
			puts("We got 7");
			break;
		case 8:
			puts("We got 8");
			break;
		case 9:
			puts("We got 9");
			break;
		case 0xA:
			puts("We got A");
			break;
		case 0xB:
			puts("We got B");
			break;
		case 0xC:
			puts("We got C");
			break;
		case 0xD:
			puts("We got D");
			break;
		case 0xE:
			puts("We got E");
			break;
		case 0xF:
			puts("We got F");
			break;
	}
}
