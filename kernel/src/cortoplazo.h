#ifndef KERNEL_CORTOPLAZO_
#define KERNEL_CORTOPLAZO_

#include "kernel.h"
#include <kernel_utilities.h>
#include <pcb.h>

void *cortoPlazo(void *args);

void cortoPlazoFifo(t_socket_cpu *socket_cpu);

t_socket_cpu *buscar_cpu_libre();

void enviar_a_cpu_dispatch(PCB *pcb, t_socket_cpu *socket_cpu);

void esperar_respuesta_cpu(t_socket_cpu *socket_cpu);

#endif
