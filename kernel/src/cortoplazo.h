#ifndef KERNEL_CORTOPLAZO_
#define KERNEL_CORTOPLAZO_

#include "kernel.h"
#include <kernel_utilities.h>
#include <pcb.h>
#include <commons/collections/dictionary.h>

void *cortoPlazo(void *args);

void cortoPlazoFifo(t_socket_cpu *socket_cpu);
void cortoPlazoSJF(t_socket_cpu *socket_cpu);
void cortoPlazoSJFConDesalojo(t_socket_cpu *socket_cpu);

void *waiter_devoluciones_cpu(void *args);

void enviar_interrupcion(t_socket_cpu *socket_cpu, int pid, int pc);

t_socket_cpu *buscar_cpu_libre();

void enviar_a_cpu_dispatch(PCB *pcb, t_socket_cpu *socket_cpu);

void esperar_respuesta_cpu(PCB *pcb, t_socket_cpu *socket_cpu);

void esperar_respuesta_cpu_sjf(PCB *pcb, t_socket_cpu *socket_cpu);

void esperar_respuesta_cp_desalojo(PCB *pcb, t_socket_cpu *socket_cpu);

void manejo_respuesta_desalojo(t_socket_cpu *socket_cpu);

#endif
