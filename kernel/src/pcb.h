#ifndef PCB_H_
#define PCB_H_

#include <utils/utils.h>
#include <utils/utils.h>
#include <commons/log.h>
#include <commons/config.h>
#include <time.h>



enum t_estado
{
    NEW,
    READY,
    EXEC,
    BLOCK,
    SUP_BLOCK,
    SUP_READY,
    EXIT
};typedef enum t_estado t_estado;

typedef struct{
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
    uint32_t EX;
    uint32_t FX;
    uint32_t GX;
    uint32_t HX;
}registrosPCB;

typedef struct{
    int pid;
    int pc;
    registrosPCB *registros;
    int memoria_necesaria;
    uint32_t base;
    uint32_t limite;
    char* path_instrucciones;
    int me[7];
    long mt[7];
    t_estado estado;
    struct timespec timestamp_ultimo_estado;
}PCB;


//FUNCIONES
PCB* iniciar_pcb();
int generar_pid_unico();

void cambiar_estado(PCB *pcb, t_estado estadoNuevo);

long diff_in_milliseconds(struct timespec start, struct timespec end);

void inicializarRegistros(registrosPCB *reg);

#endif