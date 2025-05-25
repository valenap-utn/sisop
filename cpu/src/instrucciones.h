#ifndef CPU_INSTRUCCIONES_
#define CPU_INSTRUCCIONES_

#include "cpu.h"
#include <cpu_utilities.h>
#include <utils/utils.h>
#include <string.h>


typedef enum {
    SYSCALL,
    USUARIO,
}priv_instrucion_t ;

typedef struct
{
    int opCode;
    char ** data;
    priv_instrucion_t tipo;
}
instruccion_t;

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


void noop();
void write_(uint32_t* , uint32_t* );
void read_(uint32_t* , int );
void goto_(int valor);
void noop();
void io();
void init_proc();
void dump_memory();
void exit_();

char * Fetch();
instruccion_t Decode(char *);
void Execute(instruccion_t);
void Check_Int();
int instrStringMap(char []);
void MMU(uint32_t*direccion);
void * ciclo_instruccion(void *);

#endif