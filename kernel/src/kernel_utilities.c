#include <kernel_utilities.h>
#include <pcb.h>

extern t_log *logger;
extern t_config *config;

extern list_struct_t *lista_sockets_cpu_libres;
extern list_struct_t *lista_sockets_cpu_ocupados;
extern list_struct_t *lista_sockets_io;

//colas_planificadores
extern list_struct_t *lista_procesos_new;
extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_procesos_exec;

//semaforos auxiliares
sem_t * sem_proceso_fin;
sem_t * sem_respuesta_memoria;

//configs
t_log_level current_log_level;
char * puerto_dispatch;
char * puerto_interrupt;
char * puerto_io;
enum_algoritmo_largoPlazo algoritmo_largoPlazo;

enum_algoritmo_cortoPlazo algoritmo_cortoPlazo;

void inicializarKernel(){

    config = config_create("./kernel.config");
    logger = log_create("kernel.log", "Kernel", 1, current_log_level);

    levantarConfig();

    inicializarSemaforos();
    inicializarListasKernel();

    pthread_t tid_server_mh_cpu;
    pthread_t tid_server_mh_io;
    pthread_t tid_largoplazo, tid_cortoplazo;
    
    pthread_create(&tid_server_mh_cpu, NULL, server_mh_cpu, NULL);
    pthread_create(&tid_server_mh_io, NULL, server_mh_io, NULL);
    
    //Al iniciar el proceso Kernel, el algoritmo de Largo Plazo debe estar frenado (estado STOP) y se deberá esperar un ingreso de un Enter por teclado para poder iniciar con la planificación.
    //me imagino que hay que leer teclado aca en main, y arrancar la siguiente linea cuando se presione
    pthread_create(&tid_largoplazo, NULL, largoPlazo, NULL);
    pthread_create(&tid_largoplazo, NULL, cortoPlazo, NULL);
    

    pthread_join(tid_server_mh_cpu, NULL);
    pthread_join(tid_server_mh_io, NULL);
    pthread_join(tid_largoplazo, NULL);
    pthread_join(tid_cortoplazo, NULL);


}

