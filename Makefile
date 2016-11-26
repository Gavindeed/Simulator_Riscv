CC = g++
PROGRAM = riscv

O_FILES = machine.o register_file.o memory_monitor.o instruction.o syscall.o memory.o cache.o

$(PROGRAM) : $(O_FILES)
	$(CC) -o $(PROGRAM) main.o $(O_FILES)

main.o : main.cc machine.h
	$(CC) -c main.cc

machine.o : machine.cc machine.h alias.h param.h syscall.h
	$(CC) -c machine.cc

register_file.o : register_file.cc register_file.h
	$(CC) -c register_file.cc

memory.o : memory.cc memory.h storage.h
	$(CC) -c memory.cc

cache.o : cache.cc cache.h storage.h
	$(CC) -c cache.cc

memory_monitor.o : memory_monitor.cc memory_monitor.h
	$(CC) -c memory_monitor.cc

instruction.o : instruction.cc instruction.h
	$(CC) -c instruction.cc

syscall.o : syscall.cc syscall.h
	$(CC) -c syscall.cc

lab3-main.o : lab3-main.cc cache.h memory.h param.h
	$(CC) -c lab3-main.cc

clean : 
	rm $(PROGRAM) main.o $(O_FILES)

lab3 :
	$(CC) -o lab3 lab3-main memory.o cache.o