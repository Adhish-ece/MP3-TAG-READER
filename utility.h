#ifndef UTITLITY_H
#define UTITLITY_H
#include "common.h"


OperationType GetOperatioType(int argc,char *argv[],OperationType *flag);
Status View();
uint SynsafeToInt(char size[4]);
uint BigEndianToInt(char size[4]);
void UTFtoASCII(const char *utf16_data, char *ascii_outuput);
Status Edit(OperationType op);
void Printing_Help();
void IntToBigEndian(uint value, char out[4]);
void IntToSynsafe(uint value, char out[4]);


#endif