#ifndef PCB_H_
#define PCB_H_

#include <utils/utils.h>



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
    uint32_t PC;
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
    registrosPCB *registros;
    int memoria_necesaria;
    uint32_t base;
    uint32_t limite;
    list_struct_t* me; //metricas de estado
    list_struct_t* mt; //metricas de tiempo
    t_list* instrucciones;
    int cant_instrucciones;
}PCB;

//FUNCIONES
PCB* iniciar_pcb();

void inicializarRegistros(registrosPCB *reg);

#endif