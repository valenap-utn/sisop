#include <largoplazo.h>

extern enum_algoritmo_largoPlazo algoritmo_largoPlazo;
extern t_log *logger;
extern int flag_all_start;

extern list_struct_t *lista_procesos_new;
extern sem_t *sem_proceso_fin;

void *largoPlazo(void *args){

    esperar_flag_global(&flag_all_start);
    log_debug(logger, "largo plazo arranca");

    pthread_t *tid_aux;

    switch(algoritmo_largoPlazo){
        case LPL_FIFO:
            largoPlazoFifo();
            break;

        case LPL_SMALL:
            largoPlazoSmallFirst();
            pthread_create(tid_aux, largoPlazoFallidos, NULL);

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
        PCB * pcb = desencolar_cola_new(0);
        if(!encolarPeticionLargoPlazo(pcb)){
            encolar_cola_new(pcb, -1);
            sem_wait(sem_proceso_fin); //espera a que un proceso finalize para volver a intentar enviar peticiones a memoria
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
        index = cola_new_buscar_smallest();
        if (index == -1){
            log_error("LPL: lista vacia, preparese para la autodestruccion");
        }
        PCB * pcb = desencolar_cola_new(index);
        if(!encolarPeticionLargoPlazo(pcb)){
            encolar_cola_new_fallidos(pcb, 0); // se puede reencolar en cualquier lado
        }
    }
    return;
}
/// @brief Actua cada vez que se finaliza un proceso. Recorre la lista de fallidos y envia
/// el mas pequeño como peticion a memoria. Si no hay procesos fallidos no hace nada.
void * largoPlazoFallidos(void * args){
    int index = -1;
    while(true){
        sem_wait(sem_proceso_fin); // espera a que finalize un proceso
        log_debug(logger, "LPL: Termino un proceso , reintentando cargar en memoria");
        index = cola_fallidos_buscar_smallest();
        if (index == -1){ // si no hay elementos en la cola, se cancela el proceso
            continue;
        }
        PCB * pcb = desencolar_cola_fallidos(index);
        if(!encolarPeticionLargoPlazo(pcb)){
            encolar_cola_new_fallidos(pcb, 0); // se puede reencolar en cualquier lado
        }
    }
    return;
}