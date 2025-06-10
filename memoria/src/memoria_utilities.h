#ifndef MEMORIA_UTILITIES_
#define MEMORIA_UTILITIES_

#include <memoria.h>
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"

void inicializarMemoria();
void levantarConfig();
void *conexion_server_cpu(void *args);

void *cpu(void* conexion);


void *kernel(void* conexion);

int hay_espacio_en_mem(int tamanio_proceso);

void inicializarListasMemoria();


//FUNCIONES
int inicializar_proceso(int pid, int tamanio);
void suspender_proceso();
void des_suspender_proceso();
void finalizar_proceso();
void acceder_a_tdp();
void acceder_a_espacio_usuario();
void leer_pagina_completa();
void actualizar_pagina_completa();
void memory_dump();
t_list* cargar_instrucciones_desde_archivo(char* path);
PCB* buscar_proceso_por_pid(int pid);
int cargar_archivo(int pid,PCB* proceso);
#endif