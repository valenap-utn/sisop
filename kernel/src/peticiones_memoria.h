#ifndef KERNEL_PETICIONES_
#define KERNEL_PETICIONES_

#include <kernel_utilities.h>

void *administrador_peticiones_memoria(void* arg_server);
void *peticion_kernel(void *args);
void encolarPeticionMemoria(t_peticion_memoria *peticion);


#endif
