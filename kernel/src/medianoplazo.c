#include <medianoplazo.h>

extern list_struct_t *lista_procesos_block;
extern list_struct_t *lista_procesos_susp_ready;
extern list_struct_t *lista_procesos_susp_block;
extern list_struct_t *lista_procesos_ready;

extern sem_t *sem_memoria_liberada;

//condiciones globales
extern pthread_cond_t * cond_susp_ready_empty;
extern int susp_ready_empty;
extern pthread_mutex_t * mutex_susp_ready_empty;


void * medianoplazo(void * args){
      
    PCB * pcb;
    //proceso llega a cola susp_ready
    while(true){
        sem_wait(lista_procesos_susp_ready->sem);
        
        pcb = desencolar_generico(lista_procesos_susp_ready, 0);
        
        if(!encolar_peticion_medianoPlazo(pcb)){
            encolar_cola_generico(lista_procesos_susp_ready, pcb, 0);
            sem_wait(sem_memoria_liberada);
        }
        pthread_mutex_lock(lista_procesos_susp_ready->mutex);
        if(list_is_empty(lista_procesos_susp_ready->lista)){
            destrabar_flag_global(&susp_ready_empty, mutex_susp_ready_empty, cond_susp_ready_empty);
        }
        pthread_mutex_unlock(lista_procesos_susp_ready->mutex);
        
    }



    //se envia peticion a memoria
    //si falla, vuelve a la lista susp_ready
    

}
bool encolar_peticion_medianoPlazo(PCB *pcb){

    t_peticion_memoria * peticion = inicializarPeticionMemoria();

    peticion->tipo = UNSUSPEND_MEM;
    peticion->proceso = pcb;
    encolarPeticionMemoria(peticion);
    sem_wait(peticion->peticion_finalizada);
    if (peticion->respuesta_exitosa){
        log_debug(logger, "Se des suspendio un proceso %d de memoria", pcb->pid);
        encolar_cola_generico(lista_procesos_ready, pcb, -1);
        cambiar_estado(pcb, READY);
        //sem post a proceso nuevo encolado, revisar si hace falta
        return true;
    }
    else{
        log_debug(logger, "No se pudo des suspender un proceso %d en memoria", pcb->pid);
        return false;
    }

}