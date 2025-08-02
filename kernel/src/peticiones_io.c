#include <peticiones_io.h>

extern char* puerto_io;
extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_procesos_block;
extern list_struct_t *lista_procesos_susp_block;
extern list_struct_t *lista_procesos_susp_ready;
extern list_struct_t *lista_sockets_io;

sem_t * sem_IO_liberado;

extern list_struct_t *lista_procesos_esperando_io;

extern enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;

extern int tiempo_suspension;

extern sem_t *sem_memoria_liberada;

void * manager_io(void *args){

    pthread_t tid_io;

    while(true){
        sem_wait(lista_procesos_esperando_io->sem);

        elemento_cola_blocked_io * elemento_esperando_io = desencolar_cola_blocked(lista_procesos_esperando_io);
        
        //verificar si existe el IO buscado
        if(verificar_si_existe_io(elemento_esperando_io->nombre_io)){
            log_error(logger, "No existe el IO solicitado. Finalizando proceso %d", elemento_esperando_io->pcb->pid);
            PROCESS_EXIT(elemento_esperando_io->pcb);
            free(elemento_esperando_io);
            continue;
        }

        // buscar y asignar un IO
        int index = buscar_io(elemento_esperando_io->nombre_io);

        if(index == -1){
            //reencola al principio y espera que se libere un IO cualquiera
            log_debug(logger, "No se encontro dispositivo IO libre para %d", elemento_esperando_io->pcb->pid);
            encolar_cola_blocked(lista_procesos_esperando_io, elemento_esperando_io, true);
            sem_wait(sem_IO_liberado);
            continue;
        }

        //cuando se encuentre uno libre:
        t_socket_io *socket_io = get_socket_io(index);

        //se asigna el thread y seteo la flag en false
        pthread_mutex_lock(lista_sockets_io->mutex);
        socket_io->libre = false;
        pthread_mutex_unlock(lista_sockets_io->mutex);

        args_thread_io * args_thread_io = malloc(sizeof(args_thread_io));

        args_thread_io->elemento_cola_blocked_io = elemento_esperando_io;
        args_thread_io->socket_io = socket_io;

        pthread_create(&tid_io, NULL, thread_io, (void *)args_thread_io);
        pthread_detach(tid_io);

    }

}

void *server_mh_io(void *args){

    int server = iniciar_servidor(puerto_io);
    pthread_t tid_aux;

    sem_IO_liberado = inicializarSem(0);

    t_socket_io *socket_nuevo;
    t_list *paquete_recv;

    char *nombre_io;

    protocolo_socket cod_op;
    socket_nuevo = inicializarSocketIO();

    while((socket_nuevo->socket = esperar_cliente(server))){
        
        cod_op = recibir_operacion(socket_nuevo->socket);

        if(cod_op != NOMBRE_IO){
            log_error(logger, "Se recibio un protocolo inesperado de IO");
            return (void*)EXIT_FAILURE;
        }

        paquete_recv = recibir_paquete(socket_nuevo->socket);

        nombre_io = list_remove(paquete_recv, 0);
        // socket_nuevo->nombre = nombre_io;
        socket_nuevo->nombre = nombre_io;
        
        pthread_mutex_lock(lista_sockets_io->mutex);
        list_add(lista_sockets_io->lista, socket_nuevo);
        pthread_mutex_unlock(lista_sockets_io->mutex);

        log_info(logger, "Se conecto un nuevo IO %s", nombre_io);
        //destraba el manager_io cuando se agreguen io nuevos
        sem_post(sem_IO_liberado);

        // pthread_create(&tid_aux, NULL, thread_io, (void *)socket_nuevo);
        // pthread_detach(tid_aux);

        socket_nuevo = inicializarSocketIO();

    }
    return (void *)EXIT_SUCCESS;
}
/// @brief thread principal que maneja las peticiones a io

