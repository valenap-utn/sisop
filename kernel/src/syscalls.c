#include <syscalls.h>

extern list_struct_t *lista_procesos_new;
extern list_struct_t *lista_procesos_block;
extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_sockets_io;

extern enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;

extern sem_t *sem_memoria_liberada;

void PROCESS_CREATE(char *path, int tam_proceso) {
    

    PCB* nuevo_pcb = iniciar_pcb();
    nuevo_pcb->memoria_necesaria = tam_proceso;

    log_info(logger, "## (%d) Se crea el Proceso - Estado: NEW", nuevo_pcb->pid);

    nuevo_pcb->path_instrucciones = malloc(strlen(path)+1);
    strcpy(nuevo_pcb->path_instrucciones, path);
    
    encolar_cola_generico(lista_procesos_new, nuevo_pcb, -1);

}
void PROCESS_EXIT(PCB *pcb) {
    
    // encolar peticion
    t_peticion_memoria * peticion = inicializarPeticionMemoria();
    log_info(logger, "## (pid: %d) - Finaliza el proceso", pcb->pid);

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
    liberar_peticion_memoria(peticion);

    // el proceso en teoria deberia ya estar fuera de la cola ready, entonces podemos eliminarlo
    //primero cambio el estado para que refleje en las metricas
    cambiar_estado(pcb, EXIT);

    //logueo
    loguear_metricas(pcb);

    // mando pal lobby al pcb
    pcb_destroy(pcb);

    //post memoria_liberada para destrabar largo y mediano plazo
    sem_post(sem_memoria_liberada);
}

void DUMP_MEMORY(PCB *pcb) {
    
    // encolar peticion
    t_peticion_memoria * peticion = inicializarPeticionMemoria();

    peticion->tipo = DUMP_MEM;
    peticion->proceso = pcb;
    encolarPeticionMemoria(peticion);
    

    //se bloquea el proceso automaticamente
    //encolar_cola_generico(lista_procesos_block, pcb, 0);
    //no encolamos para no ser victimas de mediano plazo
    cambiar_estado(pcb, BLOCK);

    // me creo un thread temporal que espera la respuesta, y devuelve el proceso a ready
    // o lo manda a exit
    pthread_t tid_aux;
    pthread_create(&tid_aux, NULL, dump_mem_waiter, (void*)peticion);

    //me desentiendo del thread, porque se que eventualmente va a terminar.
    pthread_detach(tid_aux);

    //termino la funcion para que el planificador siga su ciclo con el proceso siguiente
    return;
}
void * dump_mem_waiter(void *args){
    t_peticion_memoria * peticion = args;
    
    sem_wait(peticion->peticion_finalizada);

    if (peticion->respuesta_exitosa){        
        encolar_cola_generico(lista_procesos_ready, peticion->proceso, -1);
        cambiar_estado(peticion->proceso, READY);
        log_debug(logger, "Dump completado, finalizando thread auxiliar y enviando proceso %d a ready", peticion->proceso->pid);
    }else{
        PROCESS_EXIT(peticion->proceso);
        log_debug(logger, "El DUMP fallo, finalizando proceso %d", peticion->proceso->pid);
    }
    liberar_peticion_memoria(peticion);

    return (void*) EXIT_SUCCESS;
}

void IO_syscall(PCB *pcb, char * nombre_io, int tiempo) {
    
    elemento_cola_blocked_io * elem_blocked_io = malloc(sizeof(elemento_cola_blocked_io));

    elem_blocked_io->pcb = pcb;
    elem_blocked_io->tiempo = tiempo;

    int index = buscar_io(nombre_io);

    if(index == -1){
        PROCESS_EXIT(pcb);
        if(algoritmo_cortoPlazo == CPL_SJF_CD){sem_post(lista_procesos_ready->sem);}
        return;
    }

    t_socket_io *socket_io = get_socket_io(index);

    encolar_cola_blocked(socket_io->cola_blocked, elem_blocked_io);

    encolar_cola_generico(lista_procesos_block, pcb, -1);
    cambiar_estado(pcb, BLOCK);
    if(algoritmo_cortoPlazo == CPL_SJF_CD){sem_post(lista_procesos_ready->sem);}
    
    return;
}