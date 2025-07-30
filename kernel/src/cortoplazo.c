#include <cortoplazo.h>

extern enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;
extern t_log *logger;

extern int flag_all_start;

extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_exec;
extern list_struct_t *lista_sockets_cpu_libres;
extern list_struct_t *lista_sockets_cpu_ocupados;
extern list_struct_t *lista_sockets_io;

extern sem_t * sem_syscall;
pthread_mutex_t * mutex_waiter;


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
        PCB* pcb = NULL;
        if (list_is_empty(lista_procesos_ready->lista)) {
            log_warning(logger, "No se pudo remover PCB: la lista de procesos READY está vacía.");
        } else {
            pcb = list_remove(lista_procesos_ready->lista, 0);
            pthread_mutex_unlock(lista_procesos_ready->mutex);
            log_info(logger, "## (%d) - Planificado por FIFO", pcb->pid);
            enviar_a_cpu_dispatch(pcb, socket_cpu);
            cambiar_estado(pcb, EXEC);
            esperar_respuesta_cpu(pcb, socket_cpu);
        }
    }

}
void enviar_a_cpu_dispatch_srt(PCB *pcb, t_socket_cpu *socket_cpu, list_struct_t * lista_exec) {

    t_paquete *paquete = crear_paquete(DISPATCH_CPU);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pc), sizeof(int));

    

    enviar_paquete(paquete, socket_cpu->dispatch);

    recibir_paquete_ok(socket_cpu->interrupt);

    log_info(logger, "## (%d) - Enviado a CPU con PC=%d", pcb->pid, pcb->pc);

    eliminar_paquete(paquete);

    encolar_cola_generico(lista_exec, pcb, 0);

}

