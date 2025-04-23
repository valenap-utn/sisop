#ifndef CPU_UTILITIES_
#define CPU_UTILITIES_

#include <cpu.h>

void inicializarCpu();
void levantarConfig();
void *conexion_cliente_kernel(void *args);
void *conexion_cliente_memoria(void *args);

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