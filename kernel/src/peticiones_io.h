#ifndef KERNEL_PETICIONES_IO_
#define KERNEL_PETICIONES_IO_

#include <kernel_utilities.h>

typedef struct 
{
    PCB *pcb;
    int tiempo;
    
}elemento_cola_blocked_io;

void *server_mh_io(void *args);
void *thread_io(void *args);

t_socket_io *inicializarSocketIO();

void liberar_socket_io(t_socket_io *socket);

int buscar_io(char *nombre_a_buscar);

t_socket_io *get_socket_io(int index);

elemento_cola_blocked_io *desencolar_cola_blocked(list_struct_t *cola);

void encolar_cola_blocked(list_struct_t *cola, elemento_cola_blocked_io *elem);

#endif
