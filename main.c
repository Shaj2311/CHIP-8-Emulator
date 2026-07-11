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

int main()
{
	chip8_init();
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
