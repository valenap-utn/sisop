#include <kernel.h>

//variables globales
t_log *logger;
t_config *config;

list_struct_t *lista_sockets_cpu;
list_struct_t *lista_sockets_io;

list_struct_t *lista_procesos_new;
list_struct_t *lista_procesos_ready;

int socket_dispatch_cpu = -1;

//variables globales

int main(int argc, char* argv[]) {
    inicializarKernel();
    
    

    return 0;
}
