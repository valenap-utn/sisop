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
    list_struct_t* me;
    list_struct_t* mt;
    t_estado estado;
}PCB;

//FUNCIONES
PCB* iniciar_pcb();
int generar_pid_unico();

void inicializarRegistros(registrosPCB *reg);

#endif