/// @brief detecta nuevos procesos en su cola blocked y los manda a esperar a su IO
/// @param args 
/// @return 
void * thread_io(void * args){

    args_thread_io * argumentos = args;
    t_socket_io * socket_io = argumentos->socket_io;
    PCB * proceso_aux;
    elemento_cola_blocked_io * elemento_cola = argumentos->elemento_cola_blocked_io;
    t_paquete * paquete_send;

    proceso_aux = elemento_cola->pcb;

    paquete_send = crear_paquete(DORMIR_IO);
    agregar_a_paquete(paquete_send, &proceso_aux->pid, sizeof(int));
    agregar_a_paquete(paquete_send, &elemento_cola->tiempo, sizeof(int));

    enviar_paquete(paquete_send, socket_io->socket);
    log_debug(logger, "Envio pid: %d al IO: %s", elemento_cola->pcb->pid, elemento_cola->nombre_io);

    if(recibir_paquete_ok(socket_io->socket)){
        log_error(logger, "El dispositivo IO %s se desconecto prematuramente", socket_io->nombre);

        //finalizar el timer suspend
        pthread_mutex_lock(lista_procesos_block->mutex);
        pthread_cancel(elemento_cola->tid_suspend);
        pthread_mutex_unlock(lista_procesos_block->mutex);

        //si esta en block:
        pthread_mutex_lock(lista_procesos_block->mutex);
        
        if(list_remove_element(lista_procesos_block->lista, proceso_aux)){
            PROCESS_EXIT(proceso_aux);
        }
        //si esta en susp_block
        else if(list_remove_element(lista_procesos_susp_block->lista, proceso_aux)){
            PROCESS_EXIT(proceso_aux);
        }pthread_mutex_unlock(lista_procesos_block->mutex);
        if((algoritmo_cortoPlazo == CPL_SJF_CD) || (algoritmo_cortoPlazo == CPL_SJF)){

            sem_post(lista_procesos_ready->sem);
        }

        liberar_socket_io(socket_io);
        pthread_mutex_lock(lista_sockets_io->mutex);
        list_remove_element(lista_sockets_io->lista, socket_io);
        pthread_mutex_unlock(lista_sockets_io->mutex);
        sem_post(sem_IO_liberado);
        return (void *)EXIT_FAILURE;
    }else{
        log_info(logger, "## (PID: %d) finalizó IO y pasa a READY", proceso_aux->pid);

        //finalizar el timer suspend
        pthread_mutex_lock(lista_procesos_block->mutex);
        pthread_cancel(elemento_cola->tid_suspend);
        pthread_mutex_unlock(lista_procesos_block->mutex);
        
        //si esta en block:
        pthread_mutex_lock(lista_procesos_block->mutex);
        if(list_remove_element(lista_procesos_block->lista, proceso_aux)){
            log_debug(logger, "Se mueve el pcb de block a ready por thread_io");
            encolar_cola_generico(lista_procesos_ready, proceso_aux, -1);
            cambiar_estado(proceso_aux, READY);
        }
        //si esta en susp_block
        else if(list_remove_element(lista_procesos_susp_block->lista, proceso_aux)){
            log_debug(logger, "Se mueve el pcb de susp_block a susp_ready por thread_io");
            encolar_cola_generico(lista_procesos_susp_ready, proceso_aux, -1);
            cambiar_estado(proceso_aux, SUP_READY);
        }else log_error(logger, "en hilo IO el proceso no esta ni en blocked ni en susp_blocked");
        
        pthread_mutex_unlock(lista_procesos_block->mutex);
        socket_io->libre = true;
        sem_post(sem_IO_liberado);
        return (void *)EXIT_SUCCESS;
    }
}
t_socket_io * inicializarSocketIO(){
    t_socket_io * socket_io = calloc(1, sizeof(t_socket_io));
    socket_io->libre = true;

    return socket_io;
}
void liberar_socket_io(t_socket_io *socket){
    free (socket->nombre);
    close(socket->socket);

    free(socket);

}
/// @param nombre_a_buscar 
/// @return devuelve el primer io vacio con el nombre buscado.
/// si no hay libres, devuelve -1
int buscar_io(char * nombre_a_buscar){

    pthread_mutex_lock(lista_sockets_io->mutex);

    if(list_is_empty(lista_sockets_io->lista)){
        pthread_mutex_unlock(lista_sockets_io->mutex);
        return -1;
    }

    t_list_iterator * iterator = list_iterator_create(lista_sockets_io->lista);
    t_socket_io * socket_io = NULL;

    while (list_iterator_has_next(iterator)){
        socket_io = list_iterator_next(iterator);
        if ((socket_io->libre)&&(!strcmp(socket_io->nombre, nombre_a_buscar))){
            pthread_mutex_unlock(lista_sockets_io->mutex);
            return list_iterator_index(iterator);
        }else socket_io = NULL;
    }
    list_iterator_destroy(iterator);

    pthread_mutex_unlock(lista_sockets_io->mutex);

    if (socket_io == NULL){
        return -1;
    }
}
/// @brief Devuelve 0 si existe el io buscado
/// @param nombre_a_buscar 
/// @return 0 si existe, -1 si no
int verificar_si_existe_io(char * nombre_a_buscar){

    pthread_mutex_lock(lista_sockets_io->mutex);

    if(list_is_empty(lista_sockets_io->lista)){
        pthread_mutex_unlock(lista_sockets_io->mutex);
        return -1;
    }

    t_list_iterator * iterator = list_iterator_create(lista_sockets_io->lista);
    t_socket_io * socket_io = NULL;

    while (list_iterator_has_next(iterator)){
        socket_io = list_iterator_next(iterator);
        if (!strcmp(socket_io->nombre, nombre_a_buscar)){
            pthread_mutex_unlock(lista_sockets_io->mutex);
            return 0;
        }else socket_io = NULL;
    }
    list_iterator_destroy(iterator);

    pthread_mutex_unlock(lista_sockets_io->mutex);

    if (socket_io == NULL){
        return -1;
    }
}
t_socket_io * get_socket_io(int index){
    pthread_mutex_lock(lista_sockets_io->mutex);
    t_socket_io * socket_io = list_get(lista_sockets_io->lista, index);
    pthread_mutex_unlock(lista_sockets_io->mutex);
    return socket_io;
}
elemento_cola_blocked_io * desencolar_cola_blocked(list_struct_t *cola){

    elemento_cola_blocked_io *elem;
    pthread_mutex_lock(cola->mutex);

    elem = list_remove(cola->lista, 0);

    pthread_mutex_unlock(cola->mutex);

    return elem;
}
void encolar_cola_blocked(list_struct_t *cola, elemento_cola_blocked_io *elem, int principio){

    pthread_mutex_lock(cola->mutex);

    if (principio){
        list_add_in_index(cola->lista, 0, elem);
    }else{
        int index_aux = 0;
        if (list_is_empty(cola->lista)){
            index_aux = 0;
        }else{
            elemento_cola_blocked_io * elem_aux;
            t_list_iterator *iterator = list_iterator_create(cola->lista);
            while(list_iterator_has_next(iterator)){
                elem_aux = list_iterator_next(iterator);
            }
            index_aux = list_iterator_index(iterator)+1;
            list_iterator_destroy(iterator);
        }
        list_add_in_index(cola->lista, index_aux, elem);
    }
    sem_post(cola->sem);  
    pthread_mutex_unlock(cola->mutex);
}
void * timer_suspend(void * args){
    PCB * pcb = args;

    PCB *pcb_aux;
    t_peticion_memoria * peticion = inicializarPeticionMemoria();
    
    usleep(tiempo_suspension*1000);

    pthread_mutex_lock(lista_procesos_block->mutex);
    t_list_iterator *iterator = list_iterator_create(lista_procesos_block->lista);

    while(list_iterator_has_next(iterator)){
        pcb_aux = list_iterator_next(iterator);
        if (pcb_aux == pcb){
            //pcb sigue en block
            list_iterator_remove(iterator);
            encolar_cola_generico(lista_procesos_susp_block, pcb_aux, -1);
            cambiar_estado(pcb, SUP_BLOCK);
            peticion->proceso = pcb_aux;
            peticion->tipo = SUSP_MEM;
            encolarPeticionMemoria(peticion);
            sem_wait(peticion->peticion_finalizada);
            cambiar_estado(pcb_aux, SUP_BLOCK);
            sem_post(sem_memoria_liberada);
            break;
        }
    }
    pthread_mutex_unlock(lista_procesos_block->mutex);
}
