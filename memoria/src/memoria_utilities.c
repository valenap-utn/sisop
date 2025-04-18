#include <memoria_utilities.h>

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;
char * puerto_cpu;
int socket_cliente_cpu;

void inicializarMemoria(){
    
    config = config_create("./memoria.config");
    levantarConfig();
    logger = log_create("memoria.log", "Memoria", 1, current_log_level);
    
    pthread_t tid_cpu;

    pthread_create(&tid_cpu, NULL, conexion_server_cpu, NULL);
    
    pthread_join(tid_cpu, NULL);
}
void levantarConfig(){
    puerto_cpu = config_get_string_value(config, "PUERTO_ESCUCHA_CPU");
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    printf("%d", current_log_level);

}

void *conexion_server_cpu(void *args){
    int server_cpu = iniciar_servidor(puerto_cpu);
    log_info(logger, "Servidor listo para recibir al cliente CPU");
    socket_cliente_cpu = esperar_cliente(server_cpu);
   	close(server_cpu);
	close(socket_cliente_cpu);
    pthread_exit(EXIT_SUCCESS);
}