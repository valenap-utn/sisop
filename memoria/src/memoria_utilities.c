#include <memoria_utilities.h>

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;
char * puerto_cpu;
extern list_struct_t *lista_sockets_cpu;

int tam_memoria,tam_pagina,cant_entradas_x_tabla,cant_niveles;
extern char* dump_path;
char* swapfile_path;

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
    while (*socket_nuevo = esperar_cliente(server_cpu)){
        
        // será error del cliente?
        if (*socket_nuevo == -1) {
            log_error(logger, "Fallo al aceptar conexión del cliente CPU");
            close(server_cpu);
            pthread_exit(NULL);
        }
        
        pthread_mutex_lock(lista_sockets_cpu->mutex);
        list_add(lista_sockets_cpu->lista, socket_nuevo);
        pthread_mutex_unlock(lista_sockets_cpu->mutex);

    }
    
    

    close(*socket_nuevo);
    close(server_cpu);
    pthread_exit(NULL);
}

void inicializarListasMemoria(){
    lista_sockets_cpu = inicializarLista();
}


void inicializar_proceso(){
    
}