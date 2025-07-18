#include <kernel_utilities.h>


extern t_log *logger;
extern t_config *config;

int pid_actual;

extern int estimacion_inicial;
extern double alfa;

extern list_struct_t *lista_sockets_cpu;
extern list_struct_t *lista_sockets_io;
extern list_struct_t *lista_peticiones_memoria_pendientes;
extern list_struct_t *lista_peticiones_io_pendientes;

//colas_planificadores
extern list_struct_t *lista_procesos_new;
extern list_struct_t *lista_procesos_new_fallidos;
extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_procesos_exec;
extern list_struct_t *lista_procesos_block;
extern list_struct_t *lista_procesos_susp_ready;
extern list_struct_t *lista_procesos_susp_block;

extern t_dictionary *diccionario_cpu_pcb;

//semaforos auxiliares
sem_t * sem_memoria_liberada;
sem_t * sem_desencolado;

//condiciones globales
pthread_cond_t * cond_susp_ready_empty;
int susp_ready_empty;
pthread_mutex_t * mutex_susp_ready_empty;

extern pthread_cond_t * cond_all_start;
extern int flag_all_start;
extern pthread_mutex_t * mutex_all_start;
//

extern pthread_mutex_t * mutex_pid_mayor;

//configs
t_log_level current_log_level;
char * puerto_dispatch;
char * puerto_interrupt;
char * puerto_io;
char * puerto_memoria;
char * ip_memoria;
int tiempo_suspension;
enum_algoritmo_largoPlazo algoritmo_largoPlazo;

enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;

void inicializarKernel(){

    config = config_create("./kernel.config");

    logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_INFO);

    levantarConfig();

    log_destroy(logger);
    logger = log_create("kernel.log", "Kernel", 1, current_log_level);

    inicializarSemaforos();
    inicializarListasKernel();

    diccionario_cpu_pcb = dictionary_create();

}

void levantarConfig(){

    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);

    puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    puerto_io = config_get_string_value(config, "PUERTO_ESCUCHA_IO");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char * alg_largoplazo_temp;
    alg_largoplazo_temp = config_get_string_value(config, "ALGORITMO_INGRESO_A_READY");
    algoritmo_largoPlazo = alg_largoPlazo_from_string(alg_largoplazo_temp);
    char * alg_cortoplazo_temp;
    alg_cortoplazo_temp = config_get_string_value(config, "ALGORITMO_CORTO_PLAZO");
    algoritmo_cortoPlazo = alg_cortoPlazo_from_string(alg_cortoplazo_temp);

    tiempo_suspension = config_get_int_value(config, "TIEMPO_SUSPENSION");

    estimacion_inicial = config_get_int_value(config, "ESTIMACION_INICIAL");
    alfa = config_get_double_value(config, "ALFA");
}
/// @brief Thread que espera conexiones de CPU nuevas y las agrega a la lista de sockets. Nunca para de esperar y aceptar nuevos
/// @param args 
/// @return void
void *server_mh_cpu(void *args){

    int server_dispatch = iniciar_servidor(puerto_dispatch);
    int server_interrupt = iniciar_servidor(puerto_interrupt);

    pthread_t tid_nuevo_cortoplazo;

    t_socket_cpu *socket_nuevo = malloc(sizeof(t_socket_cpu));

    while((socket_nuevo->dispatch = esperar_cliente(server_dispatch))){
        
        socket_nuevo->interrupt = esperar_cliente(server_interrupt);

        pthread_mutex_lock(lista_sockets_cpu->mutex);
        list_add(lista_sockets_cpu->lista, socket_nuevo);
        pthread_mutex_unlock(lista_sockets_cpu->mutex);

        //creamos un nuevo cortoplazo para cada CPU que se conecte
        pthread_create(&tid_nuevo_cortoplazo, NULL, cortoPlazo, (void*)socket_nuevo);

        socket_nuevo = malloc(sizeof(t_socket_cpu));

    }
    pthread_join(tid_nuevo_cortoplazo, NULL);
    return (void *)EXIT_SUCCESS;
}

