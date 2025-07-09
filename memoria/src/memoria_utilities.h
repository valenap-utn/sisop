#ifndef MEMORIA_UTILITIES_
#define MEMORIA_UTILITIES_

#include <memoria.h>
#include <utils/utils.h>
#include "../../kernel/src/pcb.h"

// void inicializarMemoria();

void levantarConfig();

void inicializar_mem_prin();

void *conexion_server_cpu(void *args);

void *conexion_server_kernel(void *args);

void inicializarListasMemoria();

void *cpu(void* conexion);

void peticion_kernel(int socket_kernel);

int hay_espacio_en_mem(int tamanio_proceso);
t_list* cargar_instrucciones_desde_archivo(char* path_instrucciones);
struct t_tabla_proceso* buscar_proceso_por_pid(int pid);

int acceder_a_tdp(int pid, int* indices_por_nivel);

int cargar_archivo(int pid);
void dump_tabla_nivel(FILE* f, Tabla_Nivel** niveles, int nivel_actual);
void dump_tabla_nivel_completo(FILE* f, Tabla_Nivel** niveles, int nivel_actual);

int inicializar_proceso(int pid, int tamanio, char* nombreArchivo);

//BITMAP
int contar_marcos_libres();
int asignar_marco_libre();
void liberar_marco(int marco);

//TDPs
struct Tabla_Nivel* crear_tabla_nivel(int nivel_actual, int nro_pagina);
struct Tabla_Principal* crear_tabla_principal();
void liberar_tabla_nivel(Tabla_Nivel* tabla);
void liberar_tabla_principal(Tabla_Principal* tabla);

//FUTURAS (sin implementar ni nada aún)
void suspender_proceso(int pid);
void des_suspender_proceso(int pid);

void obtener_indices_por_nivel(int nro_pagina_logica, int* indices);
int obtener_marco_por_indice(Tabla_Principal* tabla, int nro_pagina_logica);
void marcar_marco_en_tabla(Tabla_Principal* proceso,int nro_pagina_logica,int marco);

void finalizar_proceso(int pid);

// void acceder_a_espacio_usuario();
// void leer_pagina_completa();
// void actualizar_pagina_completa();
// void memory_dump();


#endif