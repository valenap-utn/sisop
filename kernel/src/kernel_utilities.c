#include <kernel_utilities.h>

extern t_log *logger;
extern t_config *config;

extern list_struct_t *lista_sockets_cpu;

//configs
t_log_level current_log_level;
char * puerto_dispatch;
char * puerto_interrupt;

void inicializarKernel(){

    config = config_create("./kernel.config");
    levantarConfig();

    logger = log_create("kernel.log", "Kernel", 1, current_log_level);

    inicializarSemaforos();
    inicializarListasKernel();

    pthread_t tid_server_mh;

    pthread_create(&tid_server_mh, NULL, server_mh_cpu, NULL);
    pthread_join(tid_server_mh, NULL);


}
void levantarConfig(){

    
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    printf("%d", current_log_level);

    puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");

}
/// @brief Thread que espera conexiones de CPU nuevas y las agrega a la lista de sockets. Nunca para de esperar y aceptar nuevos
/// @param args 
/// @return void
void *server_mh_cpu(void *args){

    int server_dispatch = iniciar_servidor(puerto_dispatch);
    int server_interrupt = iniciar_servidor(puerto_interrupt);

    t_socket_cpu *socket_nuevo = malloc(sizeof(t_socket_cpu));

    while(socket_nuevo->dispatch = esperar_cliente(server_dispatch)){
        
        socket_nuevo->interrupt = esperar_cliente(server_interrupt);

        pthread_mutex_lock(lista_sockets_cpu->mutex);
        list_add(lista_sockets_cpu->lista, socket_nuevo);
        pthread_mutex_unlock(lista_sockets_cpu->mutex);

        socket_nuevo = malloc(sizeof(t_socket_cpu));

    }
}
void inicializarSemaforos(){
    //vacia por ahora
    return;    
}
void inicializarListasKernel(){
    lista_sockets_cpu = inicializarLista();
}