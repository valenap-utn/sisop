#ifndef KERNEL_PETICIONES_IO_
#define KERNEL_PETICIONES_IO_

#include <kernel_utilities.h>

void *server_mh_io(void *args);
void *administrador_peticiones_io(void *args);
void *peticion_io(void *args);
void encolar_peticion_io(t_peticion_io *peticion, int index);
t_peticion_io *desencolar_peticion_io();

t_socket_io *inicializarSocketIO(char *nombre);

void liberar_socket_io(t_socket_io *socket);

int buscar_io(char *nombre_a_buscar);

t_socket_io *get_socket_io(int index);

elemento_cola_blocked_io *desencolar_cola_blocked(list_struct_t *cola);

#endif
