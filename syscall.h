#define SYS_write 64
#define SYS_read 63
#define SYS_gettimeofday 169
#define SYS_exit 93

#include "memory_monitor.h"

extern void syscall(lint reg10, lint reg11, lint reg12, lint reg13, lint reg17, Memory *memory);