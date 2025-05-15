#ifndef MEMORIA_MAIN_
#define MEMORIA_MAIN_

#include <memoria_utilities.h>
#include <utils/utils.h>
#include <commons/log.h>
#include <commons/config.h>
#include <errno.h>


//COMUNICACION CON KERNEL y CPU
enum comu_cpu{
    ACCEDER_A_TDP,
    ACCEDER_A_ESPACIO_USUARIO,
    LEER_PAG_COMPLETA,
    ACTUALIZAR_PAG_COMPLETA,
    MEMORY_DUMP
};

enum comu_kernel{
    INICIALIZAR_PROCESO,
    SUSPENDER_PROCESO,
    DESSUPENDER_PROCESO,
    FINALIZAR_PROCESO
};

// typedef struct{
    
// }t_memoria;

typedef struct t_tabla_nivel {
    struct t_entrada_tabla** entradas; // Arreglo de punteros a entradas
} t_tabla_nivel;

typedef struct t_entrada_tabla {
    bool presente;
    bool es_utlimo_nivel;
    t_tabla_nivel* siguiente_nivel; // Si no es el último nivel
    uint32_t marco_fisico;       // Si es el último nivel
} t_entrada_tabla;

typedef struct {
    int pid;
    t_tabla_nivel* tabla_raiz; // Apunta al primer nivel
} t_proceso;

typedef struct {
    bool presencia;
    bool uso;
    bool modificado;
    uint32_t marco; // Solo válido si es tabla de último nivel
} t_entrada_pagina;

typedef struct t_tabla_pagina {
    bool es_nivel_final; // True si apunta a marcos
    union {
        struct t_tabla_pagina** sub_tablas; // para niveles intermedios
        t_entrada_pagina* entradas;         // para nivel final
    };
} t_tabla_pagina;



typedef struct {
    int cant_accesos_tdp; //cantidad de accesos a tabla de paginas
    int cant_instr_sol; //cantidad de instrucciones solicitadas
    int cant_bajadas_swap; //cantidad de bajadas a SWAP
    int cant_subidas_memoria; //cantidad de subidas a MP o al espacio
    int cant_lecturas; //cantidad de lecturas de memoria
    int cant_escrituras; //cantidad de escrituras de memoria
}t_metricas;

char* crear_directorio(char* ruta_a_agregar);

#endif