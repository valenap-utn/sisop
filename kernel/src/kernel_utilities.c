#include <kernel_utilities.h>

extern t_log *logger;
extern t_config *config;

extern list_struct_t *lista_sockets_cpu;
extern list_struct_t *lista_sockets_io;


//configs
t_log_level current_log_level;
char * puerto_dispatch;
char * puerto_interrupt;
char * puerto_io;

void inicializarKernel(){

    config = config_create("./kernel.config");
    levantarConfig();

    logger = log_create("kernel.log", "Kernel", 1, current_log_level);

    inicializarSemaforos();
    inicializarListasKernel();

    pthread_t tid_server_mh_cpu;
    pthread_t tid_server_mh_io;

    pthread_create(&tid_server_mh_cpu, NULL, server_mh_cpu, NULL);
    pthread_create(&tid_server_mh_io, NULL, server_mh_io, NULL);


    // sleep(5);
    // t_list_iterator *iterator = list_iterator_create(lista_sockets_cpu->lista);
    // t_socket_cpu *element;
    // while(list_iterator_has_next(iterator)){
    //     element = list_iterator_next(iterator);

    //     log_debug(logger, "%d", element->interrupt);
        
    // }

    pthread_join(tid_server_mh_cpu, NULL);
    pthread_join(tid_server_mh_io, NULL);


}

void levantarConfig(){

    
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);

    puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    puerto_io = config_get_string_value(config, "PUERTO_ESCUCHA_IO");

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
void *server_mh_io(void *args){

    int server = iniciar_servidor(puerto_io);

    t_socket_io *socket_nuevo = malloc(sizeof(t_socket_io));
    t_list *paquete_recv;

    char *nombre_io;

    protocolo_socket cod_op;

    while(socket_nuevo->socket = esperar_cliente(server)){

        cod_op = recibir_operacion(socket_nuevo->socket);

        if(cod_op != NOMBRE_IO){
            log_error(logger, "Se recibio un protocolo inesperado de IO");
            return (void*)EXIT_FAILURE;
        }

        paquete_recv = recibir_paquete(socket_nuevo->socket);

        nombre_io = list_remove(paquete_recv, 0);
        // socket_nuevo->nombre = nombre_io;
        socket_nuevo->nombre = nombre_io;
        
        pthread_mutex_lock(lista_sockets_io->mutex);
        list_add(lista_sockets_io->lista, socket_nuevo);
        pthread_mutex_unlock(lista_sockets_io->mutex);

        log_debug(logger, socket_nuevo->nombre);

        socket_nuevo = malloc(sizeof(t_socket_io));

    }
}
void inicializarSemaforos(){
    //vacia por ahora
    return;    
}
void inicializarListasKernel(){
    lista_sockets_cpu = inicializarLista();
    lista_sockets_io = inicializarLista();
}