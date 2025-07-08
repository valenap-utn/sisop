#ifndef CPU_UTILITIES_
#define CPU_UTILITIES_

#include <cpu.h>

void inicializarCpu(char *);
void levantarConfig();
void *conexion_cliente_kernel(void *args);
void *conexion_cliente_memoria(void *args);


#endif