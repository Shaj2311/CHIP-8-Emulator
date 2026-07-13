#include "instructions.h"
#include "chip8.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void SYS(uint16_t addr)
{
	//The documentation says this instruction is ignored by
	//modern interpreters. Okay :)
}

void CLS()
{
	//clear screen
	memset(chip8.frameBuffer, 0, sizeof(chip8.frameBuffer));
}

void RET()
{
	//return from subroutine
	chip8.pc = chip8.stack[chip8.sp--];
}

void JP(uint16_t addr)
{
	//jump to addr
	chip8.pc = addr;
}

void CALL(uint16_t addr)
{
	//call subroutine at addr
	chip8.stack[++(chip8.sp)] = chip8.pc;
	chip8.pc = addr;
}

void SEvb(uint8_t regx, uint8_t byte)
{
	//skip (next instruction) if register x == byte
	if(chip8.vregs[regx] == byte)
		chip8.pc += 2;
}

void SNEvb(uint8_t regx, uint8_t byte)
{
	//skip (next instruction) if register x != byte
	if(chip8.vregs[regx] != byte)
		chip8.pc += 2;
}

void SEvv(uint8_t regx, uint8_t regy)
{
	//skip (next instruction if register4 x == register y)
	if(chip8.vregs[regx] == chip8.vregs[regy])
		chip8.pc += 2;
}

void LDvb(uint8_t regx, uint8_t byte)
{
	//set register x to byte
	chip8.vregs[regx] = byte;
}

void ADD(uint8_t regx, uint8_t byte)
{
	//register x += byte
	chip8.vregs[regx] += byte;
}

void LDvv(uint8_t regx, uint8_t regy)
{
	//set register x to register y
	chip8.vregs[regx] = chip8.vregs[regy];
}

void OR(uint8_t regx, uint8_t regy)
{
	//register x = bitwise OR of registers x and y
	chip8.vregs[regx] |= chip8.vregs[regy];
}

void AND(uint8_t regx, uint8_t regy)
{
	//register x = bitwise AND of registers x and y
	chip8.vregs[regx] &= chip8.vregs[regy];
}

void XOR(uint8_t regx, uint8_t regy)
{
	//register x = bitwise XOR of registers x and y
	chip8.vregs[regx] ^= chip8.vregs[regy];
}

void ADDc(uint8_t regx, uint8_t regy)
{
	//register x += register y, register F = carry
	uint16_t result = (uint16_t)chip8.vregs[regx] + (uint16_t)chip8.vregs[regy];
	chip8.vregs[regx] += chip8.vregs[regy];
	chip8.vregs[0xF] = result > 0xFF;
}

void SUBc(uint8_t regx, uint8_t regy)
{
	//register x -= register y, register F = NOT borrow
	uint8_t notBorrow = chip8.vregs[regx] > chip8.vregs[regy];
	chip8.vregs[regx] -= chip8.vregs[regy];
	chip8.vregs[0xF] = notBorrow;
}

void SHR(uint8_t regx)
{
	//bit-shift register x right by 1 bit
	chip8.vregs[0xF] = chip8.vregs[regx] & 0x0001;
	chip8.vregs[regx] /= 2;
}

void SUBN(uint8_t regx, uint8_t regy)
{
	//register x -= register y, register F = NOT borrow
	chip8.vregs[0xF] = chip8.vregs[regy] > chip8.vregs[regx];
	chip8.vregs[regx] = chip8.vregs[regy] - chip8.vregs[regx];
}

void SHL(uint8_t regx)
{
	//bit-shift register x left by 1 bit
	chip8.vregs[0xF] = chip8.vregs[regx] & 0x80;
	chip8.vregs[regx] *= 2;
}

void SNEvv(uint8_t regx, uint8_t regy)
{
	//skip next instruction if register x == register y
	if(chip8.vregs[regx] != chip8.vregs[regy])
		chip8.pc += 2;
}

void LDi(uint16_t addr)
{
	//set index register to addr
	chip8.idx = addr;
}

