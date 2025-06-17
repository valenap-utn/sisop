#include <peticiones_memoria.h>

extern list_struct_t * lista_peticiones_pendientes;


/// @brief Es necesario enviar una peticion a la lista de peticiones, y esperar a que postee el semaforo interno de la peticion. Despues, leer el bool de respuesta_exitosa. Importante recordar de liberar la peticion al recibir la respuesta
/// @param arg_server 
/// @return 
void *administrador_peticiones_memoria(void* arg_server){
	t_peticion_largoPlazo *peticion;
	t_args_peticion_largoPlazo *args_peticion = malloc(sizeof(t_args_peticion_largoPlazo));
	argumentos_thread * args = arg_server;
	pthread_t aux_thread;
    int socket_memoria = -1;
	
	while(1){
		sem_wait(lista_peticiones_pendientes->sem);
		pthread_mutex_lock(lista_peticiones_pendientes->mutex);
		peticion = list_remove(lista_peticiones_pendientes->lista, 0);
		pthread_mutex_unlock(lista_peticiones_pendientes->mutex);
		do{
			socket_memoria = crear_conexion(args->ip, args->puerto);
			sleep(1);

		}while(socket_memoria == -1);
		args_peticion->peticion = peticion;
		args_peticion->socket = socket_memoria; 
		pthread_create(&aux_thread, NULL, peticion_kernel, (void *)args_peticion);
        
        //Aca meti una nueva inicializacion, el anterior no queda flotando porque peticion_kernel tiene su direccion y luego hace un free.
        //El nuevo malloc asegura que no se modificara el contenido antes que peticion_kernel pueda guardarlo en su stack local.
        //Asumo que esto ya es suficiente para salvarnos de la posible condicion de carrera, en caso que haya una nueva peticion cercana.
        args_peticion = malloc(sizeof(t_args_peticion_largoPlazo));

		pthread_detach(aux_thread);
	}
    pthread_exit(EXIT_SUCCESS);
}

void *peticion_kernel(void *args) {
    t_args_peticion_largoPlazo *args_peticion = args;
    int socket = args_peticion->socket;
    t_peticion_largoPlazo *peticion = args_peticion->peticion;
    free(args);

    PCB *proceso = peticion->proceso;
    t_paquete *send_protocolo;
    protocolo_socket op;
    switch (peticion->tipo) {
        // //analizar si esta bien
        // case PROCESS_CREATE_OP:
        //     send_protocolo = crear_paquete(PROCESS_CREATE_OP);
        //     agregar_a_paquete(send_protocolo, &proceso->pid, sizeof(proceso->pid));
        //     agregar_a_paquete(send_protocolo, &proceso->memoria_necesaria, sizeof(proceso->memoria_necesaria));
        //     agregar_a_paquete(send_protocolo, &proceso->estado, sizeof(proceso->estado));
		// 	log_info(logger, "Se envió la peticion de PROCESS CREATE del PID: %d Tamaño: %d", proceso->pid, proceso->memoria_necesaria);
        //     enviar_paquete(send_protocolo, socket);
        //     op = recibir_operacion(socket);
        //     switch (op) {
        //         case SUCCESS:
        //             log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
        //             peticion->respuesta_exitosa = true;
        //             break;

        //         case ERROR:
        //             log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
        //             peticion->respuesta_exitosa = false;
        //             break;

        //         case OK:
        //             log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
        //             peticion->respuesta_exitosa = true;
        //             break;	

        //         default:
        //             log_info(logger, "Código de operación desconocido recibido: %d", op);
        //             peticion->respuesta_exitosa = false;
        //             break;
        //     }
        //     break;

        // //same anterior
        // case PROCESS_EXIT_OP:
        //     send_protocolo = crear_paquete(PROCESS_EXIT_OP);
        //     agregar_a_paquete(send_protocolo, &proceso->pid, sizeof(int));
		// 	log_info(logger, "Se envió la peticion de PROCESS EXIT del PID: %d", proceso->pid);
		// 	//sem_post(sem_proceso_finalizado);
        //     enviar_paquete(send_protocolo, socket);
        //     op = recibir_operacion(socket);
        //     switch (op) {
        //         case SUCCESS:
        //             log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
        //             peticion->respuesta_exitosa = true;
        //             break;

        //         case ERROR:
        //             log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
        //             peticion->respuesta_exitosa = false;
        //             break;

        //         case OK:
        //             log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
        //             peticion->respuesta_exitosa = true;
        //             break;	

        //         default:
        //             log_info(logger, "Código de operación desconocido recibido: %d", op);
        //             peticion->respuesta_exitosa = false;
        //             break;
        //     }
        //     break;

        // //redefinir
        // case DUMP_MEMORY_OP:
        //     send_protocolo = crear_paquete(DUMP_MEMORY_OP);
        //     agregar_a_paquete(send_protocolo, &hilo_actual->tid, sizeof(int));
		// 	agregar_a_paquete(send_protocolo, &proceso_actual->pid, sizeof(int));
		// 	log_info(logger, "Se envió la peticion de DUMP MEMORY");
           
        //     enviar_paquete(send_protocolo, socket);
        //     op = recibir_operacion(socket);
        //     switch (op) {
        //         case SUCCESS:
        //             log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
        //             peticion->respuesta_exitosa = true;
        //             break;

        //         case ERROR:
        //             log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
        //             peticion->respuesta_exitosa = false;
        //             break;

        //         case OK:
        //             log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
        //             peticion->respuesta_exitosa = true;
        //             break;	

        //         default:
        //             log_info(logger, "Código de operación desconocido recibido: %d", op);
        //             peticion->respuesta_exitosa = false;
        //             break;
        //     }
        //     break;
        default:
            log_error(logger, "En peticion_kernel: Tipo de operación desconocido: %d", peticion->tipo);
            return NULL;
    }

    
	sem_post(peticion->peticion_finalizada);
    eliminar_paquete(send_protocolo);
    liberar_conexion(socket);

    return NULL;
}