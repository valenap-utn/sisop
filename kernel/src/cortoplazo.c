#include <cortoplazo.h>

extern enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;
extern t_log *logger;

PCB * pcb_running;

extern int flag_all_start;

int flag_pcbrunning;
pthread_cond_t *cond_pcb;
pthread_mutex_t *mutex_pcb;

int flag_syscall = 0;
pthread_cond_t *cond_syscall;
pthread_mutex_t *mutex_syscall;

extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_procesos_exec;
extern list_struct_t *lista_sockets_cpu_libres;
extern list_struct_t *lista_sockets_cpu_ocupados;
extern list_struct_t *lista_sockets_io;

extern sem_t * sem_syscall;
pthread_mutex_t *perate;

t_dictionary *diccionario_cpu_pcb;

/// @brief Hay que crear un thread por cada CPU, y tener en cuenta 
/// las zonas de mutua exclusion de las listas que se usan
/// @param args es un t_socket_cpu 
/// @return EXIT_SUCCESS / EXIT_FAILURE -> no son usados
void *cortoPlazo (void *args) {

    t_socket_cpu * socket_cpu = args;

    switch (algoritmo_cortoPlazo) {
            case CPL_FIFO:
                cortoPlazoFifo(socket_cpu);
                break;
            case CPL_SJF:
                cortoPlazoSJF(socket_cpu);
                break;
            case CPL_SJF_CD:
                cortoPlazoSJFConDesalojo(socket_cpu);
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
        
        enviar_a_cpu_dispatch(pcb, socket_cpu);

        cambiar_estado(pcb, EXEC);

        esperar_respuesta_cpu(pcb, socket_cpu);

    }

}

void enviar_a_cpu_dispatch(PCB *pcb, t_socket_cpu *socket_cpu) {
    
    t_paquete *paquete = crear_paquete(DISPATCH_CPU);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pc), sizeof(int));

    

    enviar_paquete(paquete, socket_cpu->dispatch);

    recibir_paquete_ok(socket_cpu->interrupt);

    log_info(logger, "## (%d) - Enviado a CPU con PC=%d", pcb->pid, pcb->pc);

    eliminar_paquete(paquete);

    encolar_cola_generico(lista_procesos_exec, pcb, 0);
}

/// @brief Queda esperando la devolucion de CPU. Generalmente una syscall o un process_exit.
/// @param socket_cpu puntero a t_socket_cpu. Se va a leer el valor -> interrupt
/// @param pid 🤔🤔🤔
void esperar_respuesta_cpu(PCB * pcb, t_socket_cpu *socket_cpu){
    protocolo_socket motivo;
    
    log_info(logger, "Esperando motivo de devolucion de CPU");
    motivo = recibir_operacion(socket_cpu->interrupt);

    t_list *paquete_respuesta = recibir_paquete(socket_cpu->interrupt);

    switch (motivo) {

        case PROCESS_EXIT_CPU:

            log_info(logger, "## (PID: %d) - Solicitó syscall: PROCESS EXIT", pcb->pid);
            
            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);
            PROCESS_EXIT(pcb);

            break;

        case DESALOJO_CPU:
            
            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);
            pcb->pc = *(int*)list_remove(paquete_respuesta, 0);

            cambiar_estado(pcb, READY);
            encolar_cola_generico(lista_procesos_ready, pcb, -1);

            break;

        case PROCESS_INIT_CPU:
            log_info(logger, "## (PID: %d) - Solicitó syscall: PROCESS INIT", pcb->pid);

            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);
            pcb->pc = *(int*)list_remove(paquete_respuesta, 0);

            char * path = list_remove(paquete_respuesta, 0);
            int tamaño = *(int*)list_remove(paquete_respuesta, 0);
            
            // vuelve a READY - reencolado por desalojo
            cambiar_estado(pcb, READY);
            encolar_cola_generico(lista_procesos_ready, pcb, -1);

            PROCESS_CREATE(path, tamaño);

            break;

        case DUMP_MEM_CPU:

            log_info(logger, "## (PID: %d) - Solicitó syscall: DUMP MEMORY", pcb->pid);

            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);
            pcb->pc = *(int*)list_remove(paquete_respuesta, 0);

            DUMP_MEMORY(pcb);

            break;

        case IO_CPU:

            log_info(logger, "## (PID: %d) - Solicitó syscall: IO", pcb->pid);

            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);
            pcb->pc = *(int*)list_remove(paquete_respuesta, 0);

            char * nombre_io = list_remove(paquete_respuesta, 0);
            int tiempo = *(int *)list_remove(paquete_respuesta, 0);

            IO_syscall(pcb, nombre_io, tiempo);

            break;

        default:
            log_info(logger, "Motivo: %d desconocido para el pid %d\n", motivo, pcb->pid);
            list_destroy(paquete_respuesta);
            break;
    }    
    if(motivo != DESALOJO_CPU){
        destrabar_flag_global(&flag_syscall, mutex_syscall, cond_syscall);
    }
}

