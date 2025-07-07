#ifndef KERNEL_UTILITIES_
#define KERNEL_UTILITIES_

#include <kernel.h>
#include <utils/utils.h>
#include <largoplazo.h>
#include <cortoplazo.h>
#include <pcb.h>


//elemento de la lista de peticiones para Kernel Largo plazo
typedef struct 
{
    protocolo_socket tipo;
    PCB *proceso;
    sem_t * peticion_finalizada;
    bool respuesta_exitosa;
    
}t_peticion_memoria;

typedef struct
{
    int socket;
    t_peticion_memoria *peticion;

}t_args_peticion_memoria;

void inicializarKernel();
void *server_mh_cpu(void *args);
void *server_mh_io(void *args);
void inicializarSemaforos();
void inicializarListasKernel();
enum_algoritmo_largoPlazo alg_largoPlazo_from_string(char *string);
enum_algoritmo_cortoPlazo alg_cortoPlazo_from_string(char *string);
bool encolarPeticionLargoPlazo(PCB *pcb);
void encolarPeticionMemoria(t_peticion_memoria *peticion);
PCB *desencolar_cola_new(int index);
PCB *desencolar_cola_fallidos(int index);
int cola_new_buscar_smallest();
int cola_fallidos_buscar_smallest();
void encolar_cola_new(PCB *pcb, int index);
void encolar_cola_new_fallidos(PCB *pcb, int index);
void encolar_cola_ready(PCB *pcb);
t_peticion_memoria *inicializarPeticionMemoria();
void liberar_peticionLargoPlazo(t_peticion_memoria *peticion);
void esperar_flag_global();
void destrabar_flag_global(int *flag);
void levantarConfig();

#endif