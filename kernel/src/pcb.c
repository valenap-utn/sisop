#include <pcb.h>
#include <utils/utils.h>
#include <commons/log.h>
#include <commons/config.h>

//variables globales

//variables globales

int main2(int argc, char* argv[]) {

    return 0;
}

//A chequear...
int pid = 0;
int pc = 0;

PCB* iniciar_pcb(){
    PCB* pcb = malloc(sizeof(PCB));
    pcb->pid = pid++;
    pcb->pc = pc++;
    pcb->me = list_create();
    pcb->mt = list_create();
    return pcb;
}


