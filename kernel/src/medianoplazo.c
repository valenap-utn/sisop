#include <medianoplazo.h>

extern list_struct_t *lista_procesos_block;
extern list_struct_t *lista_procesos_susp_ready;
extern list_struct_t *lista_procesos_susp_block;

void * medianoplazo(void * args){
    //proceso entra en blocked: timer
    //se pasa a susp_block si supera timer. Se envia peticion a memoria
    //este comportamiento esta a cargo del thread de IO. mediano plazo solo chequea si
    //algun proceso vuelve a susp_ready para pasarlo a ready eventualmente

    //luego:
    //proceso llega a cola susp_ready
    //se envia peticion a memoria
    //si falla, vuelve a la lista susp_ready
    

}