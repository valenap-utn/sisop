#ifndef KERNEL_CORTOPLAZO_
#define KERNEL_CORTOPLAZO_

#include <kernel.h>
#include <kernel_utilities.h>
#include <pcb.h>

void *cortoPlazo(void *args);

void cortoPlazoFifo(void);

void enviar_a_cpu_dispatch(PCB *pcb);

#endif
