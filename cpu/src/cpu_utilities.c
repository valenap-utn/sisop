#include <cpu_utilities.h>

extern t_log *logger;
extern t_config *config;

extern list_struct_t * cola_interrupciones;

extern int socket_interrupt, socket_dispatch;
int socket_memoria;
//configs
t_log_level current_log_level;
char * puerto_dispatch;
char * puerto_interrupt;
char * puerto_memoria;
char * ip_kernel;
char * ip_memoria;


/* ------ TLB ------ */
int entradas_tlb;
char* reemplazo_tlb;
TLB_t* TLB_tabla = NULL;


/* ------ CACHÉ ------ */
int entradas_cache;
char * reemplazo_cache;
int retardo_cache;
cache_t* cache_tabla = NULL; 

int puntero_cache;


void inicializarCpu(char *nombreCpuLog){
    char * NewnombreCpuLog = (char*) malloc(strlen(nombreCpuLog)+16);
    sprintf(NewnombreCpuLog,"%s.log", nombreCpuLog); // FIJARSE QUE NO TENGA MEMORY LEAKS

    cola_interrupciones = inicializarLista();

    config = config_create("./cpu.config");
    levantarConfig();

    logger = log_create(NewnombreCpuLog, "CPU", 1, current_log_level);

    pthread_t tid_conexion_kernel;
    pthread_t tid_conexion_memoria;
    pthread_t tid_ciclo_inst;
    
    
    pthread_create(&tid_conexion_memoria, NULL, conexion_cliente_memoria, NULL);
    pthread_join(tid_conexion_memoria, NULL);
    pthread_create(&tid_conexion_kernel, NULL, conexion_cliente_kernel, NULL);
    pthread_join(tid_conexion_kernel, NULL);
    
    pthread_create(&tid_ciclo_inst, NULL, ciclo_instruccion, NULL);
    pthread_join(tid_ciclo_inst, NULL);

    //TLB
    if(entradas_tlb > 0){
        TLB_tabla = malloc(sizeof(TLB_t) * entradas_tlb);
        for(int i = 0; i < entradas_tlb; i++){
            TLB_tabla[i].ocupado = false;
        }
    }

    //CACHÉ
    if(entradas_cache > 0){
        cache_tabla = malloc(sizeof(cache_t) * entradas_cache);
        for(int i = 0; i < entradas_cache; i++){
            cache_tabla[i].ocupado = 0;
            cache_tabla[i].uso = 0;
            cache_tabla[i].modificado = 0;
        }
        puntero_cache = 0;
    }

}

void levantarConfig(){

    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_dispatch = config_get_string_value(config, "PUERTO_KERNEL_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_KERNEL_INTERRUPT");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

    entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
    // reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");

    // entradas_cache = config_get_int_value(config, "ENTRADAS_CACHE");
    // reemplazo_cache = config_get_string_value(config, "REEMPLAZO_CACHE");
    retardo_cache= config_get_int_value(config, "RETARDO_CACHE");

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
    return (void *)EXIT_SUCCESS;
}

void *conexion_cliente_memoria(void *args){
    
	do
	{
		socket_memoria = crear_conexion(ip_memoria, puerto_memoria);
		sleep(1);
        

	}while(socket_memoria == -1);
    log_info(logger, "Se realizó la conexion con MEMORIA");
    return (void *)EXIT_SUCCESS;
}
void encolar_interrupcion_generico(list_struct_t * cola, interrupcion_t * interrupcion, int index){
    pthread_mutex_lock(cola->mutex);
    list_add_in_index(cola->lista, interrupcion, index);
    pthread_mutex_lock(cola->mutex);
}