#include <memoria_utilities.h>

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;
char * puerto_cpu;
int socket_cliente_cpu;
pthread_t tid_cpu;
argumentos_thread arg_cpu;
void *ret_value;

void inicializarMemoria(){
    
    config = config_create("./memoria.config");
    levantarConfig();
    logger = log_create("memoria.log", "Memoria", 1, current_log_level);
    pthread_create(&tid_cpu, NULL, conexion_server_cpu, (void *)&arg_cpu);
    
    pthread_join(tid_cpu, ret_value);
}
void levantarConfig(){
    puerto_cpu = config_get_string_value(config, "PUERTO_ESCUCHA");
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

    log_info(logger, "Servidor escuchando en el puerto %s, esperando cliente CPU..", puerto_cpu);

    socket_cliente_cpu = esperar_cliente(server_cpu);
    // será error del cliente?
    if (socket_cliente_cpu == -1) {
        log_error(logger, "Fallo al aceptar conexión del cliente CPU");
        close(server_cpu);
        pthread_exit(NULL);
    }

    log_info(logger, "Servidor listo para recibir al cliente CPU", socket_cliente_cpu);

    close(socket_cliente_cpu);
    close(server_cpu);
    pthread_exit(NULL);
}
