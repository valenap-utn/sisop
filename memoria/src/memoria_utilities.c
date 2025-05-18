#include <memoria_utilities.h>



void* espacio_de_usuario;

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;
char * puerto_cpu;
extern list_struct_t *lista_sockets_cpu;

int tam_memoria,tam_pagina,cant_entradas_x_tabla,cant_niveles;
extern char* dump_path;
char* swapfile_path;
char* path_instrucciones;


void inicializarMemoria(){
    
    config = config_create("./memoria.config");
    levantarConfig();
    logger = log_create("memoria.log", "Memoria", 1, current_log_level);
    
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    cant_entradas_x_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
    cant_niveles = config_get_int_value(config,"CANTIDAD_NIVELES");
    swapfile_path = config_get_string_value(config,"PATH_SWAPFILE");
    dump_path = config_get_string_value(config,"DUMP_PATH");
    dump_path = crear_directorio("/dump_files");

    //Creo que esto está bien, no lo habíamos agregado antes
    path_instrucciones = config_get_string_value(config,"PATH_INSTRUCCIONES");
    path_instrucciones = crear_directorio("/scripts");

    espacio_de_usuario = calloc(1,tam_memoria); //RECORDAR HACER UN FREE DE ESTO (no lo puse)
    
    inicializarListasMemoria();

    pthread_t tid_cpu;
    pthread_create(&tid_cpu, NULL, conexion_server_cpu, NULL);
    
    pthread_join(tid_cpu, NULL);
    pthread_join(tid_cpu, NULL);
}

void levantarConfig(){
    puerto_cpu = config_get_string_value(config, "PUERTO_ESCUCHA_CPU");
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    printf("%d", current_log_level);

}

/*
void *conexion_server_cpu(void *args){
    int server_cpu = iniciar_servidor(puerto_cpu);
    log_info(logger, "Servidor listo para recibir al cliente CPU");
    socket_cliente_cpu = esperar_cliente(server_cpu);
   	close(server_cpu);
	close(socket_cliente_cpu);
    pthread_exit(EXIT_SUCCESS);
}
*/

//CONEXION CPU-MEMORIA

void *conexion_server_cpu(void *args) {
    
    int server_cpu = iniciar_servidor(puerto_cpu);

    /* // será error del puerto?
    if (server_cpu == -1) {
        log_error(logger, "Fallo al iniciar el servidor en el puerto %s", puerto_cpu);
        pthread_exit(NULL);
    }
    */
    int *socket_nuevo = malloc(sizeof(int));


    log_info(logger, "Servidor escuchando en el puerto %s, esperando cliente CPU..", puerto_cpu);
    while ((*socket_nuevo = esperar_cliente(server_cpu))){
        
        // será error del cliente?
        if (*socket_nuevo == -1) {
            log_error(logger, "Fallo al aceptar conexión del cliente CPU");
            close(server_cpu);
            pthread_exit(NULL);
        }
        
        pthread_mutex_lock(lista_sockets_cpu->mutex);
        list_add(lista_sockets_cpu->lista, socket_nuevo);
        pthread_mutex_unlock(lista_sockets_cpu->mutex);
        
        //MANEJA LAS INSTRUCCIONES
        // cpu(socket_nuevo);(void*)socket_nuevo
        //pthread_create();
        sleep(0.5);
    }

    close(*socket_nuevo);
    close(server_cpu);
    pthread_exit(NULL);
}


void inicializarListasMemoria(){
    lista_sockets_cpu = inicializarLista();
}


void * cpu(void* args){
    //realizar handshake

    int conexion = *(int *)args;
    comu_cpu peticion;
    t_list *paquete_recv;

    while(1){
        peticion = recibir_operacion(conexion);

        //retardo para peticiones
        usleep(config_get_int_value(config,"RETARDO_MEMORIA")*1000);

        int pid;
        int pc;

        switch(peticion){
            case ACCEDER_A_TDP:
            break;

            case ACCEDER_A_ESPACIO_USUARIO:
            break;

            case LEER_PAG_COMPLETA:
            break;

            case ACTUALIZAR_PAG_COMPLETA:
            break;

            case MEMORY_DUMP:
            break;

            case PEDIR_INSTRUCCIONES:
            break;
            
            //A CHEQUEAR que esté bien
            case OBTENER_INSTRUCCION:

                t_paquete *paquete_send;
                paquete_recv = recibir_paquete(conexion);
                pid = *(int *)list_remove(paquete_recv, 0);
                pc = *(int *)list_remove(paquete_recv, 0);

                PCB* proceso = buscar_proceso_por_pid(pid);
                if (!proceso) {
                    log_error(logger, "PID %d no encontrado al pedir instrucción", pid);
                    break;
                }

                if (pc > list_size(proceso->instrucciones)) {
                    log_error(logger, "Índice %d fuera de rango para PID %d", pc, pid);
                    break;
                }

                char* instruccion = list_get(proceso->instrucciones, pc);
                int len = strlen(instruccion) + 1;

                paquete_send = crear_paquete(DEVOLVER_INSTRUCCION);
                agregar_a_paquete(paquete_send, instruccion, len);

                enviar_paquete(paquete_send, conexion);

                // log_info(logger, "Se envió instrucción [%s] del PID %d - Index %d", instruccion, pid, index);

            break;
        }
    }
}

