#ifndef IO_UTILITIES_
#define IO_UTILITIES_

#include <io.h>

void inicializarIo();
void levantarConfig();
void dormir_IO(char* nombre_modulo_io,int segundos_espera,int pid);
void *conexion_cliente_kernel(void *);

#endif