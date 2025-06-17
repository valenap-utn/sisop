#include <largoplazo.h>

extern enum_algoritmo_largoPlazo algoritmo_largoPlazo;
extern t_log *logger;

extern list_struct_t *lista_procesos_new;
extern sem_t *sem_proceso_fin;

void *largoPlazo(void *args){
    switch(algoritmo_largoPlazo){
        case LPL_FIFO:
            largoPlazoFifo();
            break;

        //Agregar aca mientras se van haciendo
        
        default:
            log_error(logger, "algoritmo no reconocido en Largo Plazo");

    }
    return (void *)EXIT_SUCCESS;
}

void largoPlazoFifo(){
    while(true){
        sem_wait(lista_procesos_new->sem); // espera a que haya un nuevo elemento en la cola_new
        log_debug(logger, "Nuevo proceso a crear: Intentando cargar en memoria");
        PCB * pcb = desencolar_cola_new(0);
        if(!encolarPeticionLargoPlazo(pcb)){
            encolar_cola_new(pcb);
            sem_wait(sem_proceso_fin); //espera a que un proceso finalize para volver a intentar enviar peticiones a memoria
        }
    }
    return;
}