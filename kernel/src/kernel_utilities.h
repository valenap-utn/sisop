#ifndef KERNEL_UTILITIES_
#define KERNEL_UTILITIES_

#include <kernel.h>
#include <utils/utils.h>
#include <largoplazo.h>

void inicializarKernel();
void *server_mh_cpu(void *args);
void *server_mh_io(void *args);
void inicializarSemaforos();
void inicializarListasKernel();
enum_algoritmo_largoPlazo alg_largoPlazo_from_string(char *string);
void levantarConfig();

#endif