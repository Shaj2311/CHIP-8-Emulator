#ifndef INSTRUCT_H
#define INSTRUCT_H
#include <stdint.h>

void SYS(uint16_t addr);
void CLS();
void RET();
void JP(uint16_t addr);
void CALL(uint16_t addr);
void SEvb(uint8_t regx, uint8_t byte);
void SNEvb(uint8_t regx, uint8_t byte);
void SEvv(uint8_t regx, uint8_t regy);
void LDvb(uint8_t regx, uint8_t byte);
void ADD(uint8_t regx, uint8_t byte);
void LDvv(uint8_t regx, uint8_t regy);
void OR(uint8_t regx, uint8_t regy);
void AND(uint8_t regx, uint8_t regy);
void XOR(uint8_t regx, uint8_t regy);
void ADDc(uint8_t regx, uint8_t regy);
void SUBc(uint8_t regx, uint8_t regy);
void SHR(uint8_t regx);
void SUBN(uint8_t regx, uint8_t regy);
void SHL(uint8_t regx);
void SNEvv(uint8_t regx, uint8_t regy);
void LDi(uint16_t addr);
void JPv(uint16_t addr);
void RND(uint8_t regx, uint8_t byte);
void DRW(uint8_t regx, uint8_t regy, uint8_t nibble);
void SKP(uint8_t regx);
void SKNP(uint8_t regx);
void LDvd(uint8_t regx);
void LDvk(uint8_t regx);
void LDdv(uint8_t regx);
void LDsv(uint8_t regx);
void ADDI(uint8_t regx);
void LDf(uint8_t regx);
void LDb(uint8_t regx);
void LDiv(uint8_t regx);
void LDvi(uint8_t regx);

#endif
