#ifndef _Memory_Monitor_H
#define _Memory_Monitor_H
#include "param.h"
#include <elf.h>
#include <math.h>

#define MEMSIZE 0x10000000
class Memory
{
	public:
		Memory(char* filename);
		~Memory();
		void* Load(lint ad,int length);
		void Store(lint ad,int length,char*content);
		void* Translate(lint ad);
		Elf64_Addr entry;
		Elf64_Addr inisp;
	private:
		char simumem[MEMSIZE];// virtual memory
};
#endif