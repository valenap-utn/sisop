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
    MEMORY_DUMP,
    PEDIR_INSTRUCCIONES
};

enum comu_kernel{
    INICIALIZAR_PROCESO,
    SUSPENDER_PROCESO,
    DESSUPENDER_PROCESO,
    FINALIZAR_PROCESO
};

typedef struct {
    int cant_accesos_tdp; //cantidad de accesos a tabla de paginas
    int cant_instr_sol; //cantidad de instrucciones solicitadas
    int cant_bajadas_swap; //cantidad de bajadas a SWAP
    int cant_subidas_memoria; //cantidad de subidas a MP o al espacio
    int cant_lecturas; //cantidad de lecturas de memoria
    int cant_escrituras; //cantidad de escrituras de memoria
}t_metricas;

typedef struct{
    void* espacio;
    t_list *tabla_paginas;
    t_metricas metricas;
}t_memoria;


char* crear_directorio(char* ruta_a_agregar);

#endif