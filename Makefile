CC = g++
PROGRAM = riscv

O_FILES = main.o machine.o register_file.o memory_monitor.o instruction.o syscall.o

$(PROGRAM) : $(O_FILES)
	$(CC) -o $(PROGRAM) $(O_FILES)

main.o : main.cc machine.h
	$(CC) -c main.cc

machine.o : machine.cc machine.h alias.h param.h syscall.h
	$(CC) -c machine.cc

register_file.o : register_file.cc register_file.h
	$(CC) -c register_file.cc

memory_monitor.o : memory_monitor.cc memory_monitor.h
	$(CC) -c memory_monitor.cc

instruction.o : instruction.cc instruction.h
	$(CC) -c instruction.cc

syscall.o : syscall.cc syscall.h
	$(CC) -c syscall.cc

clean : 
	rm $(PROGRAM) $(O_FILES)