void kernel(int* conexion){
    //hanshake
    while(1){
        int peticion;
        recv(*conexion,&peticion,sizeof(int),MSG_WAITALL);
        usleep(config_get_int_value(config,"RETARDO_MEMORIA")*1000);
        int pid;

        switch(peticion){
            case INICIALIZAR_PROCESO:

                int tamanio;

                recv(*conexion,&pid,sizeof(int),MSG_WAITALL);
                recv(*conexion,&tamanio,sizeof(int),MSG_WAITALL);

                if(hay_espacio_en_mem(tamanio)) inicializar_proceso(pid,tamanio);
                else log_error(logger,"No se pudo inicializar el proceso por falta de memoria");

            break;

            case SUSPENDER_PROCESO:
            break;

            case DESSUPENDER_PROCESO:
            break;

            case FINALIZAR_PROCESO:
            break;
        }
    }
}


//Hay que ir calculando el tam_memoria disponible en algun lado
int hay_espacio_en_mem(int tamanio_proceso){
    return (tamanio_proceso > tam_memoria) ? 0 : 1;
}

            


//CONEXION KERNEL-MEMORIA


//OTRAS COSAS DE MEMORIA

//FUNCIONES PARA TDP

int pc = 0; //no se si esto ya viene de antes (desde el kernel), creo que si

int inicializar_proceso(int pid, int tamanio){
    PCB* nuevo_proceso = malloc(sizeof(PCB));
    nuevo_proceso->pid = pid;
    nuevo_proceso->PC = pc++;
    nuevo_proceso->me = inicializarLista();
    nuevo_proceso->mt = inicializarLista();
    nuevo_proceso->instrucciones = cargar_instrucciones_desde_archivo(path_instrucciones); 
    
    
    if (nuevo_proceso->instrucciones == NULL) {
        log_error(logger, "Error cargando instrucciones para PID %d", pid);
        free(nuevo_proceso);
        return -1;
    }

    nuevo_proceso->cant_instrucciones = list_size(nuevo_proceso->instrucciones);

    // Agregalo a tu lista global de procesos
    //list_add(procesos_memoria, nuevo_proceso);

    log_info(logger, "Creación de Proceso: ## PID: <%d> - Proceso Creado - Tamaño: <%d>", pid,tamanio);

    return 0; // o retornar "OK" si se espera mensaje
}


//Para cargar instrucciones desde path de config en nuevo_proceso->instrucciones
t_list* cargar_instrucciones_desde_archivo(char* path) {
    FILE* archivo = fopen(path, "r");
    if (!archivo) {
        log_error(logger, "No se pudo abrir el archivo de instrucciones en %s", path);
        return NULL;
    }

    t_list* instrucciones = list_create();
    char* linea = NULL;
    size_t len = 0;

    while (getline(&linea, &len, archivo) != -1) {
        // Eliminar salto de línea si existe
        linea[strcspn(linea, "\n")] = '\0';
        list_add(instrucciones, strdup(linea));
    }

    free(linea);
    fclose(archivo);
    return instrucciones;
}

PCB* buscar_proceso_por_pid(int pid) {
    t_list* procesos_memoria = list_create(); //ESTO ESTA ACA PORQUE FALTARIA LA LISTA DE PROCESOS GLOBAL AHRE
    for (int i = 0; i < list_size(procesos_memoria); i++) {
        PCB* proceso = list_get(procesos_memoria, i);
        if (proceso->pid == pid) {
            return proceso;
        }
    }
    return NULL;  // No se encontró el proceso
}
