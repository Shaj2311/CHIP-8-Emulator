#include "chip8.h"
#include "instructions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define DEFAULT_CYCLES_PER_FRAME 10

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define SDL_WINDOW_WIDTH 640
#define SDL_WINDOW_HEIGHT 320
#define SDL_AUDIO_SAMPLE_RATE 44100
#define SDL_AUDIO_FREQUENCY 440
#define SDL_AUDIO_VOLUME 3000

void chip8_reset_hardware();
void chip8_load_rom(char *romName);
void chip8_init();
void FDE_cycle();
void interpret_opcode(uint16_t opcode);

void sdl_init();
void sdl_render_frame();
void sdl_fill_audio_buffer();
void sdl_poll_events();
void sdl_cleanup();

void parseInput(int argc, char **argv);

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

//SDL Globals
SDL_Window *sdl_window;
SDL_Renderer *sdl_renderer;
SDL_Texture *sdl_frame_texture;
SDL_AudioStream *sdl_audio_stream;
uint32_t sdl_render_pixels[64 * 32];
uint64_t sdl_perf_freq;
int16_t sdl_audio_samples[SDL_AUDIO_SAMPLE_RATE];
int sdl_running = 1;

//Runtime Globals
char *ROM_NAME;
int CYCLES_PER_FRAME;

int main(int argc, char **argv)
{
	//parse command line args (ROM name and custom cycles per frame)
	parseInput(argc, argv);

	//set performance frequency (to convert ticks to milliseconds later)
	sdl_perf_freq = SDL_GetPerformanceFrequency();

	//set random seed
	srand(time(0));

	//initialize machine and load ROM
	chip8_init();

	//Initialize SDL
	sdl_init();

	puts("Executing ROM");
	//FDE cycle
	while(sdl_running)
	{
		uint64_t startTicks = SDL_GetPerformanceCounter();

		sdl_fill_audio_buffer();

		sdl_poll_events();

		//run INSTRUCT_PER_ITER instructions
		for(int i = 0; i < CYCLES_PER_FRAME; i++)
			FDE_cycle();

		sdl_render_frame();

		//get end ticks
		uint64_t endTicks = SDL_GetPerformanceCounter();

		//calculate time delta and apply pace limiting
		double milliseconds = ((double)(endTicks - startTicks) / sdl_perf_freq) * 1000;
		if(milliseconds < (1000.f/60))
			SDL_Delay((uint32_t)((1000.f/60) - milliseconds));

		//update timers
		if(chip8.delay)
			chip8.delay--;
		if(chip8.sound)
		{
			chip8.sound--;
		}

		//update audio status
		if(chip8.sound == 0)
			SDL_PauseAudioStreamDevice(sdl_audio_stream);
		else
			SDL_ResumeAudioStreamDevice(sdl_audio_stream);
	}

	sdl_cleanup();
	puts("Shutting down");
	return 0;
}

void parseInput(int argc, char **argv)
{
	if(argc < 2)
	{
		puts("ROM not specified");
		exit(1);
	}
	if(argc > 3)
	{
		printf("Invalid argument: %s\n", argv[3]);
		exit(1);
	}

	//Parse input
	ROM_NAME = argv[1];
	if(argc == 3)
	{
		if(!(CYCLES_PER_FRAME = (int)strtol(argv[2], 0, 10)))
		{
			printf("Invalid value: %d\n", CYCLES_PER_FRAME);
			exit(1);
		}
	}
	else
		CYCLES_PER_FRAME = DEFAULT_CYCLES_PER_FRAME;
	printf("INSTRUCT_PER_ITER: %d\n", CYCLES_PER_FRAME);
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
	chip8_load_rom(ROM_NAME);

	//set program counter to 0x200
	chip8.pc = 0x200;
}

void FDE_cycle()
{
	//use fancy bit manipulation to read Big Endian to Big Endian
	//(directly fetching a uint16_t value will flip the bytes around as
	// it's expecting them to be in Little Endian)
	uint16_t opcode = (chip8.mem[chip8.pc] << 8) | chip8.mem[chip8.pc + 1];

	//move forward by two bytes
	chip8.pc += 2;
	//loop back if reached end of memory
	if(chip8.pc > 0xFFF)
		chip8.pc = 0x200;

	interpret_opcode(opcode);
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
			if(nibbleVal == 0)
			{
				SEvv(regx, regy);
				break;
			}
			else
			{
				printf("Invalid opcode detected: 0x%04x\n", opcode);
				exit(1);
			}
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
				case 0xE:
					SHL(regx);
					break;
				default:
					printf("Invalid opcode detected: 0x%04x\n", opcode);
					exit(1);
			}
			break;
		case 9:
			if(nibbleVal == 0)
			{
				SNEvv(regx, regy);
				break;
			}
			else
			{
				printf("Invalid opcode detected: 0x%04x\n", opcode);
				exit(1);
			}
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
			switch(opcode & 0x00FF)
			{
				case 0x9E:
					SKP(regx);
					break;
				case 0xA1:
					SKNP(regx);
					break;
				default:
					printf("Invalid opcode detected: 0x%04x\n", opcode);
					exit(1);
			}
			break;
		case 0xF:
			switch(byteVal)
			{
				case 0x07:
					LDvd(regx);
					break;
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
					printf("Invalid opcode detected: 0x%04x\n", opcode);
					exit(1);
			}
			break;
	}
}