void inicializarSemaforos(){
    sem_memoria_liberada = inicializarSem(0);

    cond_all_start = inicializarCond();
    mutex_all_start = inicializarMutex();

    cond_susp_ready_empty = inicializarCond();
    mutex_susp_ready_empty = inicializarMutex();

    mutex_pid_mayor = inicializarMutex();

    sem_desencolado = inicializarSem(0);


    return;    
}
void inicializarListasKernel(){
    lista_sockets_cpu = inicializarLista();
    lista_sockets_io = inicializarLista();
    lista_procesos_new = inicializarLista();
    lista_procesos_new_fallidos = inicializarLista();
    lista_procesos_ready = inicializarLista();
    lista_procesos_exec = inicializarLista();
    lista_procesos_block = inicializarLista();
    lista_procesos_susp_ready = inicializarLista();
    lista_procesos_susp_block = inicializarLista();
    lista_peticiones_memoria_pendientes = inicializarLista();

}
enum_algoritmo_largoPlazo alg_largoPlazo_from_string(char * string){
    if(!strcmp(string, "FIFO")){
        return LPL_FIFO;
    }
    if(!strcmp(string, "PMCP")){
        return LPL_SMALL;
    }
    //agregar mas elseif aca mientras se van creando
    log_error(logger, "Config de largo plazo no reconocido");
    return -1;
}
enum_algoritmo_cortoPlazo alg_cortoPlazo_from_string(char * string){
    if(!strcmp(string, "FIFO")){
        return CPL_FIFO;
    }
    if (!strcmp(string, "SJF")) {
        return CPL_SJF;
    }
     if (!strcmp(string, "SRT")) {
        return CPL_SJF_CD;
    }
    log_error(logger, "Config de corto plazo no reconocido");
    return -1;
}


/// @brief desencola de lista_procesos_new con el index indicado (0 para FIFO)
/// @param index 
/// @return el PCB de la posicion index
PCB *desencolar_generico(list_struct_t *cola, int index){
    pthread_mutex_lock(cola->mutex);
    PCB *pcb = list_remove(cola->lista, index);
    pthread_mutex_unlock(cola->mutex);
    return pcb;
}
/// @brief Encola de lista_procesos_new
/// @param pcb 
/// @param index 0 para inicio de lista, -1 para final
void encolar_cola_generico(list_struct_t *cola, PCB *pcb, int index){
    pthread_mutex_lock(cola->mutex);
    list_add_in_index(cola->lista, index, pcb);
    pthread_mutex_unlock(cola->mutex);
    sem_post(cola->sem);
    return;
}
int cola_new_buscar_smallest(){
    pthread_mutex_lock(lista_procesos_new->mutex);
    if (list_is_empty(lista_procesos_new->lista)){
        return -1;
    }
    t_list_iterator * iterator = list_iterator_create(lista_procesos_new->lista);
    PCB *pcb;
    PCB *pcb_smallest = (PCB*)list_iterator_next(iterator);
    int index = 0;
    while (list_iterator_has_next(iterator)){
        pcb = (PCB*)list_iterator_next(iterator);
        if (pcb->memoria_necesaria <= pcb_smallest->memoria_necesaria){
            pcb_smallest = pcb;
            index = list_iterator_index(iterator);
        }
    }
    pthread_mutex_unlock(lista_procesos_new->mutex);

    return index;
}
int cola_fallidos_buscar_smallest(){
    pthread_mutex_lock(lista_procesos_new_fallidos->mutex);
    if (list_is_empty(lista_procesos_new_fallidos->lista)){
        pthread_mutex_unlock(lista_procesos_new_fallidos->mutex);
        return -1;
    }
    t_list_iterator * iterator = list_iterator_create(lista_procesos_new_fallidos->lista);
    PCB *pcb;
    PCB *pcb_smallest = (PCB*)list_iterator_next(iterator);
    int index = 0;
    while (list_iterator_has_next(iterator)){
        pcb = (PCB*)list_iterator_next(iterator);
        if (pcb->memoria_necesaria <= pcb_smallest->memoria_necesaria){
            pcb_smallest = pcb;
            index = list_iterator_index(iterator);
        }
    }
    pthread_mutex_unlock(lista_procesos_new->mutex);

    return index;
}
/// @param pid 
/// @param cola -> list_struct_t generico
/// @return index
int buscar_en_cola_por_pid(list_struct_t * cola, int pid_buscado){

    pthread_mutex_lock(cola->mutex);
    if (list_is_empty(cola->lista)){
        return -1;
    }
    t_list_iterator * iterator = list_iterator_create(cola->lista);
    PCB *pcb;
    int index = 0;
    while (list_iterator_has_next(iterator)){
        pcb = (PCB*)list_iterator_next(iterator);
        if (pcb->pid == pid_buscado){
            index = list_iterator_index(iterator);
            break;
        }
    }
    pthread_mutex_unlock(cola->mutex);

    return index;
}
t_peticion_memoria * inicializarPeticionMemoria(){
    t_peticion_memoria * peticion = malloc(sizeof(t_peticion_memoria));
    peticion->peticion_finalizada = inicializarSem(0);

    return peticion;
}

t_peticion_io * inicializarPeticionIO(){
    t_peticion_io * peticion = malloc(sizeof(t_peticion_io));
    peticion->peticion_finalizada = inicializarSem(0);
    return peticion;
}
void liberar_peticion_memoria(t_peticion_memoria * peticion){
    sem_destroy(peticion->peticion_finalizada);
    free(peticion);
}