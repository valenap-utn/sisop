#ifndef KERNEL_PETICIONES_MEM_
#define KERNEL_PETICIONES_MEM_

#include <kernel_utilities.h>

void *administrador_peticiones_memoria(void* arg_server);
void peticion_kernel(t_args_peticion_memoria *args_peticion);
void encolarPeticionMemoria(t_peticion_memoria *peticion);

t_peticion_memoria *desencolarPeticionMemoria();

#endif
