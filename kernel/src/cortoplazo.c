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
/// @param pid
void esperar_respuesta_cpu(int pid, t_socket_cpu *socket_cpu){
    protocolo_socket motivo;
    
    log_info(logger, "Esperando motivo de devolucion de CPU");
    motivo = recibir_operacion(socket_cpu->interrupt);

    t_list *paquete_respuesta = recibir_paquete(socket_cpu->interrupt);
    int tid = hilo_actual->tid;
    char* nombre_mutex;
    int tiempo;
    int pid = hilo_actual->pid;
    char * nombre_archivo;
    int prioridad;
    int tamanio;
    FILE * archivo;
    char * ok_recibido;
    switch (motivo) {
        case FIN_QUANTUM:
            log_info(logger, "## (%d:%d) - Desalojado por fin de Quantum\n", hilo_actual->pid, tid);
            encolar_corto_plazo_multinivel(hilo_actual);
            sem_post(sem_hilo_actual_encolado);
            ok_recibido = list_remove(paquete_respuesta, 0);
            free(ok_recibido);
            list_destroy(paquete_respuesta);
            break;

        case PROCESS_CREATE_OP:
            log_info(logger, "## (%d:%d) - Solicitó syscall: PROCESS_CREATE", hilo_actual->pid, tid);
            nombre_archivo = list_remove(paquete_respuesta, 0);
            archivo = fopen(nombre_archivo, "r");
            tamanio = * (int *)list_remove(paquete_respuesta, 0);
            prioridad = * (int *)list_remove(paquete_respuesta, 0);
            pid = proceso_actual->pid;
            PROCESS_CREATE(archivo, tamanio, prioridad);
            //sem_wait(sem_hilo_nuevo_encolado);
            encolar_hilo_corto_plazo(hilo_actual);
            sem_post(sem_hilo_actual_encolado);
            log_info(logger, "Despues de encolar el hilo nuevo del PROCESS CREATE se hace un post y comienza la ejecucion de nuevo");
            break; 

        case PROCESS_EXIT_OP:
            log_info(logger, "## (%d:%d) - Solicitó syscall: PROCESS_EXIT", hilo_actual->pid, tid);
            PROCESS_EXIT();
            sem_post(sem_hilo_actual_encolado);
            ok_recibido = list_remove(paquete_respuesta, 0);
            free(ok_recibido);
            list_destroy(paquete_respuesta);
            break;

        case THREAD_CREATE_OP:
            log_info(logger, "## (%d:%d) - Solicitó syscall: THREAD_CREATE", hilo_actual->pid, hilo_actual->tid);
            nombre_archivo = list_remove(paquete_respuesta, 0);
            archivo = fopen(nombre_archivo, "r");
            prioridad = *(int *)list_remove(paquete_respuesta, 0);
            pid = proceso_actual->pid;
            THREAD_CREATE(archivo, prioridad);
            encolar_hilo_corto_plazo(hilo_actual);
            sem_post(sem_hilo_actual_encolado);
            break;

        case THREAD_EXIT_OP:
            log_info(logger, "PID:%d TID:%d inicio un THREAD EXIT\n", hilo_actual->pid, tid);
            desbloquear_hilos(hilo_actual->tid);
            THREAD_EXIT();
            sem_post(sem_hilo_actual_encolado);
            ok_recibido = list_remove(paquete_respuesta, 0);
            free(ok_recibido);
            list_destroy(paquete_respuesta);
            break; 
        
        case THREAD_CANCEL_OP:
            log_info(logger, "PID:%d TID:%d inicio un THREAD CANCEL\n", hilo_actual->pid, tid);
            tid = *(int *)list_remove(paquete_respuesta, 0);
            desbloquear_hilos(tid);
            THREAD_CANCEL(tid);
            sem_post(sem_hilo_actual_encolado);
            break;   

        case THREAD_JOIN_OP:
            tid = *(int *)list_remove(paquete_respuesta, 0);
            log_info(logger, "## (%d:%d) - Bloqueado por: THREAD_JOIN, esperando al hilo %d\n", hilo_actual->pid, hilo_actual->tid, tid);
            // Transicionar el hilo al estado block (se hace en la syscall) y esperar a que termine el otro hilo para poder seguir ejecutando
            THREAD_JOIN(tid);
            sem_post(sem_hilo_actual_encolado);
            //encolar_hilo_corto_plazo(hilo_actual);
            //esperar_desbloqueo_ejecutar_hilo(tid); -> ya no se usá, la lógica está en finalizacion -> "desbloquear hilos"
            break;

        case MUTEX_CREATE_OP:
            nombre_mutex = list_remove(paquete_respuesta, 0);
            log_info(logger, "## (%d:%d) creo un NUEVO MUTEX: '%s'\n", hilo_actual->pid, hilo_actual->tid, nombre_mutex);
            MUTEX_CREATE(nombre_mutex);
            encolar_corto_plazo_multinivel(hilo_actual);
            sem_post(sem_hilo_actual_encolado);
            break;

        case MUTEX_LOCK_OP:
            nombre_mutex = list_remove(paquete_respuesta, 0);
            log_info(logger, "## (%d:%d) - Bloqueado por MUTEX_LOCK: '%s'\n", hilo_actual->pid, hilo_actual->tid, nombre_mutex);
            log_info(logger, "## Estado de los Mutex y sus hilos bloqueados:");
            MUTEX_LOCK(nombre_mutex);

            sem_post(sem_hilo_actual_encolado);
            break;

        case MUTEX_UNLOCK_OP:
            nombre_mutex = list_remove(paquete_respuesta, 0);
            log_info(logger, "PID:%d TID:%d está intentando LIBERAR el MUTEX: '%s'\n", hilo_actual->pid, hilo_actual->tid, nombre_mutex);
            MUTEX_UNLOCK(nombre_mutex);
            //enviar_a_cpu_dispatch(hilo_actual->pid, hilo_actual->tid);
            //sem_post(sem_hilo_actual_encolado);
            break;

        case IO_SYSCALL:
            tiempo = *(int *)list_remove(paquete_respuesta, 0);
            log_info(logger, "## (%d:%d) - Bloqueado por: IO\n", hilo_actual->pid, tid);
            IO(tiempo, hilo_actual->tid);
            sem_post(sem_hilo_actual_encolado);
            break;   

        case DUMP_MEMORY_OP:
            log_info(logger, "PID:%d TID:%d lanza un dump del proceso padre\n", hilo_actual->pid, tid);
            pid = proceso_actual->pid;
            encolar_hilo_corto_plazo(hilo_actual);
            DUMP_MEMORY(pid);
            sem_post(sem_hilo_actual_encolado);
            break;   

        case SEGMENTATION_FAULT:
            log_info(logger, "TID:%d PID:%d es finalizado por SEGMENTATION FAULT\n", hilo_actual->tid);
            PROCESS_EXIT();
            ok_recibido = list_remove(paquete_respuesta, 0);
            free(ok_recibido);
            list_destroy(paquete_respuesta);
            break;
        default:
            log_info(logger, "Motivo: %d desconocido para el hilo %d\n", motivo, tid);
            break;
    }
}