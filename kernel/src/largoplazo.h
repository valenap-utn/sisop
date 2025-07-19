#ifndef KERNEL_LARGOPLAZO_
#define KERNEL_LARGOPLAZO_

#include <kernel.h>
#include <kernel_utilities.h>
#include <peticiones_memoria.h>
#include <pcb.h>



void * largoPlazo(void * args);

void largoPlazoFifo();

void largoPlazoSmallFirst();
void *largoPlazoFallidos(void *args);

bool encolarPeticionLargoPlazo(PCB *pcb);
void esperar_prioridad_susp_ready();

bool hay_algo_en_susp_ready();

#endif


