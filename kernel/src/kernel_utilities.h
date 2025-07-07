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
    PCB *proceso;
    int tiempo;
    char * nombre;
    sem_t * peticion_finalizada;
    bool respuesta_exitosa;
    
}t_peticion_io;

typedef struct{
    int socket;
    char *nombre;
    list_struct_t *cola_blocked;
}t_socket_io;

typedef struct 
{
    PCB *pcb;
    int tiempo;
    
}elemento_cola_blocked_io;

typedef struct
{
    int socket;
    t_peticion_memoria *peticion;

}t_args_peticion_memoria;

void inicializarKernel();
void *server_mh_cpu(void *args);
void inicializarSemaforos();
void inicializarListasKernel();
enum_algoritmo_largoPlazo alg_largoPlazo_from_string(char *string);
enum_algoritmo_cortoPlazo alg_cortoPlazo_from_string(char *string);
PCB *desencolar_generico(list_struct_t * cola, int index);
int cola_new_buscar_smallest();
int cola_fallidos_buscar_smallest();
int buscar_en_cola_por_pid(list_struct_t *cola, int pid_buscado);
void encolar_cola_generico(list_struct_t *cola, PCB *pcb, int index);
t_peticion_memoria *inicializarPeticionMemoria();
t_peticion_io *inicializarPeticionIO();
void liberar_peticion_memoria(t_peticion_memoria *peticion);
void levantarConfig();

#endif