#ifndef KERNEL_SYSCALLS_
#define KERNEL_SYSCALLS_

#include <kernel_utilities.h>
#include <pcb.h>
#include <peticiones_io.h>

void PROCESS_CREATE(char *instrucciones_path, int tam_proceso);

void PROCESS_EXIT(PCB *pcb);

void *dump_mem_waiter(void *args);

#endif
