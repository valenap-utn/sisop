#include <cpu_utilities.h>

extern t_log *logger;
extern t_config *config;

extern int pid;

extern list_struct_t * cola_interrupciones;
extern int flag_hay_interrupcion;

extern sem_t * sem_dispatch;

extern int socket_interrupt, socket_dispatch;
int socket_memoria;
//configs
t_log_level current_log_level;
char * puerto_dispatch;
char * puerto_interrupt;
char * puerto_memoria;
char * ip_kernel;
char * ip_memoria;


/* ------ TLB ------ */
int entradas_tlb;
char* reemplazo_tlb;
TLB_t* TLB_tabla = NULL;


/* ------ CACHÉ ------ */
int entradas_cache;
char * reemplazo_cache;
int retardo_cache;
cache_t* cache; 

//variables para MMU
extern int tam_pag, cant_niv, entradas_x_tabla;

int puntero_cache;


void inicializarCpu(char *nombreCpuLog){
    char * NewnombreCpuLog = (char*) malloc(strlen(nombreCpuLog)+16);
    sprintf(NewnombreCpuLog,"%s.log", nombreCpuLog); // FIJARSE QUE NO TENGA MEMORY LEAKS

    cola_interrupciones = inicializarLista();

    sem_dispatch = inicializarSem(0);

    config = config_create("./cpu.config");
    levantarConfig();

    logger = log_create(NewnombreCpuLog, "CPU", 1, current_log_level);

    pthread_t tid_conexion_kernel;
    pthread_t tid_conexion_memoria;
    pthread_t tid_ciclo_inst;
    pthread_t tid_dispatch_kernel;

    
    pthread_create(&tid_conexion_memoria, NULL, conexion_cliente_memoria, NULL);
    pthread_join(tid_conexion_memoria, NULL);
    recibir_valores_memoria(socket_memoria);

    pthread_create(&tid_conexion_kernel, NULL, conexion_cliente_kernel, NULL);
    pthread_join(tid_conexion_kernel, NULL);

    pthread_create(&tid_dispatch_kernel, NULL, conexion_kernel_dispatch, NULL);
    pthread_create(&tid_ciclo_inst, NULL, ciclo_instruccion, NULL);
    


    //TLB
    if(entradas_tlb > 0){
        TLB_tabla = malloc(sizeof(TLB_t) * entradas_tlb);
        for(int i = 0; i < entradas_tlb; i++){
            TLB_tabla[i].ocupado = false;
        }
    }

    //CACHÉ
    if(entradas_cache > 0){
        cache = malloc(sizeof(cache_t) * entradas_cache);
        for(int i = 0; i < entradas_cache; i++){
            cache[i].ocupado = 0;
            cache[i].uso = 0;
            cache[i].modificado = 0;
        }
        puntero_cache = 0;
    }

    pthread_join(tid_ciclo_inst, NULL);
    pthread_join(tid_dispatch_kernel, NULL);

}

void levantarConfig(){

    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_dispatch = config_get_string_value(config, "PUERTO_KERNEL_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_KERNEL_INTERRUPT");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
    reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");

    entradas_cache = config_get_int_value(config, "ENTRADAS_CACHE");
    reemplazo_cache = config_get_string_value(config, "REEMPLAZO_CACHE");
    retardo_cache= config_get_int_value(config, "RETARDO_CACHE");

}

void *conexion_cliente_kernel(void *args){
    
	do
	{
		socket_dispatch = crear_conexion(ip_kernel, puerto_dispatch);
		sleep(1);
        

	}while(socket_dispatch == -1);
    log_info(logger, "Se realizó la conexion con CPU DISPATCH");
    //semaforo aca?

	do
	{
		socket_interrupt = crear_conexion(ip_kernel, puerto_interrupt);
		sleep(1);
        

	}while(socket_interrupt == -1);
    log_info(logger, "Se realizó la conexion con CPU INTERRUPT");
    //semaforo aca?
    return (void *)EXIT_SUCCESS;
}


