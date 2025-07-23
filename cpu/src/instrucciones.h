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
    int nro_pagina;
    char * contenido;
    int ocupado;
    int uso;
    int modificado;
}cache_t;





void noop();
void write_(int dir_logica, char *datos);
void read_(int dir_logica , int tamanio);
void goto_(int nuevo_pc);
void noop();
void io(char * Dispositivo, int tiempo);
void init_proc(char * Dispositivo, int tamanno);
void dump_memory();
void exit_();

char * Fetch();
instruccion_t *Decode(char * instr);
void Execute(instruccion_t *);
void Check_Int();
int instrStringMap(char[]);
int Cache_paginas(int Direccion);

void traducir_DL(int dir_logica, int *nro_pagina, int *offset);

int obtener_DF(int marco, int offset);

void * ciclo_instruccion(void *);
void Actualizar_pc();

//TLB
int buscar_en_tlb(int pid_actual, int pagina_buscada);
int get_timestamp();
void agregar_a_tlb(int pid_actual, int pagina, int marco);

void traducir_DL(int dir_logica, int* nro_pagina, int* offset);
int obtener_DF(int marco, int offset);

int obtener_marco(int pid, int nro_pagina,int offset);
int buscar_victima_LRU();
int buscar_victima_FIFO();

void limpiar_entradas_tlb(int pid_a_eliminar);

//CACHÉ
int buscar_en_cache(int pid_actual, int nro_pagina, char **contenido_out);
void escribir_en_cache(int pid_actual, char *nuevo_valor, int nro_pagina, int fue_escritura);
int reemplazo_clock();
int reemplazo_clock_M();
int avanzar_puntero(int index);
void escribir_en_memoria(cache_t entrada);
void limpiar_cache_de_proceso(int pid_a_eliminar);

void dump_estado_cache();

#endif