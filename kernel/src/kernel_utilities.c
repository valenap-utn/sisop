#include <kernel_utilities.h>

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;
extern list_struct_t *lista_sockets_cpu;

void inicializarKernel(){

    config = config_create("./kernel.config");
    levantarConfig();

    logger = log_create("kernel.log", "Kernel", 1, current_log_level);

    inicializarSemaforos();
    inicializarListasKernel();

}
void levantarConfig(){

    
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    printf("%d", current_log_level);

}
/// @brief Thread que espera conexiones de CPU nuevas y las agrega a la lista de sockets. Nunca para de esperar y aceptar nuevos
/// @param args 
/// @return void
void *server_mh_cpu(void *args){
    argumentos_thread *argumentos = args;

    int server = iniciar_servidor(argumentos->puerto);

    int *socket_nuevo = malloc(sizeof(int));

    while(*socket_nuevo = esperar_cliente(server)){
        
        pthread_mutex_lock(lista_sockets_cpu->mutex);
        list_add(lista_sockets_cpu->lista, socket_nuevo);
        pthread_mutex_unlock(lista_sockets_cpu->mutex);

        socket_nuevo = malloc(sizeof(int));

    }

}
void inicializarSemaforos(){
    //vacia por ahora
    return;    
}
void inicializarListasKernel(){
    inicializarLista(lista_sockets_cpu);
}