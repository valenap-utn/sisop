#include <pcb.h>
#include <utils/utils.h>
#include <commons/log.h>
#include <commons/config.h>

//variables globales

//variables globales

//A chequear...
int pid = 0;
int pc = 0;

PCB* iniciar_pcb(){
    PCB* pcb = malloc(sizeof(PCB));
    pcb->pid = pid++;
    pcb->pc = pc++;
    pcb->me = inicializarLista();
    pcb->mt = inicializarLista();
    return pcb;
}


