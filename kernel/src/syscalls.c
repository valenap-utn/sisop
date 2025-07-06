#include <syscalls.h>

extern list_struct_t *lista_procesos_new;

void PROCESS_CREATE(char *path, int tam_proceso) {
    

    PCB* nuevo_pcb = iniciar_pcb();
    nuevo_pcb->memoria_necesaria = tam_proceso;

    log_info(logger, "## (%d) Se crea el Proceso - Estado: NEW", nuevo_pcb->pid);

    nuevo_pcb->path_instrucciones = path;
    
    encolar_cola_new(nuevo_pcb, -1);
    
    sem_post(lista_procesos_new->sem);

}