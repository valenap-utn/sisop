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
    t_list* me; //metricas de estado
    t_list* mt; //metricas de tiempo
    t_list* instrucciones;
}PCB;

//FUNCIONES
PCB* iniciar_pcb();

#endif