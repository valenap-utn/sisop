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


/* ------ TLB ------ */
typedef struct TLB_t
{
    int pid; //o proceso ? (para saber a quién pertenece)
    int pagina;
    int marco;
    bool ocupado;
    uint64_t timestamp; //para LRU
}
TLB_t;

/* ------ CACHÉ ------ */
typedef struct cache_t{
    int pid;
    int dir_fisica;
    int contenido;
    int ocupado;
    int uso;
    int modificado;
}cache_t;





void noop();
void write_(int dir_logica , int datos);
void read_(int dir_logica , int tamanio);
void goto_(int nuevo_pc);
void noop();
void io(char * Dispositivo, int tiempo);
void init_proc(char * Dispositivo, int tamanno);
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
int buscar_en_tlb(int pid_actual, int pagina_buscada);
int get_timestamp();
void agregar_a_tlb(int pid_actual, int pagina, int marco);

int buscar_victima_LRU();
int buscar_victima_FIFO();

void limpiar_entradas_tlb(int pid_a_eliminar);

//CACHÉ
int buscar_en_cache(int pid_actual, int dir_fisica, int* contenido_out);
void escribir_en_cache(int pid_actual, int dir_fisica, int nuevo_valor);
int reemplazo_clock();
int reemplazo_clock_M();
int avanzar_puntero(int index);
void escribir_cache_en_memoria(cache_t entrada);
void limpiar_cache_de_proceso(int pid_a_eliminar);


#endif