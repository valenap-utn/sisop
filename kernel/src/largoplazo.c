#include <largoplazo.h>

extern enum_algoritmo_largoPlazo algoritmo_largoPlazo;
extern t_log *logger;

extern list_struct_t *lista_procesos_new;
extern list_struct_t *lista_procesos_new_fallidos;
extern list_struct_t *lista_procesos_ready;
extern list_struct_t *lista_procesos_susp_ready;

extern pthread_cond_t * cond_all_start;
extern int flag_all_start;
extern pthread_mutex_t * mutex_all_start;


extern sem_t *sem_memoria_liberada;
extern pthread_cond_t * cond_susp_ready_empty;
extern int susp_ready_empty;
extern pthread_mutex_t * mutex_susp_ready_empty;


void *largoPlazo(void *args){

    esperar_flag_global(&flag_all_start, mutex_all_start, cond_all_start);
    log_debug(logger, "largo plazo arranca");

    pthread_t tid_aux;

    switch(algoritmo_largoPlazo){
        case LPL_FIFO:
            largoPlazoFifo();
            break;

        case LPL_SMALL:
            pthread_create(&tid_aux, NULL, largoPlazoFallidos, NULL);
            pthread_detach(tid_aux);
            largoPlazoSmallFirst();

        //Agregar aca mientras se van haciendo
        
        default:
            log_error(logger, "algoritmo no reconocido en Largo Plazo");

    }
    return (void *)EXIT_SUCCESS;
}

void largoPlazoFifo(){

    //nota, hay que agregar un mutex desde mediano plazo para no enviar peticiones de largo
    //plazo a memoria siempre que hayan elementos esperando en la cola de susp_ready

    while(true){
        sem_wait(lista_procesos_new->sem); // espera a que haya un nuevo elemento en la cola_new
        log_debug(logger, "Nuevo proceso a crear: Intentando cargar en memoria");
        
        //el planificador queda en pausa hasta que susp_ready este vacia
        esperar_prioridad_susp_ready();

        PCB * pcb = desencolar_generico(lista_procesos_new, 0);
        if(!encolarPeticionLargoPlazo(pcb)){
            encolar_cola_generico(lista_procesos_new, pcb, 0);
            sem_wait(sem_memoria_liberada); //espera a que un proceso finalize para volver a intentar enviar peticiones a memoria
            //si el sem_wait le dio prioridad a largo plazo, se le devuelve la ejecucion a medianoplazo
            if (hay_algo_en_susp_ready()){
                log_debug(logger, "LPL: se intento darle prioridad a MP");
                sem_post(sem_memoria_liberada);
                sem_wait(sem_memoria_liberada);
                log_debug(logger, "LPL: se despauso largo plazo luego de dar prioridad a MP");
            }
        }
    }
    return;
}

/// @brief El funcionamiento de este algoritmo es el siguiente:
/// Llega un proceso a new -> se intenta cargar en memoria.
/// Si lo logra, se queda esperando un proceso nuevo en la cola new.
/// Si falla, se agrega el proceso a la cola de fallidos, el cual lo intenta cargar en memoria
/// cada vez que se finalice un proceso.
/// En caso de que haya mas de un proceso en cualquier cola, busca el mas pequeño.
void largoPlazoSmallFirst(){

    //nota, hay que agregar un mutex desde mediano plazo para no enviar peticiones de largo
    //plazo a memoria siempre que hayan elementos esperando en la cola de susp_ready

    int index;
    while(true){
        sem_wait(lista_procesos_new->sem); // espera a que haya un nuevo elemento en la cola_new
        log_debug(logger, "Nuevo proceso a crear: Intentando cargar en memoria");

        //el planificador queda en pausa hasta que susp_ready este vacia
        esperar_prioridad_susp_ready();

        index = cola_new_buscar_smallest();
        if (index == -1){
            log_error(logger, "LPL: lista vacia, preparese para la autodestruccion");
        }
        PCB * pcb = desencolar_generico(lista_procesos_new, index);
        if(!encolarPeticionLargoPlazo(pcb)){
            encolar_cola_generico(lista_procesos_new_fallidos, pcb, 0); // se puede reencolar en cualquier lado
        }
    }
    return;
}
/// @brief Actua cada vez que se finaliza un proceso. Recorre la lista de fallidos y envia
/// el mas pequeño como peticion a memoria. Si no hay procesos fallidos no hace nada.
void * largoPlazoFallidos(void * args){
    int index = -1;
    while(true){

        esperar_prioridad_susp_ready();
        sem_wait(sem_memoria_liberada); // espera a que finalize un proceso
        log_debug(logger, "LPL: Termino un proceso , reintentando cargar en memoria");

        if (hay_algo_en_susp_ready()){
            log_debug(logger, "LPL: se intento darle prioridad a MP");
            sem_post(sem_memoria_liberada);
            sem_wait(sem_memoria_liberada);
            log_debug(logger, "LPL: se despauso largo plazo luego de dar prioridad a MP");
        }
        
        index = cola_fallidos_buscar_smallest();
        if (index == -1){ // si no hay elementos en la cola, se cancela el proceso
            continue;
        }
        PCB * pcb = desencolar_generico(lista_procesos_new_fallidos, index);
        if(!encolarPeticionLargoPlazo(pcb)){
            encolar_cola_generico(lista_procesos_new_fallidos, pcb, 0); // se puede reencolar en cualquier lado
        }
    }
    return (void *)EXIT_SUCCESS;
}
bool encolarPeticionLargoPlazo(PCB *pcb){
    t_peticion_memoria * peticion = inicializarPeticionMemoria();

    peticion->tipo = PROCESS_CREATE_MEM;
    peticion->proceso = pcb;
    encolarPeticionMemoria(peticion);
    sem_wait(peticion->peticion_finalizada);
    if (peticion->respuesta_exitosa){
        log_debug(logger, "Se cargo un nuevo proceso en memoria");
        encolar_cola_generico(lista_procesos_ready, pcb, -1);
        cambiar_estado(pcb, READY);
        //sem post a proceso nuevo encolado, revisar si hace falta
        return true;
    }
    else{
        log_debug(logger, "No se pudo cargar proceso nuevo en memoria");
        return false;
    }
}
/// @brief Se queda esperando que no haya nada en susp_ready
void esperar_prioridad_susp_ready(){
    pthread_mutex_lock(lista_procesos_susp_ready->mutex);
    int lista_vacia = list_is_empty(lista_procesos_susp_ready->lista);
    pthread_mutex_unlock(lista_procesos_susp_ready->mutex);
    if (!lista_vacia){
        esperar_flag_global(&susp_ready_empty, mutex_susp_ready_empty, cond_susp_ready_empty);
        return;
    }
}
/// @brief idem esperar_prioridad_susp_ready pero no espera 
/// @return true si hay algo en la cola
bool hay_algo_en_susp_ready(){
    pthread_mutex_lock(lista_procesos_susp_ready->mutex);
    int lista_vacia = list_is_empty(lista_procesos_susp_ready->lista);
    pthread_mutex_unlock(lista_procesos_susp_ready->mutex);
    if (!lista_vacia){
        return true;
    }else return false;
}