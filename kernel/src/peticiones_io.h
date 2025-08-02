#ifndef KERNEL_PETICIONES_IO_
#define KERNEL_PETICIONES_IO_

#include <kernel_utilities.h>

typedef struct 
{
    PCB *pcb;
    int tiempo;
    char * nombre_io;
    pthread_t tid_suspend;
    
}elemento_cola_blocked_io;

typedef struct 
{
    t_socket_io * socket_io;
    elemento_cola_blocked_io * elemento_cola_blocked_io;
    
}args_thread_io;

void *manager_io(void *args);

void *server_mh_io(void *args);
void *thread_io(void *args);

t_socket_io *inicializarSocketIO();

void liberar_socket_io(t_socket_io *socket);

int buscar_io(char *nombre_a_buscar);

int verificar_si_existe_io(char *nombre_a_buscar);

t_socket_io *get_socket_io(int index);

elemento_cola_blocked_io *desencolar_cola_blocked(list_struct_t *cola);

void encolar_cola_blocked(list_struct_t *cola, elemento_cola_blocked_io *elem, int principio);

void * timer_suspend(void * args);

#endif
