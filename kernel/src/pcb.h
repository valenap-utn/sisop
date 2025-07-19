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
    int pid;
    int pc;
    int memoria_necesaria;
    uint32_t base;
    uint32_t limite;
    char* path_instrucciones;
    int me[7];
    long mt[7];
    t_estado estado;
    struct timespec timestamp_ultimo_estado;
    struct timespec timestamp_ultimo_exec;
    double estimacion_rafaga;
    double rafaga_real_anterior;
}PCB;


//FUNCIONES
PCB* iniciar_pcb();
int generar_pid_unico();

void cambiar_estado(PCB *pcb, t_estado estadoNuevo);

void pcb_destroy(PCB *pcb);

void loguear_metricas(PCB *pcb);

long diff_in_milliseconds(struct timespec start, struct timespec end);

void actualizar_estimacion(PCB *pcb); // SJF: nuevo valor

#endif