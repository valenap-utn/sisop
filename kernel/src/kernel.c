#include <kernel.h>

//variables globales
t_log *logger;
t_config *config;

list_struct_t *lista_sockets_cpu;
list_struct_t *lista_sockets_io;
//variables globales

int main(int argc, char* argv[]) {
    inicializarKernel();
    return 0;
}
