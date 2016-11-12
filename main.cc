#include "machine.h"
//#include "system.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		printf("No Input File!\n");
		return 1;
	}
	Machine *machine = new Machine(argv[1]);
	//machine->SetVerbose();
	//machine->SetDebug();
	machine->Run();
	printf("Run over!\n");
	return 0;
} 