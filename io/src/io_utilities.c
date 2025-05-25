#include <io_utilities.h>

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;

char * ip_kernel;
char * puerto_kernel;

extern char *nombre_modulo_io;
extern int segundos_espera;

int socket_kernel;

void inicializarIo(){

    config = config_create("./io.config");
    levantarConfig();

    logger = log_create("io.log", "IO", 1, current_log_level);

    pthread_t tid_conexion_kernel;

    pthread_create(&tid_conexion_kernel, NULL, conexion_cliente_kernel, NULL);
    pthread_join(tid_conexion_kernel, NULL);


}
void levantarConfig(){

    
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);

    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");

}
void *conexion_cliente_kernel(void *args){
    
	do
	{
		socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
		sleep(1);
        

	}while(socket_kernel == -1);
    log_info(logger, "Se realizó la conexion con KERNEL");
    
    t_paquete *paquete_send = crear_paquete(NOMBRE_IO);
    agregar_a_paquete(paquete_send, nombre_modulo_io, strlen(nombre_modulo_io) + 1);
    enviar_paquete(paquete_send, socket_kernel);
    
    return (void *)EXIT_SUCCESS;
}

void dormir_IO(){
    
    int pid = -1;
    int milisecs = -1;
    protocolo_socket cod_op = -1;

    while(cod_op = recibir_operacion(socket_kernel)){
        if(cod_op != DORMIR_IO){
            log_error(logger, "Error, operacion desconocida");
            return;
        }

        t_list *paquete_recv = recibir_paquete(socket_kernel);

        pid = (int *)list_remove(paquete_recv, 0);
        milisecs = (int *)list_remove(paquete_recv, 0);

        log_info(logger, "## PID <%d> - Inicio de IO - Tiempo:  <%d SEGUNDOS>",pid, milisecs);
        usleep(milisecs * 1000);
        log_info(logger, "Finalización de IO: “## PID: <%d> - Fin de IO”.",pid);

        enviar_paquete_ok(socket_kernel);
    }

    

    
    

}
