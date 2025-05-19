#ifndef KERNEL_UTILITIES_
#define KERNEL_UTILITIES_

#include <kernel.h>
#include <utils/utils.h>
#include <largoplazo.h>
#include <pcb.h>


//elemento de la lista de peticiones para Kernel Largo plazo
typedef struct 
{
    protocolo_socket tipo;
    PCB *proceso;
    bool respuesta_recibida;
    bool respuesta_exitosa;
}t_peticion_largoPlazo;

void inicializarKernel();
void *server_mh_cpu(void *args);
void *server_mh_io(void *args);
void inicializarSemaforos();
void inicializarListasKernel();
enum_algoritmo_largoPlazo alg_largoPlazo_from_string(char *string);
enum_algoritmo_cortoPlazo alg_cortoPlazo_from_string(char *string);
bool encolarPeticionLargoPlazo(PCB *pcb);
void encolarPeticionMemoria(t_peticion_largoPlazo *peticion);
PCB *desencolar_cola_new(int index);
void encolar_cola_new(PCB *pcb);
void encolar_cola_ready(PCB *pcb);
void encolar_cola_new_ordenado_smallerFirst(PCB *pcb);
void levantarConfig();

#endif