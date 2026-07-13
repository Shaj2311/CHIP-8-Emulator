# CHIP-8-Emulator
Clean, lightweight CHIP-8 emulator

## Description
This is a lightweight, memory efficient CHIP-8 emulator written in C, that uses SDL3 for audio and graphics rendering.

## Features
* Hardware emulation according to CHIP-8 documentation
* Full implementation of CHIP-8's instruction set
* Audio and graphics handled via SDL3
* Configurable clock speed (custom cycles per frame)

## Requirements
* SDL3

## Getting Started
Make sure you have SDL3 installed.<BR>
Then, clone this repository and compile the emulator:
```
git clone https://github.com/Shaj2311/CHIP-8-Emulator
cd ./CHIP-8-Emulator
make
./chip8 -h
```
