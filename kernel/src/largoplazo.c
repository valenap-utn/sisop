#include <largoplazo.h>

extern enum_algoritmo_largoPlazo algoritmo_largoPlazo;
extern t_log *logger;

extern list_struct_t *lista_procesos_new;
extern sem_t *sem_proceso_fin;

void *largoPlazo(void *args){
    switch(algoritmo_largoPlazo){
        case LPL_FIFO:
            largoPlazoFifo();
            break;

        //Agregar aca mientras se van haciendo
        
        default:
            log_error(logger, "algoritmo no reconocido en Largo Plazo");

    }
    return (void *)EXIT_SUCCESS;
}

void largoPlazoFifo(){
    while(true){
        sem_wait(lista_procesos_new->sem); // espera a que haya un nuevo elemento en la cola_new
        log_debug(logger, "Nuevo proceso a crear: Intentando cargar en memoria");
        PCB * pcb = desencolar_cola_new(0);
        if(!encolarPeticionLargoPlazo(pcb)){
            encolar_cola_new(pcb);
            sem_wait(sem_proceso_fin); //espera a que un proceso finalize para volver a intentar enviar peticiones a memoria
        }
    }
    return;
}
// void *administrador_peticiones_memoria(void* arg_server){
// 	t_peticion *peticion;
// 	t_paquete_peticion args_peticion; 
// 	argumentos_thread * args = arg_server;
// 	pthread_t aux_thread;
	
// 	while(1){
// 		sem_wait(sem_lista_t_peticiones);
// 		pthread_mutex_lock(mutex_lista_t_peticiones);
// 		peticion = list_remove(lista_t_peticiones, 0);
// 		pthread_mutex_unlock(mutex_lista_t_peticiones);
// 		do{
// 			conexion_kernel_memoria = crear_conexion(args->ip, args->puerto);
// 			sleep(1);

// 		}while(conexion_kernel_memoria == -1);
// 		args_peticion.peticion = peticion;
// 		args_peticion.socket = conexion_kernel_memoria; 
// 		pthread_create(&aux_thread, NULL, peticion_kernel, (void *)&args_peticion);
// 		pthread_detach(aux_thread);
// 	}
//     pthread_exit(EXIT_SUCCESS);
// }

// void *peticion_kernel(void *args) {
//     t_paquete_peticion *args_peticion = args;
//     int socket = args_peticion->socket;
//     t_peticion *peticion = args_peticion->peticion;
//     t_pcb *proceso = peticion->proceso;
//     t_tcb *hilo = peticion->hilo;
//     t_paquete *send_protocolo;
//     protocolo_socket op;
//     switch (peticion->tipo) {
//         case PROCESS_CREATE_OP:
//             send_protocolo = crear_paquete(PROCESS_CREATE_OP);
//             agregar_a_paquete(send_protocolo, &proceso->pid, sizeof(proceso->pid));
//             agregar_a_paquete(send_protocolo, &proceso->memoria_necesaria, sizeof(proceso->memoria_necesaria));
//             agregar_a_paquete(send_protocolo, &proceso->estado, sizeof(proceso->estado));
// 			log_info(logger, "Se envió la peticion de PROCESS CREATE del PID: %d Tamaño: %d", proceso->pid, proceso->memoria_necesaria);
//             enviar_paquete(send_protocolo, socket);
//             op = recibir_operacion(socket);
//             switch (op) {
//                 case SUCCESS:
//                     log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;

//                 case ERROR:
//                     log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = false;
//                     break;

//                 case OK:
//                     log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;	

//                 default:
//                     log_info(logger, "Código de operación desconocido recibido: %d", op);
//                     peticion->respuesta_exitosa = false;
//                     break;
//             }
//             break;

//         case PROCESS_EXIT_OP:
//             send_protocolo = crear_paquete(PROCESS_EXIT_OP);
//             agregar_a_paquete(send_protocolo, &proceso->pid, sizeof(int));
// 			log_info(logger, "Se envió la peticion de PROCESS EXIT del PID: %d", proceso->pid);
// 			//sem_post(sem_proceso_finalizado);
//             enviar_paquete(send_protocolo, socket);
//             op = recibir_operacion(socket);
//             switch (op) {
//                 case SUCCESS:
//                     log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;

//                 case ERROR:
//                     log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = false;
//                     break;

//                 case OK:
//                     log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;	

//                 default:
//                     log_info(logger, "Código de operación desconocido recibido: %d", op);
//                     peticion->respuesta_exitosa = false;
//                     break;
//             }
//             break;

