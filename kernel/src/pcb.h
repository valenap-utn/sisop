#ifndef PCB_H_
#define PCB_H_

#include <utils/utils.h>

typedef struct{
    int pid;
    registrosPCB *registros;
    int memoria_necesaria;
    t_estado estado;
    uint32_t base;
    uint32_t limite;
    t_list* me;
    t_list* mt;
}PCB;

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

//FUNCIONES
PCB* iniciar_pcb();

#endif