#include <syscalls.h>

extern list_struct_t *lista_procesos_new;
extern sem_t *sem_proceso_fin;

void PROCESS_CREATE(char *path, int tam_proceso) {
    

    PCB* nuevo_pcb = iniciar_pcb();
    nuevo_pcb->memoria_necesaria = tam_proceso;

    log_info(logger, "## (%d) Se crea el Proceso - Estado: NEW", nuevo_pcb->pid);

    nuevo_pcb->path_instrucciones = path;
    
    encolar_cola_new(nuevo_pcb, -1);

    sem_post(lista_procesos_new->sem);

}
void PROCESS_EXIT(PCB *pcb) {
    
    // encolar peticion
    t_peticion_memoria * peticion = inicializarPeticionMemoria();

    peticion->tipo = PROCESS_EXIT_MEM;
    peticion->proceso = pcb;
    encolarPeticionMemoria(peticion);
    //espero a que termine (siempre deberia ser success=true)
    sem_wait(peticion->peticion_finalizada);

    if (peticion->respuesta_exitosa){
        log_debug(logger, "Se elimino un proceso de memoria");
    }
    else{
        log_error(logger, "No se pudo eliminar el proceso de memoria");
        return;
    }    

    // el proceso en teoria deberia ya estar fuera de la cola ready, entonces podemos eliminarlo
    //primero cambio el estado para que refleje en las metricas
    cambiar_estado(pcb, EXIT);

    //logueo
    loguear_metricas(pcb);

    // mando pal lobby al pcb
    pcb_destroy(pcb);

    //post fin_proceso para que largo plazo pueda intentar de nuevo
    sem_post(sem_proceso_fin);
}