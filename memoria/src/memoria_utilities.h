#ifndef MEMORIA_UTILITIES_
#define MEMORIA_UTILITIES_

#include <memoria.h>

void inicializarMemoria();
void levantarConfig();
void *conexion_server_cpu(void *args);

void inicializarListasMemoria();


//FUNCIONES
void inicializar_proceso();
void suspender_proceso();
void des_suspender_proceso();
void finalizar_proceso();
void acceder_a_tdp();
void acceder_a_espacio_usuario();
void leer_pagina_completa();
void actualizar_pagina_completa();
void memory_dump();


#endif