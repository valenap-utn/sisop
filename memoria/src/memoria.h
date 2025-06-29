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

//COMUNICACION CON KERNEL y CPU
enum comu_cpu{
    ACCEDER_A_TDP,
    DEVOLVER_MARCO, //para cuando se ejecuta ACCEDER_A_TDP (consultar, si es así esto)

    ACCEDER_A_ESPACIO_USUARIO,
    DEVOLVER_VALOR,

    LEER_PAG_COMPLETA,
    DEVOLVER_PAGINA,

    ACTUALIZAR_PAG_COMPLETA,
    MEMORY_DUMP,
    PEDIR_INSTRUCCION,
    // OBTENER_INSTRUCCION,
    DEVOLVER_INSTRUCCION,
}typedef comu_cpu;

enum comu_kernel{
    INICIALIZAR_PROCESO,
    SUSPENDER_PROCESO,
    DESSUPENDER_PROCESO,
    FINALIZAR_PROCESO
}typedef comu_kernel;

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
    t_metricas metricas;
}t_memoria;

// Agrego estructura para asociar tablas con procesos
typedef struct t_tabla_proceso{
    int pid;
    struct Tabla_Principal* tabla_principal;
    t_list* instrucciones;
}t_tabla_proceso;


char* crear_directorio(char* ruta_a_agregar);

/* ------- PROPUESTA by valucha para TDP ------- */

typedef struct Tabla_Principal{
    int nro_pagina;
    struct Tabla_Nivel** niveles; //array de punteros al nivel 2 
}Tabla_Principal; //tabla_global

typedef struct Tabla_Nivel{
    int nro_pagina;
    int esta_presente; //bool
    int es_ultimo_nivel; //bool
    union{
        struct Tabla_Nivel **sgte_nivel; 
        int marco; //si es ultimo nivel
    };
}Tabla_Nivel;





// typedef struct{
//     void* espacio;
//     t_list *tabla_paginas;
//     t_metricas metricas;
// }t_memoria;

// typedef struct {
//     int nivel;
//     t_list *tabla_paginas;
// }t_tabla_paginas;

// typedef struct {
//     int nivel;
//     t_list *tabla;
// }t_tabla_paginas_ultimo_nivel;

// typedef struct {
//     int pid;
//     bool asignado;
//     int direccion;
// }entrada_ultimo_nivel;



// N=3
// c_entradas=5

// 2 | 4 | 3 | 1 | 30

// t_list * tabla_global;

// tabla_nivel_1 = list_get(tabla_global->tabla_paginas, 2);

// tabla_nivel_2 = list_get(tabla_nivel_1->tabla_paginas, 4);

// ...

// t_tabla_paginas_ultimo_nivel * tabla = list_get(

// list_add(tabla->tabla, 0)
// list_add(tabla->tabla, 10)
// list_add(tabla->tabla, 20)

#endif