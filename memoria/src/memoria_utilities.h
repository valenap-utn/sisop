#ifndef MEMORIA_UTILITIES_
#define MEMORIA_UTILITIES_

#include <memoria.h>
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"

void inicializarMemoria();
void levantarConfig();
void *conexion_server_cpu(void *args);

void inicializarListasMemoria();

void *cpu(void* conexion);

void *kernel(void* conexion);

int hay_espacio_en_mem(int tamanio_proceso);

t_list* cargar_instrucciones_desde_archivo(char* path_instrucciones);

//FUNCIONES

struct t_tabla_proceso* buscar_proceso_por_pid(int pid);

void inicializar_mem_prin();

int cargar_archivo(int pid);

int inicializar_proceso(int pid, int tamanio);

//BITMAP
int contar_marcos_libres();
int asignar_marco_libre();
void liberar_marco(int marco);

//TDPs
struct Tabla_Nivel* crear_tabla_nivel(int nivel_actual, int nro_pagina);
struct Tabla_Principal* crear_tabla_principal();


//FUTURAS (sin implementar ni nada aún)
void suspender_proceso();
void des_suspender_proceso();
void finalizar_proceso();
void acceder_a_tdp();
void acceder_a_espacio_usuario();
void leer_pagina_completa();
void actualizar_pagina_completa();
void memory_dump();


#endif