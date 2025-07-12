#ifndef CPU_UTILITIES_
#define CPU_UTILITIES_

#include <cpu.h>

void inicializarCpu(char *);
void levantarConfig();
void *conexion_cliente_kernel(void *args);
void *conexion_cliente_memoria(void *args);

void encolar_interrupcion_generico(list_struct_t *cola, interrupcion_t *interrupcion);

#endif