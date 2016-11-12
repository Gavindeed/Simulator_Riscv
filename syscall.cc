#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include "syscall.h"
#include "memory_monitor.h"
//need include h file of load and store

void sys_exit(){
	printf("the program has exited successfully\n");
	exit(0);
}

void sys_write(lint reg10, lint reg11, lint reg12, Memory *memory){
	printf("sys_write\n");

	// we don't have fd
	if (reg10 != 1){
		printf("error!!! not stdout!!!\n");
		exit(0);
	}
	else{
		// need load function from Jingyue Gao
		void* location = memory->Load(reg11, (int)reg12); 
		write((int)reg10, location, 1);
		//write((int)reg10, location, (int)reg12);
	}
}

void sys_read(lint reg10, lint reg11, lint reg12, Memory *memory)
{
	printf("sys_read\n");

	// we don't have fd
	if (reg10 != 0)
	{
		printf("error!!! not stdin!!!\n");
		exit(0);
	}
	else{
		void* readposition = (void*)malloc((int)reg12);
		read((int)reg10, readposition, 1);
		//read((int)reg10, readposition, (int)reg12);
		// need store function from Jingyue Gao
		memory->Store(reg11, (int)reg12, (char*)readposition);
		delete readposition;
	}
	
}

void sys_gettimeofday(lint reg10, Memory *memory){
	printf("sys_gettimeofday\n");
	struct timeval t;
	if (gettimeofday(&t, NULL) == 0){
		memory->Store(reg10, sizeof(t), (char*)&t);
	}
	else{
		printf("gettimeofday error\n");
		exit(0);
	}
}


void syscall(lint reg10, lint reg11, lint reg12, lint reg13, lint reg17, Memory *memory, RegisterFile *file)
{
	switch(reg17){
	case SYS_exit:
		sys_exit();
		break;
	case SYS_read:
		sys_read(reg10, reg11, reg12, memory);
		break;
	case SYS_write: 
		sys_write(reg10, reg11, reg12, memory);
		break;
	case SYS_gettimeofday: 
		sys_gettimeofday(reg10, memory);
		break;
	case SYS_close:
		printf("sys_close, we just ignore it\n");
		file->setInteger(A0, 1);
		break;
	case SYS_fstat:
		printf("sys_fstat, we just ignore it\n");
		file->setInteger(A0, 1);
		break;
	case SYS_brk:
		printf("sys_brk, we just ignore it\n");
		file->setInteger(A0, 1);
		break;
	default:
		printf("%d syscall hasn't been defined\n", reg17);
	}
}