void JPv(uint16_t addr)
{
	//jump to location (register 0 + addr)
	chip8.pc = addr + chip8.vregs[0];
}

void RND(uint8_t regx, uint8_t byte)
{
	//set register x to (random_value AND byte)
	chip8.vregs[regx] = (rand() % 256) & byte;
}

void DRW(uint8_t regx, uint8_t regy, uint8_t nibble)
{
	//draw sprite at location on screen

	//reset collision flag
	chip8.vregs[0xF] = 0;

	//draw
	uint8_t startx = chip8.vregs[regx], starty = chip8.vregs[regy];
	//height
	for(int i = 0; i < nibble; i++)
	{
		//get a byte from sprite
		uint8_t sprite = chip8.mem[(chip8.idx + i) % 4096];
		uint8_t x,y;

		y = (starty + i) % 32;

		//width
		for(int j = 0; j < 8; j++)
		{
			//if current sprite bit is off, skip
			if(!(sprite & (0x80 >> j)))
				continue;

			x = (startx + j) % 64;

			//calculate screen position
			uint16_t frameIndex = (y * 64) + x;

			//check if current drawn bit is on (collision)
			if(chip8.frameBuffer[frameIndex])
				chip8.vregs[0xF] = 1;

			//XOR onto screen
			chip8.frameBuffer[frameIndex] ^= 1;
		}
	}

}

void SKP(uint8_t regx)
{
	//skip next instruction if key with value of x is pressed
	if(chip8.keypad[chip8.vregs[regx]])
		chip8.pc += 2;
}

void SKNP(uint8_t regx)
{
	//skip next instruction if key with value of x is not pressed
	if(!chip8.keypad[chip8.vregs[regx]])
		chip8.pc += 2;
}

void LDvd(uint8_t regx)
{
	//set register x to value of delay timer
	chip8.vregs[regx] = chip8.delay;
}

void LDvk(uint8_t regx)
{
	//wait for key press, store in register x

	unsigned char pressed = 0;

	//check all keys
	for(int i = 0; i < 0xF; i++)
	{
		//key pressed
		if(chip8.keypad[i])
		{
			//store key
			chip8.vregs[regx] = i;
			pressed = 1;
			break;
		}
	}
	//if no key pressed, run this instruction again
	if(!pressed)
		chip8.pc -= 2;
}

void LDdv(uint8_t regx)
{
	//set delay timer to value in register x
	chip8.delay = chip8.vregs[regx];
}

void LDsv(uint8_t regx)
{
	//set sound timer to value in register x
	chip8.sound = chip8.vregs[regx];
}

void ADDI(uint8_t regx)
{
	//add value of register x to index register
	chip8.idx += chip8.vregs[regx];
}

void LDf(uint8_t regx)
{
	//point index register to sprite for value of register x
	chip8.idx = 0x50 + ((chip8.vregs[regx] & 0x0F) * 5);
}

void LDb(uint8_t regx)
{
	//store BCD representation of value in register x
	//at location pointed to by index register
	uint8_t value = chip8.vregs[regx];
	chip8.mem[chip8.idx] = value / 100;
	chip8.mem[chip8.idx + 1] = (value % 100) / 10;
	chip8.mem[chip8.idx + 2] = value % 10;
}

void LDiv(uint8_t regx)
{
	//store register values from V0 to Vx in memory
	//starting at location pointed to by index register
	for(int i = 0; i <= regx; i++)
	{
		if(chip8.idx + i > 0xFFF)
		{
			puts("Attempted to write outside of memory, aborting\n");
			exit(1);
		}

		chip8.mem[chip8.idx + i] = chip8.vregs[i];
	}
}

void LDvi(uint8_t regx)
{
	for(int i = 0; i <= regx; i++)
	{
		if(chip8.idx + i > 0xFFF)
		{
			puts("Attempted to read outside of memory, aborting\n");
			exit(1);
		}
		chip8.vregs[i] = chip8.mem[chip8.idx + i];
	}
}
