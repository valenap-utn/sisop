#include <pcb.h>
#include <utils/utils.h>
#include <commons/log.h>
#include <commons/config.h>

//variables globales

//variables globales

//A chequear...
int pid = 0;
// int pc = 0; pc siempre es 0, podemos hardcodearlo en iniciar_pcb()

PCB* iniciar_pcb(){
    PCB* pcb = malloc(sizeof(PCB));
    pcb->pid = pid++;
    //a chequear -> agrego esta linea para que no de warning, para que apunte a memoria valida
    pcb->registros = malloc(sizeof(registrosPCB));
    inicializarRegistros(pcb->registros);
    pcb->base = 0;
    pcb->limite = 0;
    pcb->instrucciones = list_create();
    pcb->me = inicializarLista();
    pcb->mt = inicializarLista();
    return pcb;
}
void inicializarRegistros(registrosPCB *reg){
    reg->AX=0;
    reg->BX=0;
    reg->CX=0;
    reg->DX=0;
    reg->EX=0;
    reg->FX=0;
    reg->GX=0;
    reg->HX=0;
}

