#ifndef CPU_UTILITIES_
#define CPU_UTILITIES_

#include <cpu.h>

typedef enum {
    // Syscalls
    IO,
    INIT_PROC,
    DUMP_MEMORY,
    EXIT_I,
    // No Syscalls
    NOOP,
    WRITE_I,
    READ_I,
    GOTO,
    DISPATCH_CPU_I,
}instrucciones_t ;


typedef struct interrupcion_t
{
    instrucciones_t tipo; //o proceso ? (para saber a quién pertenece)
    int param1;
    int param2;
    char * paramstring;
    int pid;
    int pc;
}
interrupcion_t;

void inicializarCpu(char *);
void levantarConfig();
void *conexion_cliente_kernel(void *args);
void *conexion_kernel_dispatch(void *arg_kernelD);
void *conexion_cliente_memoria(void *args);

interrupcion_t *desencolar_interrupcion_generico(list_struct_t *cola);

void vaciar_cola_interrupcion(list_struct_t *cola);

void encolar_interrupcion_generico(list_struct_t *cola, interrupcion_t *interrupcion, int index);

#endif