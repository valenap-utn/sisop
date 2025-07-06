#include <cortoplazo.h>

extern enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;
extern t_log *logger;

extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_procesos_exec;
extern list_struct_t *lista_sockets_cpu_libres;
extern list_struct_t *lista_sockets_cpu_ocupados;
extern list_struct_t *lista_sockets_io;


/// @brief Hay que crear un thread por cada CPU, y tener en cuenta 
/// las zonas de mutua exclusion de las listas que se usan
/// @param args es un t_socket_cpu 
/// @return EXIT_SUCCESS / EXIT_FAILURE -> no son usados
void *cortoPlazo (void *args) {

    t_socket_cpu * socket_cpu = (t_socket_cpu *) args;

    switch (algoritmo_cortoPlazo) {
            case CPL_FIFO:
                cortoPlazoFifo(socket_cpu);
                break;

            default:
                log_error(logger, "ERROR: algoritmo no reconocido");
                break;
        }

    return (void *)EXIT_SUCCESS;

}

void cortoPlazoFifo(t_socket_cpu *socket_cpu) {

    while (true) {
        sem_wait(lista_procesos_ready->sem);

        pthread_mutex_lock(lista_procesos_ready->mutex);
        PCB *pcb = list_remove(lista_procesos_ready->lista, 0);
        pthread_mutex_unlock(lista_procesos_ready->mutex);

        log_info(logger, "## (%d) - Planificado por FIFO", pcb->pid);
        
        enviar_a_cpu_dispatch(pcb, socket_cpu->dispatch);

        esperar_respuesta_cpu(pcb->pid, socket_cpu->dispatch);

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

}

/// @brief Queda esperando la devolucion de CPU. Generalmente una syscall o un process_exit.
/// @param socket_cpu puntero a t_socket_cpu. Se va a leer el valor -> interrupt
/// @param pid 🤔🤔🤔
void esperar_respuesta_cpu(int pid, t_socket_cpu *socket_cpu){
    protocolo_socket motivo;
    
    log_info(logger, "Esperando motivo de devolucion de CPU");
    motivo = recibir_operacion(socket_cpu->interrupt);

    t_list *paquete_respuesta = recibir_paquete(socket_cpu->interrupt);

    switch (motivo) {

        default:
            log_info(logger, "Motivo: %d desconocido para el pid %d\n", motivo, pid);
            break;
    }
}