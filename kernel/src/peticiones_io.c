#include <peticiones_io.h>

extern char* puerto_io;
extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_procesos_block;
extern list_struct_t *lista_procesos_susp_block;
extern list_struct_t *lista_procesos_susp_ready;
extern list_struct_t *lista_sockets_io;

extern enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;

extern int tiempo_suspension;

extern sem_t *sem_memoria_liberada;



void *server_mh_io(void *args){

    int server = iniciar_servidor(puerto_io);
    pthread_t tid_aux;

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

        pthread_create(&tid_aux, NULL, thread_io, (void *)socket_nuevo);
        pthread_detach(tid_aux);

        log_debug(logger, "%s", socket_nuevo->nombre);

        socket_nuevo = inicializarSocketIO();

    }
    return (void *)EXIT_SUCCESS;
}
/// @brief thread principal que maneja las peticiones a io

/// @brief detecta nuevos procesos en su cola blocked y los manda a esperar a su IO
/// @param args 
/// @return 
void * thread_io(void * args){

    t_socket_io * socket_io = args;
    PCB * proceso_aux;
    elemento_cola_blocked_io * elemento_cola;
    t_paquete * paquete_send;
    pthread_t tid_aux;

    while(1){
        sem_wait(socket_io->cola_blocked->sem);

        elemento_cola = desencolar_cola_blocked(socket_io->cola_blocked);
        proceso_aux = elemento_cola->pcb;

        paquete_send = crear_paquete(DORMIR_IO);
        agregar_a_paquete(paquete_send, &proceso_aux->pid, sizeof(int));
        agregar_a_paquete(paquete_send, &elemento_cola->tiempo, sizeof(int));

        enviar_paquete(paquete_send, socket_io->socket);

        //arranca el timer de suspend
        pthread_create(&tid_aux, NULL, timer_suspend, (void*)proceso_aux);

        if(recibir_paquete_ok(socket_io->socket)){
            log_error(logger, "El dispositivo IO %s se desconecto prematuramente", socket_io->nombre);

            //finalizar el timer suspend
            pthread_mutex_lock(lista_procesos_block->mutex);
            pthread_cancel(tid_aux);
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
            if(algoritmo_cortoPlazo == CPL_SJF_CD){

                sem_post(lista_procesos_ready->sem);
            }

            t_list_iterator * iterator = list_iterator_create(socket_io->cola_blocked->lista);
            elemento_cola_blocked_io * aux_blocked;
            while (list_iterator_has_next(iterator)){
                aux_blocked = list_iterator_next(iterator);
                //si esta en block:
                pthread_mutex_lock(lista_procesos_block->mutex);
                
                if(list_remove_element(lista_procesos_block->lista, aux_blocked->pcb)){
                    PROCESS_EXIT(aux_blocked->pcb);
                }
                //si esta en susp_block
                else if(list_remove_element(lista_procesos_susp_block->lista, aux_blocked->pcb)){
                    PROCESS_EXIT(aux_blocked->pcb);
                }
                pthread_mutex_unlock(lista_procesos_block->mutex);
                if(algoritmo_cortoPlazo == CPL_SJF_CD){

                    sem_post(lista_procesos_ready->sem);
                }
            }

            liberar_socket_io(socket_io);
            pthread_mutex_lock(lista_sockets_io->mutex);
            list_remove_element(lista_sockets_io->lista, socket_io);
            pthread_mutex_unlock(lista_sockets_io->mutex);
            return (void *)EXIT_FAILURE;
        }else{
            log_info(logger, "## (PID: %d) finalizó IO y pasa a READY", proceso_aux->pid);

            //finalizar el timer suspend
            pthread_mutex_lock(lista_procesos_block->mutex);
            pthread_cancel(tid_aux);
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

        }
    }
}
t_socket_io * inicializarSocketIO(){
    t_socket_io * socket_io = calloc(1, sizeof(t_socket_io));
    socket_io->cola_blocked = inicializarLista();

    return socket_io;
}
void liberar_socket_io(t_socket_io *socket){
    list_destroy(socket->cola_blocked->lista);
    sem_destroy(socket->cola_blocked->sem);
    pthread_mutex_destroy(socket->cola_blocked->mutex);
    free (socket->nombre);
    close(socket->socket);

    free(socket);

}
/// @param nombre_a_buscar 
/// @return devuelve el primer io vacio con el nombre buscado.
/// si no hay vacios, devuelve el primero que tenga el nombre buscado
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
        if ((list_is_empty(socket_io->cola_blocked->lista))&&(!strcmp(socket_io->nombre, nombre_a_buscar))){
            pthread_mutex_unlock(lista_sockets_io->mutex);
            return list_iterator_index(iterator);
        }else socket_io = NULL;
    }
    list_iterator_destroy(iterator);

    if (socket_io == NULL){
        iterator = list_iterator_create(lista_sockets_io->lista);

        while (list_iterator_has_next(iterator)){
            socket_io = list_iterator_next(iterator);
            if (!strcmp(socket_io->nombre, nombre_a_buscar)){
                pthread_mutex_unlock(lista_sockets_io->mutex);
                return list_iterator_index(iterator);
            }else socket_io = NULL;
        }
    }
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
void encolar_cola_blocked(list_struct_t *cola, elemento_cola_blocked_io *elem){

    pthread_mutex_lock(cola->mutex);

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
            peticion->proceso = pcb_aux;
            peticion->tipo = SUSP_MEM;
            encolarPeticionMemoria(peticion);
            sem_wait(peticion->peticion_finalizada);
            sem_post(sem_memoria_liberada);
            break;
        }
    }
    pthread_mutex_unlock(lista_procesos_block->mutex);
}