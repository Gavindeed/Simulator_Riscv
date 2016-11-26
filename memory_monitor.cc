//#ifndef _Memory_Monitor_CC
//#define _Memory_Monitor_CC
#include "memory_monitor.h"
#include <memory.h>
#include <stdio.h>
MemoryMonitor::MemoryMonitor(char* filename)
{
	/*
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
	*/

	memory = new Memory(filename);
	llc = new Cache((1<<23), (1<<14), 8, 0, 1, memory);
	l2 = new Cache((1<<18), (1<<9), 8, 0, 1, llc);
	l1 = new Cache(256, 1, 16, 1, 0, memory);
	//l1 = new Cache((1<<15), (1<<6), 8, 0, 1, memory);
	entry = memory->entry;
	inisp = memory->inisp;

	StorageStats s;
	s.access_time = 0;
	memory->SetStats(s);
	llc->SetStats(s);
	l2->SetStats(s);
	l1->SetStats(s);

	StorageLatency ml;
	ml.bus_latency = 6;
	ml.hit_latency = 100;
	memory->SetLatency(ml);

	StorageLatency ll;
	ll.bus_latency = 3;
	ll.hit_latency = 10;
	llc->SetLatency(ll);

	ll.bus_latency = 2;
	ll.hit_latency = 6;
	l2->SetLatency(ll);

	ll.bus_latency = 1;
	ll.hit_latency = 3;
	l1->SetLatency(ll);
}

MemoryMonitor::~MemoryMonitor()
{
	printf("Memory Over");	
	delete l1, l2, llc;
	delete memory;
}

void* 
MemoryMonitor::Load(lint ad, int length)
{
	char* loadcontent=new char[length];
	//memcpy(loadcontent,simumem+ad,length);
	int hit;
	int time;
	l1->HandleRequest(ad, length, 1, loadcontent, hit, time);
	return loadcontent;
}

void
MemoryMonitor::Store(lint ad,int length, char*content)
{
	//memcpy(simumem+ad,content,length);
	int hit;
	int time;
	l1->HandleRequest(ad, length, 0, content, hit, time);
	return;
}

/*
void * 
Memory::Translate(lint ad)
{
	//return (void*)(simumem+ad);
	return 0;
}
*/

//#endif

/*
int main(int argc,char** argv)
{
	Memory* MyMemory=new Memory(argv[1]);
	printf("Entry Point:%x\n",MyMemory->entry);
	return 0;
}
*/