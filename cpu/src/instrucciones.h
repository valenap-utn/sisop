#ifndef CPU_INSTRUCCIONES_
#define CPU_INSTRUCCIONES_

#include "cpu.h"
#include <cpu_utilities.h>
#include <utils/utils.h>

void noop();
void write(uint32_t* direccion, uint32_t* datos);
void read(uint32_t* direccion, int tamanio);
void goto_(int valor);

void Feth();
void Decode();
void Execute();
void Execute();
void Check_Int();

typedef enum {
    // Syscalls
    IO,
    INIT_PROC,
    DUMP_MEMORY,
    EXIT,
    // No Syscalls
    NOOP,
    WRITE,
    READ,
    GOTO
}intrucciones_t ;

#endif