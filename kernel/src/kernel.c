#include <kernel.h>

//variables globales
t_log *logger;
t_config *config;

list_struct_t *lista_sockets_cpu_libres;
list_struct_t *lista_sockets_cpu_ocupados;
list_struct_t *lista_sockets_io;

list_struct_t *lista_procesos_new;
list_struct_t *lista_procesos_ready;
list_struct_t *lista_procesos_exec;
list_struct_t *lista_peticiones_pendientes;

//variables globales

int main(int argc, char* argv[]) {
    inicializarKernel();
    
    

    return 0;
}