int buscar_indice_proceso_menor_estimacion() {
    pthread_mutex_lock(lista_procesos_ready->mutex);

    if (list_is_empty(lista_procesos_ready->lista)) {
        pthread_mutex_unlock(lista_procesos_ready->mutex);
        return -1;
    }

    t_list_iterator* iterador_ready = list_iterator_create(lista_procesos_ready->lista);

    PCB* proceso_con_menor_estimacion = list_iterator_next(iterador_ready); // primero
    int indice_menor_estimacion = 0;
    int posicion_actual = 1;

    while (list_iterator_has_next(iterador_ready)) {
        PCB* proceso_actual = list_iterator_next(iterador_ready);

        if (proceso_actual->estimacion_rafaga < proceso_con_menor_estimacion->estimacion_rafaga) {
            proceso_con_menor_estimacion = proceso_actual;
            indice_menor_estimacion = posicion_actual;
        }

        posicion_actual++;
    }

    list_iterator_destroy(iterador_ready);
    pthread_mutex_unlock(lista_procesos_ready->mutex);
    return indice_menor_estimacion;
}

void cortoPlazoSJF(t_socket_cpu *socket_cpu) {
   
    while (true) {
        
        sem_wait(lista_procesos_ready->sem);

        // menor estimación
        int indice_proceso_a_planificar = buscar_indice_proceso_menor_estimacion();

        if (indice_proceso_a_planificar == -1) continue;

        // saca proc ready
        pthread_mutex_lock(lista_procesos_ready->mutex);
        PCB *proceso_seleccionado = list_remove(lista_procesos_ready->lista, indice_proceso_a_planificar);
        pthread_mutex_unlock(lista_procesos_ready->mutex);

        log_info(logger, "## (%d) - Planificado por SJF sin desalojo", proceso_seleccionado->pid);

        // inicio
        struct timespec tiempo_inicio_rafaga;
        clock_gettime(CLOCK_MONOTONIC, &tiempo_inicio_rafaga);

        enviar_a_cpu_dispatch(proceso_seleccionado, socket_cpu);

        cambiar_estado(proceso_seleccionado, EXEC);

        esperar_respuesta_cpu(proceso_seleccionado, socket_cpu);

        // fin
        struct timespec tiempo_fin_rafaga;
        clock_gettime(CLOCK_MONOTONIC, &tiempo_fin_rafaga);

        // rafaga real
        long duracion_rafaga = diff_in_milliseconds(tiempo_inicio_rafaga, tiempo_fin_rafaga);
        proceso_seleccionado->rafaga_real_anterior = duracion_rafaga;

        actualizar_estimacion(proceso_seleccionado);
    }
}

void actualizar_estimacion(PCB *pcb) {
    
    // actualiza tiempo real de ejecucion
    // struct timespec tiempo_fin_rafaga;
    
    // // med de rafaga real
    // clock_gettime(CLOCK_MONOTONIC, &tiempo_fin_rafaga);
    // long duracion_rafaga = diff_in_milliseconds(pcb->timestamp_ultimo_estado, tiempo_fin_rafaga);
    // pcb->rafaga_real_anterior = duracion_rafaga;
    
    extern double alfa;

    double estimacion_anterior = pcb->estimacion_rafaga;
    double rafaga_real = pcb->rafaga_real_anterior;

    // saco estimacion nueva
    pcb->estimacion_rafaga = alfa * rafaga_real + (1 - alfa) * estimacion_anterior;

    log_debug(logger, "Estimacion actualizada: %s", string_itoa(pcb->estimacion_rafaga));
}

