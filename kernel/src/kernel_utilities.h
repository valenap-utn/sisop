#ifndef KERNEL_UTILITIES_
#define KERNEL_UTILITIES_

#include <kernel.h>

void inicializarKernel();
void *server_mh_cpu(void *args);
void *server_mh_io(void *args);
void inicializarSemaforos();
void inicializarListasKernel();
void levantarConfig();

#endif