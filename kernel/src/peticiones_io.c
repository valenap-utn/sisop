#include <peticiones_io.h>

extern char* puerto_io;
extern list_struct_t * lista_peticiones_io_pendientes;

void *server_mh_io(void *args){

    int server = iniciar_servidor(puerto_io);

    t_socket_io *socket_nuevo = malloc(sizeof(t_socket_io));
    t_list *paquete_recv;

    char *nombre_io;

    protocolo_socket cod_op;

    while((socket_nuevo->socket = esperar_cliente(server))){

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

        log_debug(logger, "%s", socket_nuevo->nombre);

        socket_nuevo = malloc(sizeof(t_socket_io));

    }
    return (void *)EXIT_SUCCESS;
}
/// @brief thread principal que maneja las peticiones a io
void *administrador_peticiones_io(void * args){

    t_peticion_io * peticion;
    pthread_t tid_aux;

	while(1){
		sem_wait(lista_peticiones_io_pendientes->sem);
		peticion = desencolar_peticion_io();
		 
		pthread_create(&tid_aux, NULL, peticion_io, (void *)peticion);
        
        //Aca meti una nueva inicializacion, el anterior no queda flotando porque peticion_kernel tiene su direccion y luego hace un free.
        //El nuevo malloc asegura que no se modificara el contenido antes que peticion_kernel pueda guardarlo en su stack local.
        //Asumo que esto ya es suficiente para salvarnos de la posible condicion de carrera, en caso que haya una nueva peticion cercana.
        peticion = malloc(sizeof(t_args_peticion_memoria));

		pthread_detach(tid_aux);
	}
    pthread_exit(EXIT_SUCCESS);
}
void * peticion_io(void * args){

    t_peticion_io * peticion = args;  

}
void encolar_peticion_io(t_peticion_io * peticion, int index){

}
t_peticion_io * desencolar_peticion_io(){

}