void enviar_a_cpu_dispatch(PCB *pcb, t_socket_cpu *socket_cpu) {

    t_paquete *paquete = crear_paquete(DISPATCH_CPU);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(int));
    agregar_a_paquete(paquete, &(pcb->pc), sizeof(int));

    

    enviar_paquete(paquete, socket_cpu->dispatch);

    recibir_paquete_ok(socket_cpu->interrupt);

    log_info(logger, "## (%d) - Enviado a CPU con PC=%d", pcb->pid, pcb->pc);

    eliminar_paquete(paquete);

    encolar_cola_generico(lista_exec, pcb, 0);

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

            if(algoritmo_cortoPlazo == CPL_SJF){
                sem_post(lista_procesos_ready->sem);
            }

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
        sem_post(sem_syscall);
    }
}
void esperar_respuesta_cpu_sjf(PCB * pcb, t_socket_cpu *socket_cpu, list_struct_t * lista_exec){
    protocolo_socket motivo;
    
    log_info(logger, "Esperando motivo de devolucion de CPU");
    motivo = recibir_operacion(socket_cpu->interrupt);

    t_list *paquete_respuesta = recibir_paquete(socket_cpu->interrupt);

    pthread_mutex_lock(mutex_waiter);

    switch (motivo) {

        case PROCESS_EXIT_CPU:

            log_info(logger, "## (PID: %d) - Solicitó syscall: PROCESS EXIT", pcb->pid);
            
            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);

            pthread_mutex_lock(lista_exec->mutex);
            if(!list_remove_element(lista_exec->lista, pcb)){
               log_error(logger, "No se pudo remover pid %d de exec", pcb->pid);
            }
            log_debug(logger, "se remueve de exec: %d", pcb->pid);
            log_debug(logger, "elementos en exec despues de remover: %d", list_size(lista_exec->lista));
            pthread_mutex_unlock(lista_exec->mutex);

            PROCESS_EXIT(pcb);
            sem_post(lista_procesos_ready->sem);

            break;

        case DESALOJO_CPU:
            
            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);
            pcb->pc = *(int*)list_remove(paquete_respuesta, 0);

            pthread_mutex_lock(lista_exec->mutex);
            if(!list_remove_element(lista_exec->lista, pcb)){
               log_error(logger, "No se pudo remover pid %d de exec", pcb->pid);
            }
            log_debug(logger, "se remueve de exec: %d", pcb->pid);
            log_debug(logger, "elementos en exec despues de remover: %d", list_size(lista_exec->lista));

            pthread_mutex_unlock(lista_exec->mutex);

            cambiar_estado(pcb, READY);
            encolar_cola_generico(lista_procesos_ready, pcb, -1);

            break;

        case PROCESS_INIT_CPU:
            log_info(logger, "## (PID: %d) - Solicitó syscall: PROCESS INIT", pcb->pid);

            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);
            pcb->pc = *(int*)list_remove(paquete_respuesta, 0);

            char * path = list_remove(paquete_respuesta, 0);
            int tamaño = *(int*)list_remove(paquete_respuesta, 0);
            

            pthread_mutex_lock(lista_exec->mutex);
            if(!list_remove_element(lista_exec->lista, pcb)){
               log_error(logger, "No se pudo remover pid %d de exec", pcb->pid);
            }
            log_debug(logger, "se remueve de exec: %d", pcb->pid);
            log_debug(logger, "elementos en exec despues de remover: %d", list_size(lista_exec->lista));
            pthread_mutex_unlock(lista_exec->mutex);

            // vuelve a READY - reencolado por desalojo
            cambiar_estado(pcb, READY);
            encolar_cola_generico(lista_procesos_ready, pcb, -1);

            PROCESS_CREATE(path, tamaño);

            break;

        case DUMP_MEM_CPU:

            log_info(logger, "## (PID: %d) - Solicitó syscall: DUMP MEMORY", pcb->pid);

            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);
            pcb->pc = *(int*)list_remove(paquete_respuesta, 0);


            pthread_mutex_lock(lista_exec->mutex);
            if(!list_remove_element(lista_exec->lista, pcb)){
               log_error(logger, "No se pudo remover pid %d de exec", pcb->pid);
            }
            log_debug(logger, "se remueve de exec: %d", pcb->pid);
            log_debug(logger, "elementos en exec despues de remover: %d", list_size(lista_exec->lista));
            pthread_mutex_unlock(lista_exec->mutex);

            DUMP_MEMORY(pcb);

            break;

        case IO_CPU:

            log_info(logger, "## (PID: %d) - Solicitó syscall: IO", pcb->pid);

            pcb->pid = *(int*)list_remove(paquete_respuesta, 0);
            pcb->pc = *(int*)list_remove(paquete_respuesta, 0);

            char * nombre_io = list_remove(paquete_respuesta, 0);
            int tiempo = *(int *)list_remove(paquete_respuesta, 0);


            pthread_mutex_lock(lista_exec->mutex);
            if(!list_remove_element(lista_exec->lista, pcb)){
               log_error(logger, "No se pudo remover pid %d de exec", pcb->pid);
            }
            log_debug(logger, "se remueve de exec: %d", pcb->pid);
            log_debug(logger, "elementos en exec despues de remover: %d", list_size(lista_exec->lista));
            pthread_mutex_unlock(lista_exec->mutex);

            IO_syscall(pcb, nombre_io, tiempo);

            break;

        default:
            log_info(logger, "Motivo: %d desconocido para el pid %d\n", motivo, pcb->pid);
            list_destroy(paquete_respuesta);
            break;
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

    mutex_waiter = inicializarMutex();

    list_struct_t * lista_procesos_exec_srt = inicializarLista();
 
    t_waiter_args * args = malloc(sizeof(t_waiter_args *));

    args->lista_exec = lista_procesos_exec_srt;
    args->socket = socket_cpu;

    pthread_t tid_waiter;
    pthread_create(&tid_waiter, NULL, waiter_devoluciones_cpu, (void*)args);
    pthread_detach(tid_waiter);

    
    while (true) {

        sem_wait(lista_procesos_ready->sem);


        // proceso con menor estimación
        int indice_ready_menor = buscar_indice_proceso_menor_estimacion();
        if (indice_ready_menor == -1) continue; // si no hay procesos en READY -> saltear esta vuelta del while

        pthread_mutex_lock(lista_procesos_ready->mutex);
        PCB *pcb_ready = list_remove(lista_procesos_ready->lista, indice_ready_menor);
        pthread_mutex_unlock(lista_procesos_ready->mutex);

        // hay proceso ejecutando?
        pthread_mutex_lock(mutex_waiter);
        pthread_mutex_lock(lista_procesos_exec_srt->mutex);
        bool hay_proceso_en_exec = !list_is_empty(lista_procesos_exec_srt->lista);
        

        if (hay_proceso_en_exec) {
            PCB *pcb_exec = list_get(lista_procesos_exec_srt->lista, 0); // unico proc en EXEC
            pthread_mutex_unlock(lista_procesos_exec_srt->mutex);
            


            // comparar estimaciones
            log_debug(logger, "%d ready: %s, %d exec: %s", pcb_ready->pid, string_itoa(pcb_ready->estimacion_rafaga), pcb_exec->pid, string_itoa(pcb_exec->estimacion_rafaga));
            if (pcb_ready->estimacion_rafaga < pcb_exec->estimacion_rafaga) {
                if(pcb_ready->pid == pcb_exec->pid){
                    log_error(logger, "el proceso esta duplicado: %d", pcb_ready->pid);
                }
                
                //si hay una interrupcion anterior pendiente (2 IOs terminaron a la vez)
                pthread_mutex_lock(lista_procesos_exec_srt->mutex);
                if (list_size(lista_procesos_exec_srt->lista)>1){
                    pthread_mutex_lock(lista_procesos_ready->mutex);

                    log_debug(logger, "Llego una interrupcion cuando habia una pendiente, cancelando interrupcion pid: %d", pcb_ready->pid);
                    int index_aux = 0;
                    if (list_is_empty(lista_procesos_ready->lista)){
                        index_aux = 0;
                    }else{
                        PCB * pcb_aux;
                        t_list_iterator *iterator = list_iterator_create(lista_procesos_ready->lista);
                        while(list_iterator_has_next(iterator)){
                            pcb_aux = list_iterator_next(iterator);
                        }
                        index_aux = list_iterator_index(iterator)+1;
                        list_iterator_destroy(iterator);
                    }

                    list_add_in_index(lista_procesos_ready->lista, index_aux, pcb_ready);
                    pthread_mutex_unlock(lista_procesos_ready->mutex);
                    pthread_mutex_unlock(mutex_waiter);
                    pthread_mutex_unlock(lista_procesos_exec_srt->mutex);
                    continue;
                }

                pthread_mutex_unlock(lista_procesos_exec_srt->mutex);
                log_info(logger, "## (%d) - Desalojado por algoritmo SJF/SRT", pcb_exec->pid);
                log_debug(logger, "(%d) - Entro por interrupt, pc: %d", pcb_ready->pid, pcb_ready->pc);
                enviar_interrupcion(socket_cpu, pcb_ready->pid, pcb_ready->pc);
                encolar_cola_generico(lista_procesos_exec_srt, pcb_ready, -1);
                cambiar_estado(pcb_ready, EXEC);
                pthread_mutex_unlock(mutex_waiter);
            }else {
                pthread_mutex_lock(lista_procesos_ready->mutex);

                //hago esta asquerosidad para agregar al final de la lista
                int index_aux = 0;
                if (list_is_empty(lista_procesos_ready->lista)){
                    index_aux = 0;
                }else{
                    PCB * pcb_aux;
                    t_list_iterator *iterator = list_iterator_create(lista_procesos_ready->lista);
                    while(list_iterator_has_next(iterator)){
                        pcb_aux = list_iterator_next(iterator);
                    }
                    index_aux = list_iterator_index(iterator)+1;
                    list_iterator_destroy(iterator);
                }

                list_add_in_index(lista_procesos_ready->lista, index_aux, pcb_ready);
                pthread_mutex_unlock(lista_procesos_ready->mutex);
                pthread_mutex_unlock(mutex_waiter);
            }
        } else {
            pthread_mutex_unlock(lista_procesos_exec_srt->mutex);
            pthread_mutex_unlock(mutex_waiter);

            // no hay proc en EXEC entonces lo saco de ready y lo mando

            log_debug(logger, "(%d) - enviado a Dispatch", pcb_ready->pid);

            enviar_a_cpu_dispatch_srt(pcb_ready, socket_cpu, lista_procesos_exec_srt);
            cambiar_estado(pcb_ready, EXEC);

            sem_post(lista_procesos_ready->sem);
            
        }
    }
}

