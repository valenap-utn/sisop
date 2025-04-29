#include <largoplazo.h>

extern enum_algoritmo_largoPlazo algoritmo_largoPlazo;

void *largoPlazo(void *args){
    switch(algoritmo_largoPlazo){
        case FIFO:
            largoPlazoFifo();
            break;

        //Agregar aca mientras se van haciendo
        
        default:
            log_error(logger, "algoritmo no reconocido en Largo Plazo");

    }
    return (void *)EXIT_SUCCESS;
}

void largoPlazoFifo(){
    while(true); // reemplazar con el codigo

    return;
}