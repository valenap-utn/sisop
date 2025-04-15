#include <cpu_utilities.h>

extern t_log *logger;
extern t_config *config;

extern int socket_interrupt, socket_dispatch;

//configs
t_log_level current_log_level;
char * puerto_dispatch;
char * puerto_interrupt;
char * ip_kernel;

void inicializarCpu(){

    config = config_create("./cpu.config");
    levantarConfig();

    logger = log_create("cpu.log", "CPU", 1, current_log_level);

    pthread_t tid_conexion_kernel;

    pthread_create(&tid_conexion_kernel, NULL, conexion_cliente_kernel, NULL);
    pthread_join(tid_conexion_kernel, NULL);

}
void levantarConfig(){

    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    printf("%d", current_log_level);

    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_dispatch = config_get_string_value(config, "PUERTO_KERNEL_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_KERNEL_INTERRUPT");

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
    while(1); //sacar, test only
    return (void *)EXIT_SUCCESS;
}