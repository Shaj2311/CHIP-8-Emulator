#include "chip8.h"
#include "instructions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define DBG_ROM_NAME "test.rom"

void chip8_reset_hardware();
void chip8_load_rom(char *romName);
void chip8_init();
void interpret_opcode(uint16_t opcode);

//hexadecimal character font set
uint8_t fontset[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, //0
	0x20, 0x60, 0x20, 0x20, 0x70, //1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
	0x90, 0x90, 0xF0, 0x10, 0x10, //4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
	0xF0, 0x10, 0x20, 0x40, 0x40, //7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
	0xF0, 0x90, 0xF0, 0x90, 0x90, //A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
	0xF0, 0x80, 0x80, 0x80, 0xF0, //C
	0xE0, 0x90, 0x90, 0x90, 0xE0, //D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
	0xF0, 0x80, 0xF0, 0x80, 0x80, //F
};

//VM instantiation
Chip8 chip8;

int main()
{
	srand(time(0));

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

		//move forward by two bytes
		chip8.pc += 2;

		interpret_opcode(opcode);
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
	uint16_t addr = opcode & 0x0FFF;
	uint8_t regx = (opcode & 0x0F00) >> 8;
	uint8_t regy = (opcode & 0x00F0) >> 4;
	uint8_t byteVal = (opcode & 0x00FF);
	uint8_t nibbleVal = opcode & 0x000F;

	//check opcode family
	switch(opcodeFamily)
	{
		case 0:
			if(opcode == 0x00E0)
				CLS();
			else if(opcode == 0x00EE)
				RET();
			else
				SYS(addr);
			break;
		case 1:
			JP(addr);
			break;
		case 2:
			CALL(addr);
			break;
		case 3:
			SEvb(regx, byteVal);
			break;
		case 4:
			SNEvb(regx, byteVal);
			break;
		case 5:
			SEvv(regx, regy);
			break;
		case 6:
			LDvb(regx, byteVal);
			break;
		case 7:
			ADD(regx, byteVal);
			break;
		case 8:
			switch(opcode & 0x000F)
			{
				case 0:
					LDvv(regx, regy);
					break;
				case 1:
					OR(regx, regy);
					break;
				case 2:
					AND(regx, regy);
					break;
				case 3:
					XOR(regx, regy);
					break;
				case 4:
					ADDc(regx, regy);
					break;
				case 5:
					SUBc(regx, regy);
					break;
				case 6:
					SHR(regx);
					break;
				case 7:
					SUBN(regx, regy);
					break;
				default:
					puts("Invalid opcode detected");
					exit(1);
			}
			break;
		case 9:
			SNEvv(regx, regy);
			break;
		case 0xA:
			LDi(addr);
			break;
		case 0xB:
			JPv(addr);
			break;
		case 0xC:
			RND(regx, byteVal);
			break;
		case 0xD:
			DRW(regx, regy, nibbleVal);
			break;
		case 0xE:
			SKP(regx);
			break;
		case 0xF:
			switch(opcode & 0x00FF)
			{
				case 0x0A:
					LDvk(regx);
					break;
				case 0x15:
					LDdv(regx);
					break;
				case 0x18:
					LDsv(regx);
					break;
				case 0x1E:
					ADDI(regx);
					break;
				case 0x29:
					LDf(regx);
					break;
				case 0x33:
					LDb(regx);
					break;
				case 0x55:
					LDiv(regx);
					break;
				case 0x65:
					LDvi(regx);
					break;
				default:
					puts("Invalid opcode detected");
					exit(1);
			}
			break;
	}
}
