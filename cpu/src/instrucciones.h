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

typedef enum { //tipo_de_acceso
    LECTURA_AC,
    ESCRITURA_AC,
}acceso_t ;

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
    bool ocupado;
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
int Cache_paginas(int Direccion);
void recibir_valores_memoria(int socket_memoria);

void * ciclo_instruccion(void *);
void Actualizar_pc();

//TLB
int TLB(int Direccion);
int buscar_en_tlb(int pagina);
void agregar_a_tlb(int pagina, int marco);
void remplazar_TLB();
int get_timestamp();

int buscar_en_TLB();

#endif