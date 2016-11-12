//#ifndef _Memory_Monitor_CC
//#define _Memory_Monitor_CC
#include "memory_monitor.h"
#include <memory.h>
#include <stdio.h>
Memory::Memory(char* filename)
{
	inisp=MEMSIZE-1;
	char buff_ehdr[70];
	Elf64_Ehdr* p_ehdr;
	FILE* fd=fopen(filename,"rb");
	fread(buff_ehdr,1,64,fd);
	p_ehdr=(Elf64_Ehdr*)buff_ehdr;//pointer to elf header;
	entry = p_ehdr->e_entry;// entry address
	Elf64_Off	prooff=p_ehdr->e_phoff;
	Elf64_Half	pronum=p_ehdr->e_phnum;//56
	fseek(fd,prooff,SEEK_SET);
	for(int i=0;i<pronum;i++)
	{
		char buff_phdr[60];
		Elf64_Phdr* p_phdr;
		fread(buff_phdr,1,56,fd);
		p_phdr=(Elf64_Phdr*)buff_phdr;
		if(p_phdr->p_type==PT_LOAD)
		{
			Elf64_Addr SegAd=p_phdr->p_vaddr;
			Elf64_Off SegOff=p_phdr->p_offset;
			Elf64_Xword SegSize=p_phdr->p_memsz;
			fseek(fd,SegOff,SEEK_SET);
			fread(simumem+SegAd,1,SegSize,fd);
		}
	}
	fclose(fd);
}

Memory::~Memory()
{
	printf("Memory Over");	
}

void* 
Memory::Load(lint ad,int length)
{
	char* loadcontent=new char[length];
	memcpy(loadcontent,simumem+ad,length);
	return loadcontent;
}

void
Memory::Store(lint ad,int length,char*content)
{
	memcpy(simumem+ad,content,length);
	return;
}
//#endif

/*
int main(int argc,char** argv)
{
	Memory* MyMemory=new Memory(argv[1]);
	printf("Entry Point:%x\n",MyMemory->entry);
	return 0;
}
*/