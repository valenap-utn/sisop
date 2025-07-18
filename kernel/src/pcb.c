#include <pcb.h>


//variables globales
//variables globales

//A chequear...
int pid_mayor = -1;

//semaforos
pthread_mutex_t * mutex_pid_mayor;
//

PCB* iniciar_pcb(){
    extern int estimacion_inicial;
    
    PCB* pcb = calloc(1, sizeof(PCB));
    pcb->pid = generar_pid_unico();
    //a chequear -> agrego esta linea para que no de warning, para que apunte a memoria valida
    pcb->estimacion_rafaga = estimacion_inicial;
    pcb->rafaga_real_anterior = 0;

    clock_gettime(CLOCK_MONOTONIC, &pcb->timestamp_ultimo_estado);
    cambiar_estado(pcb, NEW);
    return pcb;
}

int generar_pid_unico() {
    pthread_mutex_lock(mutex_pid_mayor);
    pid_mayor +=1;
    pthread_mutex_unlock(mutex_pid_mayor);
    return pid_mayor;
}

/// @brief Calcula y actualiza los valores de las metricas. Hay que usarlo SI O SI cuando cambiamos de estado un pcb
/// aunque el estado ya este reflejado en la lista a la que pertenece.
/// @param pcb 
/// @param estadoNuevo 
void cambiar_estado(PCB *pcb, t_estado estadoNuevo){
    
    struct timespec time_now;
    clock_gettime(CLOCK_MONOTONIC, &time_now);
    long tiempo_en_estado = diff_in_milliseconds(pcb->timestamp_ultimo_estado, time_now);

    if (estadoNuevo != NEW){
        //no hace falta actualizar mt cuando se crea el pcb por primera vez
        pcb->mt[pcb->estado] += tiempo_en_estado;
        log_info(logger, "## (pid: %d) Pasa del estado %d al estado %d", pcb->pid, pcb->estado, estadoNuevo);
    }
    
    pcb->timestamp_ultimo_estado = time_now;
    pcb->estado = estadoNuevo;
    pcb->me[estadoNuevo] +=1;

    
   
}
void pcb_destroy(PCB * pcb){

    free(pcb);
    free(pcb->path_instrucciones);

    return;
}

void loguear_metricas(PCB *pcb){
    
    log_info(
        logger, "## (PID: %d) - Métricas de estado:\n"
        "NEW (count: %d) (time: %ld)\n"
        "READY (count: %d) (time: %ld)\n" 
        "EXEC (count: %d) (time: %ld)\n"
        "BLOCK (count: %d) (time: %ld)\n"
        "SUP_BLOCK (count: %d) (time: %ld)\n"
        "SUP_READY (count: %d) (time: %ld)\n"
        "EXIT (count: %d) (time: %ld)",
        pcb->pid,
        pcb->me[NEW], pcb->mt[NEW],
        pcb->me[READY], pcb->mt[READY],
        pcb->me[EXEC], pcb->mt[EXEC],
        pcb->me[BLOCK], pcb->mt[BLOCK],
        pcb->me[SUP_BLOCK], pcb->mt[SUP_BLOCK],
        pcb->me[SUP_READY], pcb->mt[SUP_READY],
        pcb->me[EXIT], pcb->mt[EXIT]);
}

long diff_in_milliseconds(struct timespec start, struct timespec end) {
    long seconds_diff = end.tv_sec - start.tv_sec;
    long nanoseconds_diff = end.tv_nsec - start.tv_nsec;
    return seconds_diff * 1000 + nanoseconds_diff / 1000000;
}
