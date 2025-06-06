#include <cortoplazo.h>

extern enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;
extern t_log *logger;

extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_procesos_exec;
extern list_struct_t *lista_sockets_cpu_libres;
extern list_struct_t *lista_sockets_cpu_ocupados;
extern list_struct_t *lista_sockets_io;

void *cortoPlazo (void *args) {

    switch (algoritmo_cortoPlazo) {
            case CPL_FIFO:
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

        sem_wait(lista_sockets_cpu_libres->sem);
        pthread_mutex_lock(lista_sockets_cpu_libres->mutex);
        t_socket_cpu * socket_cpu = list_remove(lista_sockets_cpu_libres, 0);
        pthread_mutex_unlock(lista_sockets_cpu_libres->mutex);
        
        enviar_a_cpu_dispatch(pcb, socket_cpu);
    }

}

void enviar_a_cpu_dispatch(PCB *pcb, t_socket_cpu *socket_cpu) {

    t_paquete *paquete = crear_paquete(DISPATCH__CPU);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));

    

    enviar_paquete(paquete, socket_cpu->dispatch);

    log_info(logger, "## (%d) - Enviado a CPU con PC=%d", pcb->pid, pcb->pc);

    eliminar_paquete(paquete);

    pthread_mutex_lock(lista_procesos_exec->mutex);
    list_add(lista_procesos_exec->lista, pcb);
    pthread_mutex_unlock(lista_procesos_exec->mutex);

    pthread_mutex_lock(lista_sockets_cpu_ocupados->mutex);
    list_add(lista_sockets_cpu_ocupados->lista, socket_cpu);
    pthread_mutex_unlock(lista_sockets_cpu_ocupados->mutex);
    sem_post(lista_sockets_cpu_ocupados->sem);
    
    // esperar_respuesta_cpu() ver discord, tenemos que agregar un par de cosas aca

}
t_socket_cpu *buscar_cpu_libre(){
    t_socket_cpu * socket=NULL;
    pthread_mutex_lock(lista_sockets_cpu->mutex);
    t_list_iterator iterator = list_iterator_create(lista_sockets_cpu->lista);
    while(list_iterator_has_next(iterator)){
        socket = list_iterator_next(iterator);
        if (socket->flag_libre){
            break;
        }
    }
    pthread_mutex_unlock(lista_sockets_cpu->mutex);

    if (socket == NULL){
        log_error(logger, "corto plazo: no hay CPU libres");
        return socket;
    }
    return socket;

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