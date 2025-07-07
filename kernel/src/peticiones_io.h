#ifndef KERNEL_PETICIONES_IO_
#define KERNEL_PETICIONES_IO_

#include <kernel_utilities.h>

void *server_mh_io(void *args);
void *administrador_peticiones_io(void *args);
void *peticion_io(void *args);
void encolar_peticion_io(t_peticion_io *peticion, int index);
t_peticion_io *desencolar_peticion_io();

#endif