//         case THREAD_CREATE_OP:
//             send_protocolo = crear_paquete(THREAD_CREATE_OP);
//             agregar_a_paquete(send_protocolo, &hilo->tid, sizeof(hilo->tid));
//             agregar_a_paquete(send_protocolo, &hilo->pid, sizeof(hilo->pid));
//             agregar_a_paquete(send_protocolo, &hilo->prioridad, sizeof(hilo->prioridad));
//             agregar_a_paquete(send_protocolo, &hilo->estado, sizeof(hilo->estado));
//             agregar_a_paquete(send_protocolo, &hilo->quantum_restante, sizeof(hilo->quantum_restante));
//             t_list_iterator * iterator = list_iterator_create(hilo->instrucciones);
//             char *aux_instruccion;
//             while(list_iterator_has_next(iterator)){
//                 aux_instruccion = list_iterator_next(iterator);
//                 int tamanio2 = strlen(aux_instruccion)+1;
//                 agregar_a_paquete(send_protocolo, aux_instruccion, tamanio2);
//             }
// 			log_info(logger, "Se envió la peticion de THREAD CREATE");
//             enviar_paquete(send_protocolo, socket);
//             op = recibir_operacion(socket);
//             switch (op) {
//                 case SUCCESS:
//                     log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;

//                 case ERROR:
//                     log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = false;
//                     break;

//                 case OK:
//                     log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;	

//                 default:
//                     log_info(logger, "Código de operación desconocido recibido: %d", op);
//                     peticion->respuesta_exitosa = false;
//                     break;
//             }
//             break;

//         case THREAD_EXIT_OP:
//             send_protocolo = crear_paquete(THREAD_EXIT_OP);
//             agregar_a_paquete(send_protocolo, &hilo->tid, sizeof(hilo->tid));
//             agregar_a_paquete(send_protocolo, &hilo->pid, sizeof(hilo->tid));
// 			log_info(logger, "Se envió la peticion de THREAD EXIT");
//             enviar_paquete(send_protocolo, socket);
//             op = recibir_operacion(socket);
//             switch (op) {
//                 case SUCCESS:
//                     log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;

//                 case ERROR:
//                     log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = false;
//                     break;

//                 case OK:
//                     log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;	

//                 default:
//                     log_info(logger, "Código de operación desconocido recibido: %d", op);
//                     peticion->respuesta_exitosa = false;
//                     break;
//             }
//             break;

// 		case THREAD_CANCEL_OP:
//             send_protocolo = crear_paquete(THREAD_CANCEL_OP);
//             agregar_a_paquete(send_protocolo, &hilo->tid, sizeof(hilo->tid));
//             agregar_a_paquete(send_protocolo, &hilo->pid, sizeof(hilo->tid));
// 			log_info(logger, "Se crea la peticion de THREAD CANCEL");
//             enviar_paquete(send_protocolo, socket);
//             op = recibir_operacion(socket);
//             switch (op) {
//                 case SUCCESS:
//                     log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;

//                 case ERROR:
//                     log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = false;
//                     break;

//                 case OK:
//                     log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;	

//                 default:
//                     log_info(logger, "Código de operación desconocido recibido: %d", op);
//                     peticion->respuesta_exitosa = false;
//                     break;
//             }
//             break;

//         case DUMP_MEMORY_OP:
//             send_protocolo = crear_paquete(DUMP_MEMORY_OP);
//             agregar_a_paquete(send_protocolo, &hilo_actual->tid, sizeof(int));
// 			agregar_a_paquete(send_protocolo, &proceso_actual->pid, sizeof(int));
// 			log_info(logger, "Se envió la peticion de DUMP MEMORY");
           
//             enviar_paquete(send_protocolo, socket);
//             op = recibir_operacion(socket);
//             switch (op) {
//                 case SUCCESS:
//                     log_info(logger, "'SUCCESS' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;

//                 case ERROR:
//                     log_info(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = false;
//                     break;

//                 case OK:
//                     log_info(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
//                     peticion->respuesta_exitosa = true;
//                     break;	

//                 default:
//                     log_info(logger, "Código de operación desconocido recibido: %d", op);
//                     peticion->respuesta_exitosa = false;
//                     break;
//             }
//             break;
//         default:
//             log_info(logger, "Tipo de operación desconocido: %d", peticion->tipo);
//             return NULL;
//     }

    
// 	sem_post(sem_estado_respuesta_desde_memoria);
//     eliminar_paquete(send_protocolo);
//     liberar_conexion(socket);

//     return NULL;
// }