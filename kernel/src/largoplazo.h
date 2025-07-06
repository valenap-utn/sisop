#ifndef KERNEL_LARGOPLAZO_
#define KERNEL_LARGOPLAZO_

#include <kernel.h>
#include <kernel_utilities.h>
#include <pcb.h>



void * largoPlazo(void * args);

void largoPlazoFifo();

void largoPlazoSmallFirst();
void *largoPlazoFallidos(void *args);

#endif