void *conexion_kernel_dispatch(void* arg_kernelD)
{
	argumentos_thread * args = arg_kernelD; 
	// t_list *paquete;
	int pid_aux, pc_aux;

	while(true){
		int cod_op = recibir_operacion(socket_dispatch);
		switch (cod_op){
			case DISPATCH_CPU:
                // sem_wait(sem_registros_actualizados);
				log_info(logger, "Recibí un pid para ejecutar de parte de Kernel");
				t_list *paquete = recibir_paquete(socket_dispatch);
				pid_aux = *(int *)list_remove(paquete, 0);
                pc_aux = *(int *)list_remove(paquete, 0);
				log_info(logger, "El pid a ejecutar es: %d", pid_aux);
				list_destroy(paquete);
                interrupcion_t * interrupcion = malloc(sizeof(interrupcion_t));

                interrupcion->tipo = DISPATCH_CPU_I;
                interrupcion->pid = pid_aux;
                interrupcion->pc = pc_aux;

                encolar_interrupcion_generico(cola_interrupciones, interrupcion, 0);

                flag_hay_interrupcion = 1;

                sem_post(sem_dispatch);

				break;
			case -1:
				log_info(logger, "el cliente se desconecto. Terminando servidor");
				return (void *)EXIT_FAILURE;
				break;
			default:
				log_info(logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}
		}
		
	close(socket_dispatch);
    return (void *)EXIT_SUCCESS;
}

void *conexion_cliente_memoria(void *args){
    
	do
	{
		socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
		sleep(1);
        

	}while(socket_memoria == -1);
    log_info(logger, "Se realizó la conexion con MEMORIA");
    return (void *)EXIT_SUCCESS;
}

void encolar_interrupcion_generico(list_struct_t * cola, interrupcion_t * interrupcion, int index){
    pthread_mutex_lock(cola->mutex);
    list_add_in_index(cola->lista, index, interrupcion);
    pthread_mutex_unlock(cola->mutex);
    flag_hay_interrupcion = true;
}
interrupcion_t * desencolar_interrupcion_generico(list_struct_t * cola){
    
    pthread_mutex_lock(cola->mutex);
    if(list_is_empty(cola->lista)){
        return NULL;
    }
    interrupcion_t * interrupcion = list_remove(cola->lista, 0);
    if(list_is_empty(cola->lista)){
        flag_hay_interrupcion = false;
    }
    pthread_mutex_unlock(cola->mutex);

    return interrupcion;
}
void vaciar_cola_interrupcion(list_struct_t * cola){
    pthread_mutex_lock(cola->mutex);
    list_clean_and_destroy_elements(cola->lista, free);
    pthread_mutex_unlock(cola->mutex);
    flag_hay_interrupcion = false;
}
void recibir_valores_memoria(int socket_memoria){
    t_paquete* paquete_send = crear_paquete(ENVIAR_VALORES);
    agregar_a_paquete(paquete_send, "xd", strlen("xd")+1);
    enviar_paquete(paquete_send,socket_memoria);
    eliminar_paquete(paquete_send);

    protocolo_socket cod_op = recibir_operacion(socket_memoria);
    if(cod_op != ENVIAR_VALORES){
        log_error(logger,"Error al recibir valores desde memoria");
        return;
    }

    t_list* paquete_recv = recibir_paquete(socket_memoria);

    int* tam_pagina_ptr = list_remove(paquete_recv,0);
    int* cant_niveles_ptr = list_remove(paquete_recv,0);
    int* entradas_por_tabla_ptr = list_remove(paquete_recv,0);

    tam_pag = *tam_pagina_ptr;
    cant_niv = *cant_niveles_ptr;
    entradas_x_tabla = *entradas_por_tabla_ptr;

    free(tam_pagina_ptr);
    free(cant_niveles_ptr);
    free(entradas_por_tabla_ptr);
    list_destroy(paquete_recv);
}
