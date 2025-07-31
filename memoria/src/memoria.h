#ifndef MEMORIA_MAIN_
#define MEMORIA_MAIN_

// #include <memoria_utilities.h>
#include <utils/utils.h>
#include <commons/log.h>
#include <commons/config.h>
#include <errno.h>
#include "../../kernel/src/pcb.h"
#include <sys/stat.h>
#include <stdbool.h> //para poder usar variables de tipo 'bool' 
#include <sys/time.h>

void inicializarMemoria();
char* crear_directorioSWAP();

typedef struct t_metricas{
    int cant_accesos_tdp; //cantidad de accesos a tabla de paginas
    int cant_instr_sol; //cantidad de instrucciones solicitadas
    int cant_bajadas_swap; //cantidad de bajadas a SWAP
    int cant_subidas_memoria; //cantidad de subidas a MP o al espacio
    int cant_lecturas; //cantidad de lecturas de memoria
    int cant_escrituras; //cantidad de escrituras de memoria
}t_metricas;

typedef struct t_memoria{
    void* espacio;
    t_list *tablas_por_proceso; //cambio nombre de tabla_paginas a tablas_por_proceso | Lista de t_tabla_proceso*
    bool* bitmap_marcos;       //bitmap de marcos ocupados
    int cantidad_marcos;      //total marcos disponibles
    t_list* metadata_swap;   //lista de t_swap*
}t_memoria;

// Agrego estructura para asociar tablas con procesos
typedef struct t_tabla_proceso{
    int pid;
    struct Tabla_Principal* tabla_principal;
    t_list* instrucciones;
    t_metricas metricas; //metricas por proceso!!!
    int cantidad_paginas;
}t_tabla_proceso;


char* crear_directorio();

/* ------- TDP ------- */

typedef struct Tabla_Principal{
    struct Tabla_Nivel** niveles; 
}Tabla_Principal; //tabla_global

typedef struct Tabla_Nivel{
    int paginas_contenidas;
    int esta_presente; //bool
    int es_ultimo_nivel; //bool
    union{
        struct Tabla_Nivel **sgte_nivel; 
        int marco; //si es ultimo nivel
    };
}Tabla_Nivel;


/* ------- SWAP ------- */

typedef struct t_swap{
    int  pid;
    int pagina_inicio;     //nro. pagina en archivo swap
    int cantidad_paginas; //cant. paginas que se guardaron
}t_swap;


#endif