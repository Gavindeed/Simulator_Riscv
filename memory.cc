#include "memory.h"
#include <memory.h>
Memory::Memory(char* filename)
{
	memset(simumem, 0, MEMSIZE);
	if(!filename)
		return;
	inisp=MEMSIZE-8;
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
void Memory::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
  hit = 1;
  time = latency_.hit_latency + latency_.bus_latency;
  stats_.access_time += time;
  //#ifdef SIMULATE
  //printf("memory addr: %llx\n", addr);
  if(read==1)
  {
  	memcpy(content,simumem+addr,bytes);
  }
  else
  {
  	memcpy(simumem+addr,content,bytes);
  }
  //#endif
}