void sdl_init()
{
	//Initialize video and audio
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	//set up window, renderer and texture
	sdl_window = SDL_CreateWindow("CHIP-8", SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, 0);
	sdl_renderer = SDL_CreateRenderer(sdl_window, 0);
	sdl_frame_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 64, 32);
	SDL_SetTextureScaleMode(sdl_frame_texture, SDL_SCALEMODE_NEAREST);

	//set up audio
	//define audio specs
	SDL_AudioSpec spec;
	spec.channels = 1;
	spec.format = SDL_AUDIO_S16LE;
	spec.freq = SDL_AUDIO_SAMPLE_RATE;
	//open audio stream
	sdl_audio_stream = SDL_OpenAudioDeviceStream(
			SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
			&spec,
			0,
			0
			);
	//create square wave audio samples
	for(int i = 0; i < SDL_AUDIO_SAMPLE_RATE; i++)
	{
		int cyclePeriod = SDL_AUDIO_SAMPLE_RATE / SDL_AUDIO_FREQUENCY;
		sdl_audio_samples[i] = (i % cyclePeriod) < (cyclePeriod / 2) ? SDL_AUDIO_VOLUME : -SDL_AUDIO_VOLUME;
	}
	//push audio into stream queue
	SDL_PutAudioStreamData(sdl_audio_stream, sdl_audio_samples, sizeof(sdl_audio_samples));
}

void sdl_render_frame()
{
	//prepare renderPixels
	for(int i = 0; i < 64 * 32; i++)
		sdl_render_pixels[i] = chip8.frameBuffer[i] ? 0xFFFFFFFF : 0x000000FF;

	//update texture to draw
	SDL_UpdateTexture(sdl_frame_texture, 0, sdl_render_pixels, 64 * 4); //one row is 64 positions wide, 4 bytes for one pixel (RGBA)
	//clear screen
	SDL_RenderClear(sdl_renderer);
	//draw texture onto screen
	SDL_RenderTexture(sdl_renderer, sdl_frame_texture, 0, 0); //0,0 means put WHOLE texture onto WHOLE screen
	//display
	SDL_RenderPresent(sdl_renderer);
}

void sdl_fill_audio_buffer()
{
	int16_t bytesQueued = SDL_GetAudioStreamQueued(sdl_audio_stream);

	if(
			SDL_GetAudioStreamQueued(sdl_audio_stream)
			<
			(int16_t)((SDL_AUDIO_SAMPLE_RATE * sizeof(int16_t)) / 10)
			)
		SDL_PutAudioStreamData(sdl_audio_stream, sdl_audio_samples, sizeof(sdl_audio_samples));
}

void sdl_poll_events()
{
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_EVENT_QUIT:
				sdl_running = 0;
				break;
			case SDL_EVENT_KEY_DOWN:
				if(event.key.repeat)
					break;
				puts("Key down");
				switch(event.key.key)
				{
					case SDLK_1: //1
						chip8.keypad[1] = 1;
						break;
					case SDLK_2: //2
						chip8.keypad[2] = 1;
						break;
					case SDLK_3: //3
						chip8.keypad[3] = 1;
						break;
					case SDLK_4: //C
						chip8.keypad[0xC] = 1;
						break;

					case SDLK_Q: //4
						chip8.keypad[4] = 1;
						break;
					case SDLK_W: //5
						chip8.keypad[5] = 1;
						break;
					case SDLK_E: //6
						chip8.keypad[6] = 1;
						break;
					case SDLK_R: //D
						chip8.keypad[0xD] = 1;
						break;

					case SDLK_A://7
						chip8.keypad[7] = 1;
						break;
					case SDLK_S: //8
						chip8.keypad[8] = 1;
						break;
					case SDLK_D: //9
						chip8.keypad[9] = 1;
						break;
					case SDLK_F: //E
						chip8.keypad[0xE] = 1;
						break;

					case SDLK_Z: //A
						chip8.keypad[0xA] = 1;
						break;
					case SDLK_X: //0
						chip8.keypad[0] = 1;
						break;
					case SDLK_C: //B
						chip8.keypad[0xB] = 1;
						break;
					case SDLK_V: //F
						chip8.keypad[0xF] = 1;
						break;
				}
				break;
			case SDL_EVENT_KEY_UP:
				puts("Key up");
				switch(event.key.key)
				{
					case SDLK_1: //1
						chip8.keypad[1] = 0;
						break;
					case SDLK_2: //2
						chip8.keypad[2] = 0;
						break;
					case SDLK_3: //3
						chip8.keypad[3] = 0;
						break;
					case SDLK_4: //C
						chip8.keypad[0xC] = 0;
						break;

					case SDLK_Q: //4
						chip8.keypad[4] = 0;
						break;
					case SDLK_W: //5
						chip8.keypad[5] = 0;
						break;
					case SDLK_E: //6
						chip8.keypad[6] = 0;
						break;
					case SDLK_R: //D
						chip8.keypad[0xD] = 0;
						break;

					case SDLK_A://7
						chip8.keypad[7] = 0;
						break;
					case SDLK_S: //8
						chip8.keypad[8] = 0;
						break;
					case SDLK_D: //9
						chip8.keypad[9] = 0;
						break;
					case SDLK_F: //E
						chip8.keypad[0xE] = 0;
						break;

					case SDLK_Z: //A
						chip8.keypad[0xA] = 0;
						break;
					case SDLK_X: //0
						chip8.keypad[0] = 0;
						break;
					case SDLK_C: //B
						chip8.keypad[0xB] = 0;
						break;
					case SDLK_V: //F
						chip8.keypad[0xF] = 0;
						break;
				}
				break;
		}
	}
}

void sdl_cleanup()
{
	SDL_DestroyRenderer(sdl_renderer);
	SDL_DestroyWindow(sdl_window);
	SDL_DestroyAudioStream(sdl_audio_stream);
	SDL_Quit();
}
