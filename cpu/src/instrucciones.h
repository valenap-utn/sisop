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

typedef struct TLB_t
{
    int pid; //o proceso ? (para saber a quién pertenece)
    int pagina;
    int marco;
    uint64_t timestamp; //para LRU
}
TLB_t;

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
    GOTO
}intrucciones_t ;


void noop();
void write_(int dir_logica , int datos);
void read_(int dir_logica , int tamanio);
void goto_(int nuevo_pc);
void noop();
void io();
void init_proc();
void dump_memory();
void exit_();

char * Fetch();
instruccion_t Decode(char * instr);
void Execute(instruccion_t);
void Check_Int();
int instrStringMap(char []);
int MMU(int dir_logica);
int TLB(int Direccion);
int Cache_paginas(int Direccion);

void * ciclo_instruccion(void *);
void Actualizar_pc();

#endif