void cortoPlazoSJFConDesalojo(t_socket_cpu *socket_cpu) {
    //cree un proceso que espera respuestas y llama a syscalls

    pthread_t tid_waiter;

    perate = inicializarMutex();

    cond_pcb = inicializarCond();
    mutex_pcb = inicializarMutex();

    pthread_create(&tid_waiter, NULL, waiter_devoluciones_cpu, (void*)socket_cpu);
    pthread_detach(tid_waiter);

    PCB * pcb_ready;
    t_list * paquete_recv;
    protocolo_socket codop;
    
    while (true) {

        sem_wait(lista_procesos_ready->sem);

        // proceso con menor estimación
        int indice_ready_menor = buscar_indice_proceso_menor_estimacion();
        if (indice_ready_menor == -1) continue; // si no hay procesos en READY -> saltear esta vuelta del while

pthread_mutex_lock(lista_procesos_ready->mutex);
        PCB *pcb_ready = list_remove(lista_procesos_ready->lista, indice_ready_menor);
        pthread_mutex_unlock(lista_procesos_ready->mutex);

        // hay proceso ejecutando?
        pthread_mutex_lock(lista_procesos_exec->mutex);
        bool hay_proceso_en_exec = !list_is_empty(lista_procesos_exec->lista);
        

        if (hay_proceso_en_exec) {
            PCB *pcb_exec = list_get(lista_procesos_exec->lista, 0); // unico proc en EXEC
            pthread_mutex_unlock(lista_procesos_exec->mutex);

            // comparar estimaciones
            log_debug(logger, "%d ready: %s, %d exec: %s", pcb_ready->pid, string_itoa(pcb_ready->estimacion_rafaga), pcb_exec->pid, string_itoa(pcb_exec->estimacion_rafaga));
            if (pcb_ready->estimacion_rafaga < pcb_exec->estimacion_rafaga) {
                log_info(logger, "## (%d) - Desalojo: menor estimación que PID %d", pcb_ready->pid, pcb_exec->pid);
                enviar_interrupcion(socket_cpu, pcb_ready->pid, pcb_ready->pc);
                encolar_cola_generico(lista_procesos_exec, pcb_ready, -1);
                cambiar_estado(pcb_ready, EXEC);
            }else {
                encolar_cola_generico(lista_procesos_ready, pcb_ready, -1);
                destrabar_flag_global(&flag_syscall, mutex_syscall, cond_syscall);
            }
        } else {
            pthread_mutex_unlock(lista_procesos_exec->mutex);

            // no hay proc en EXEC entonces lo saco de ready y lo mando

            log_info(logger, "## (%d) - Planificado por SJF con desalojo", pcb_ready->pid);

            // // inicio de rafaga
            // clock_gettime(CLOCK_MONOTONIC, &pcb_ready->timestamp_ultimo_estado);

            enviar_a_cpu_dispatch(pcb_ready, socket_cpu);
            cambiar_estado(pcb_ready, EXEC);
            
        }
    }
}

void *waiter_devoluciones_cpu(void * args){
    
    t_socket_cpu * socket_cpu = (t_socket_cpu *)args;
    struct timespec inicio_rafaga;

    PCB * pcb;

    while(true){

        sem_wait(lista_procesos_exec->sem);
        
        pcb = list_get(lista_procesos_exec->lista, 0);
        
        clock_gettime(CLOCK_MONOTONIC, &inicio_rafaga);

        esperar_respuesta_cpu(pcb, socket_cpu);
        
        pthread_mutex_lock(lista_procesos_exec->mutex);
        if(!list_remove_element(lista_procesos_exec->lista, pcb)){
            log_error(logger, "No se pudo remover pid %d de exec", pcb->pid);
        }
        pthread_mutex_unlock(lista_procesos_exec->mutex);

        //
        struct timespec fin_rafaga;
        clock_gettime(CLOCK_MONOTONIC, &fin_rafaga);

        pcb->rafaga_real_anterior = diff_in_milliseconds(inicio_rafaga, fin_rafaga);
        actualizar_estimacion(pcb);


    }
}

void enviar_interrupcion(t_socket_cpu *socket_cpu, int pid, int pc) {
    t_paquete *paquete = crear_paquete(DESALOJO_CPU); // interrup
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &pc, sizeof(int));
    
    enviar_paquete(paquete, socket_cpu->dispatch);
    eliminar_paquete(paquete);
}
