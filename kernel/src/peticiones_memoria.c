#include <peticiones_memoria.h>

extern list_struct_t * lista_peticiones_memoria_pendientes;
extern char * ip_memoria;
extern char * puerto_memoria;


/// @brief Es necesario enviar una peticion a la lista de peticiones, y esperar a que postee el semaforo interno de la peticion. Despues, leer el bool de respuesta_exitosa. Importante recordar de liberar la peticion al recibir la respuesta
/// @param arg_server 
/// @return 
void *administrador_peticiones_memoria(void* arg_server){
	t_peticion_memoria *peticion;
	t_args_peticion_memoria *args_peticion = malloc(sizeof(t_args_peticion_memoria));
	pthread_t aux_thread;
    int socket_memoria = -1;
	
	while(1){
		sem_wait(lista_peticiones_memoria_pendientes->sem);
		peticion = desencolarPeticionMemoria();
		do{
			socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
			if(socket_memoria==-1){
                sleep(1);
            }
		}while(socket_memoria == -1);
		args_peticion->peticion = peticion;
		args_peticion->socket = socket_memoria; 
		peticion_kernel(args_peticion);
		
	}
    pthread_exit(EXIT_SUCCESS);
}

void peticion_kernel(t_args_peticion_memoria *args_peticion) {

    int socket = args_peticion->socket;
    t_peticion_memoria *peticion = args_peticion->peticion;

    PCB *proceso = peticion->proceso;
    t_paquete *send_protocolo;
    protocolo_socket op;
    switch (peticion->tipo) {
        case PROCESS_CREATE_MEM:
            send_protocolo = crear_paquete(PROCESS_CREATE_MEM);
            agregar_a_paquete(send_protocolo, &proceso->pid, sizeof(proceso->pid));
            agregar_a_paquete(send_protocolo, &proceso->memoria_necesaria, sizeof(proceso->memoria_necesaria));
            agregar_a_paquete(send_protocolo, proceso->path_instrucciones, strlen(proceso->path_instrucciones)+1);
			log_debug(logger, "Se envió la peticion de PROCESS CREATE del PID: %d Tamaño: %d", proceso->pid, proceso->memoria_necesaria);

            log_debug(logger, "Archivo de instrucciones a enviar: %s", proceso->path_instrucciones);

            enviar_paquete(send_protocolo, socket);
            op = recibir_paquete_ok(socket);
            switch (op) {
                case OK:
                    log_debug(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
                    peticion->respuesta_exitosa = true;
                    break;	
                case PROCESS_CREATE_MEM_FAIL:
                    log_debug(logger, "FAIL recibido de memoria para %d", peticion->tipo);
                    peticion->respuesta_exitosa = false;
                    break;
                default:
                    log_error(logger, "Código de operación inesperado recibido: %d", op);
                    peticion->respuesta_exitosa = false;
                    break;
            }
            break;

        //same anterior
        case PROCESS_EXIT_MEM:
            send_protocolo = crear_paquete(PROCESS_EXIT_MEM);
            agregar_a_paquete(send_protocolo, &proceso->pid, sizeof(int));
			log_debug(logger, "Se envió la peticion de PROCESS EXIT del PID: %d", proceso->pid);
			//sem_post(sem_proceso_finalizado);
            enviar_paquete(send_protocolo, socket);
            op = recibir_paquete_ok(socket);
            switch (op) {
                case OK:
                    log_debug(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
                    peticion->respuesta_exitosa = true;
                    break;	

                default:
                    log_error(logger, "Código de operación inesperado recibido: %d", op);
                    peticion->respuesta_exitosa = false;
                    break;
            }
            break;

        //DUMP_MEM Debe enviar la peticion, pasar el proceso a blocked, y devolverlo a ready (o exit)
        //cuando reciba la respuesta de memoria.

        //mi propuesta: Pasar a blocked desde syscalls.c y desbloquear desde esta peticion
        //para que la syscall no sea bloqueante, termine y corto plazo pueda continuar con el siguiente
        //proceso.
        //Otra solucion es levantar un nuevo thread que se quede esperando la respuesta de la
        //peticion desde la misma syscall.

        //al final lo hicimos en el mismo syscall con un thread
        case DUMP_MEM:
            send_protocolo = crear_paquete(DUMP_MEM);
			agregar_a_paquete(send_protocolo, (void *)&peticion->proceso->pid, sizeof(int));
			log_debug(logger, "Se envió la peticion de DUMP MEMORY");
           
            enviar_paquete(send_protocolo, socket);
            op = recibir_paquete_ok(socket);
            switch (op) {
                case DUMP_MEM_ERROR:
                    log_debug(logger, "'ERROR' recibido desde memoria para operación %d", peticion->tipo);
                    peticion->respuesta_exitosa = false;
                    break;

                case OK:
                    log_debug(logger, "'OK' recibido desde memoria para operación %d", peticion->tipo);
                    peticion->respuesta_exitosa = true;
                    break;

                default:
                    log_error(logger, "Código de operación inesperado recibido: %d", op);
                    peticion->respuesta_exitosa = false;
                    break;
            }
            break;

        case SUSP_MEM:
            send_protocolo = crear_paquete(SUSP_MEM);
            agregar_a_paquete(send_protocolo, (void *)&peticion->proceso->pid, sizeof(int));
            log_debug(logger, "Se envia un proceso a suspend pid: ", peticion->proceso->pid);
            enviar_paquete(send_protocolo, socket);
            peticion->respuesta_exitosa = true;
            break;
            
        case UNSUSPEND_MEM:
            
            send_protocolo = crear_paquete(UNSUSPEND_MEM);
            agregar_a_paquete(send_protocolo, &proceso->pid, sizeof(proceso->pid));

			log_debug(logger, "Se envió la peticion de UNSUSPEND del PID: %d Tamaño: %d", proceso->pid, proceso->memoria_necesaria);

            enviar_paquete(send_protocolo, socket);
            op = recibir_paquete_ok(socket);
            switch (op) {
                case OK:
                    log_debug(logger, "'OK' recibido desde memoria para UNSUSPEND");
                    peticion->respuesta_exitosa = true;
                    break;	
                case UNSUSPEND_MEM_ERROR:
                    log_debug(logger, "FAIL recibido de memoria para UNSUSPEND");
                    peticion->respuesta_exitosa = false;
                    break;
                default:
                    log_error(logger, "Código de operación inesperado recibido: %d", op);
                    peticion->respuesta_exitosa = false;
                    break;
            }

            break;

        default:
            log_error(logger, "En peticion_kernel: Tipo de operación desconocido: %d", peticion->tipo);
            return;
    }

    
	sem_post(peticion->peticion_finalizada);
    eliminar_paquete(send_protocolo);
    liberar_conexion(socket);

    return;
}
void encolarPeticionMemoria(t_peticion_memoria *peticion){
    
    pthread_mutex_lock(lista_peticiones_memoria_pendientes->mutex);
    list_add(lista_peticiones_memoria_pendientes->lista, peticion);
    pthread_mutex_unlock(lista_peticiones_memoria_pendientes->mutex);
    sem_post(lista_peticiones_memoria_pendientes->sem);

    return;
}
t_peticion_memoria * desencolarPeticionMemoria(){
    
    pthread_mutex_lock(lista_peticiones_memoria_pendientes->mutex);
    t_peticion_memoria * peticion = list_remove(lista_peticiones_memoria_pendientes->lista, 0);
    pthread_mutex_unlock(lista_peticiones_memoria_pendientes->mutex);

    return peticion;
}