void *waiter_devoluciones_cpu(void * args){
    
    t_waiter_args * argumentos = args;
    
    t_socket_cpu * socket_cpu = argumentos->socket;
    list_struct_t * lista_exec = argumentos->lista_exec;
    struct timespec inicio_rafaga;
    PCB * pcb;

    while(true){

        sem_wait(lista_exec->sem);
        
        pcb = list_get(lista_exec->lista, 0);
        
        clock_gettime(CLOCK_MONOTONIC, &inicio_rafaga);

        esperar_respuesta_cpu_sjf(pcb, socket_cpu, lista_exec);
        
        
        pthread_mutex_unlock(mutex_waiter);
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

//------------------------------SRT-------------------------------

// Guardar el timestamp actual en el PCB, indicando cuándo empieza a ejecutarse.
void iniciar_medicion_rafaga(PCB *pcb) {
    clock_gettime(CLOCK_MONOTONIC, &pcb->timestamp_ultimo_estado);
}

// Calcular cuánto tiempo ejecutó realmente el proceso y actualizar la estimación.
void finalizar_medicion_y_actualizar_estimacion(PCB *pcb) {
    struct timespec fin_rafaga;
    clock_gettime(CLOCK_MONOTONIC, &fin_rafaga);

    long duracion_real = diff_in_milliseconds(pcb->timestamp_ultimo_estado, fin_rafaga);
    pcb->rafaga_real_anterior = duracion_real;

    extern double alfa;
    double estimacion_anterior = pcb->estimacion_rafaga;

    pcb->estimacion_rafaga = alfa * duracion_real + (1 - alfa) * estimacion_anterior;
}