void levantarConfig(){

    
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);

    puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    puerto_io = config_get_string_value(config, "PUERTO_ESCUCHA_IO");
    char * alg_largoplazo_temp;
    alg_largoplazo_temp = config_get_string_value(config, "ALGORITMO_COLA_NEW");
    algoritmo_largoPlazo = alg_largoPlazo_from_string(alg_largoplazo_temp);
    char * alg_cortoplazo_temp;
    alg_cortoplazo_temp = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    algoritmo_cortoPlazo = alg_cortoPlazo_from_string(alg_cortoplazo_temp);
}
/// @brief Thread que espera conexiones de CPU nuevas y las agrega a la lista de sockets. Nunca para de esperar y aceptar nuevos
/// @param args 
/// @return void
void *server_mh_cpu(void *args){

    int server_dispatch = iniciar_servidor(puerto_dispatch);
    int server_interrupt = iniciar_servidor(puerto_interrupt);

    t_socket_cpu *socket_nuevo = malloc(sizeof(t_socket_cpu));

    while((socket_nuevo->dispatch = esperar_cliente(server_dispatch))){
        
        socket_nuevo->interrupt = esperar_cliente(server_interrupt);

        pthread_mutex_lock(lista_sockets_cpu_libres->mutex);
        list_add(lista_sockets_cpu_libres->lista, socket_nuevo);
        pthread_mutex_unlock(lista_sockets_cpu_libres->mutex);

        socket_nuevo = malloc(sizeof(t_socket_cpu));

    }
    
    return (void *)EXIT_SUCCESS;
}
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
void inicializarSemaforos(){
    sem_proceso_fin = inicializarSem(0);
    sem_respuesta_memoria = inicializarSem(0);
    return;    
}
void inicializarListasKernel(){
    lista_sockets_cpu_libres = inicializarLista();
    lista_sockets_cpu_ocupados = inicializarLista();
    lista_sockets_io = inicializarLista();
    lista_procesos_new = inicializarLista();
    lista_procesos_ready = inicializarLista();
    lista_procesos_exec = inicializarLista();

}
enum_algoritmo_largoPlazo alg_largoPlazo_from_string(char * string){
    if(!strcmp(string, "FIFO")){
        return LPL_FIFO;
    }
    //agregar mas elseif aca mientras se van creando
    log_error(logger, "Config de largo plazo no reconocido");
    return -1;
}
enum_algoritmo_cortoPlazo alg_cortoPlazo_from_string(char * string){
    if(!strcmp(string, "FIFO")){
        return CPL_FIFO;
    }
    //agregar mas elseif aca mientras se van creando
    log_error(logger, "Config de corto plazo no reconocido");
    return -1;
}
bool encolarPeticionLargoPlazo(PCB *pcb){
    t_peticion_largoPlazo * peticion = malloc(sizeof(t_peticion_largoPlazo));

    peticion->tipo = PROCESS_CREATE_MEM;
    peticion->proceso = pcb;
    encolarPeticionMemoria(peticion);
    sem_wait(sem_respuesta_memoria);
    if (peticion->respuesta_exitosa){
        log_debug(logger, "Se cargo un nuevo proceso en memoria");
        encolar_cola_ready(pcb);
        //sem post a proceso nuevo encolado, revisar si hace falta
        return true;
    }
    else{
        log_debug(logger, "No se pudo cargar proceso nuevo en memoria");
        return false;
    }
}
void encolarPeticionMemoria(t_peticion_largoPlazo *peticion){
    //codigo
    //sem post a lista de peticiones para memoria
    return;
}
/// @brief desencola de lista_procesos_new con el index indicado (0 para FIFO)
/// @param index 
/// @return el PCB de la posicion index
PCB *desencolar_cola_new(int index){
    pthread_mutex_lock(lista_procesos_new->mutex);
    PCB *pcb = list_remove(lista_procesos_new->lista, index);
    pthread_mutex_unlock(lista_procesos_new->mutex);
    return pcb;
}
/// @brief Encola de lista_procesos_new
void encolar_cola_new(PCB *pcb){
    pthread_mutex_lock(lista_procesos_new->mutex);
    list_add_in_index(lista_procesos_new->lista, 0, pcb);
    pthread_mutex_unlock(lista_procesos_new->mutex);
    sem_post(lista_procesos_new->sem);
    return;
}
void encolar_cola_ready(PCB *pcb){
    pthread_mutex_lock(lista_procesos_ready->mutex);
    list_add_in_index(lista_procesos_ready->lista, -1, pcb);
    pthread_mutex_unlock(lista_procesos_ready->mutex);
    // sem_post(lista_procesos_ready->sem); por ahora creo que no hace falta
    return;
}
void encolar_cola_new_ordenado_smallerFirst(PCB * pcb){
    int index = 0;
    PCB *pcb_aux;
    
    t_list_iterator *iterator = list_iterator_create(lista_procesos_new->lista);
    while(list_iterator_has_next(iterator)){
        pcb_aux = list_iterator_next(iterator);
        index = list_iterator_index(iterator);
        if (pcb->memoria_necesaria < pcb_aux->memoria_necesaria){
            pthread_mutex_lock(lista_procesos_new->mutex);
            list_add_in_index(lista_procesos_new->lista, index, pcb);
            pthread_mutex_unlock(lista_procesos_new->mutex);
            return;
        }
    }
    return;
}

void inicializarConexiones(void) {
    char* ip_cpu = config_get_string_value(config, "IP_CPU");
    char* puerto_dispatch_cpu = config_get_string_value(config, "PUERTO_CPU_DISPATCH");

    socket_dispatch_cpu = crear_conexion(ip_cpu, puerto_dispatch_cpu);

    if (socket_dispatch_cpu == -1) {
        log_error(logger, "No se pudo conectar a CPU (dispatch)");
        exit(EXIT_FAILURE);
    }

    log_info(logger, "Conexión establecida con CPU (dispatch)");
}
