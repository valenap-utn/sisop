#include <cortoplazo.h>

extern enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;
extern t_log *logger;

extern list_struct_t *lista_procesos_ready;

extern int socket_dispatch_cpu;

void *cortoPlazo (void *args) {

    switch (algoritmo_cortoPlazo) {
            case FIFO_CP:
                cortoPlazoFifo();
                break;

            default:
                log_error(logger, "ERROR: algoritmo no reconocido");
                break;
        }

    return (void *)EXIT_SUCCESS;

}

void cortoPlazoFifo(void) {

    while (true) {
        sem_wait(lista_procesos_ready->sem);

        pthread_mutex_lock(lista_procesos_ready->mutex);
        PCB *pcb = list_remove(lista_procesos_ready->lista, 0);
        pthread_mutex_unlock(lista_procesos_ready->mutex);

        log_info(logger, "## (%d) - Planificado por FIFO", pcb->pid);

        enviar_a_cpu_dispatch(pcb); // TO DO
    }

}

void enviar_a_cpu_dispatch(PCB *pcb) {

    t_paquete *paquete = crear_paquete(DISPATCH__CPU);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pc), sizeof(int));

    enviar_paquete(paquete, socket_dispatch_cpu);

    log_info(logger, "## (%d) - Enviado a CPU con PC=%d", pcb->pid, pcb->pc);

    eliminar_paquete(paquete);
}

/* TP ANTERIOR

void enviar_a_cpu_dispatch(int tid, int pid)
{   sem_wait(sem_estado_conexion_cpu_dispatch);
    t_paquete * send_handshake = crear_paquete(INFO_HILO);
    agregar_a_paquete(send_handshake, &tid, sizeof(tid)); 
    agregar_a_paquete(send_handshake, &pid, sizeof(pid)); 
    enviar_paquete(send_handshake, conexion_kernel_cpu_dispatch); 
    eliminar_paquete(send_handshake);
    //Se espera la respuesta, primero el tid y luego el motivo
    sem_post(sem_estado_conexion_cpu_dispatch);
}

*/