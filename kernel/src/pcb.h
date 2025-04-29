#ifndef PCB_H_
#define PCB_H_

#include <utils/utils.h>

typedef enum  
{
    NEW,
    READY,
    EXEC,
    BLOCK,
    SUP_BLOCK,
    SUP_READY,
    EXIT
}t_estado;

typedef struct{
    int pid;
    int pc;
    t_list* me;
    t_list* mt;    
}PCB;

//FUNCIONES
PCB* iniciar_pcb();

#endif