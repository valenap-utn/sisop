#include <syscalls.h>

extern list_struct_t *lista_procesos_new;

void PROCESS_CREATE(char *path, int tam_proceso) {
    

    PCB* nuevo_pcb = iniciar_pcb();
    nuevo_pcb->memoria_necesaria = tam_proceso;

    log_info(logger, "## (%d) Se crea el Proceso - Estado: NEW", nuevo_pcb->pid);

    nuevo_pcb->path_instrucciones = path;
    
    pthread_mutex_lock(lista_procesos_new->mutex);
    list_add(lista_procesos_new->lista, nuevo_pcb);
    pthread_mutex_unlock(lista_procesos_new->mutex);
    
    sem_post(lista_procesos_new->sem);

}