#ifndef CPU_INSTRUCCIONES_
#define CPU_INSTRUCCIONES_

#include "cpu.h"
#include <cpu_utilities.h>
#include <utils/utils.h>

void noop();
void write(uint32_t* direccion, uint32_t* datos);
void read(uint32_t* direccion, int tamanio);
void goto_(int valor);


#endif