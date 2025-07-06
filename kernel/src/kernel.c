#include <kernel.h>

//variables globales
t_log *logger;
t_config *config;

int flag_all_start = 0; 

list_struct_t *lista_sockets_cpu;
list_struct_t *lista_sockets_io;

list_struct_t *lista_procesos_new;
list_struct_t *lista_procesos_ready;
list_struct_t *lista_procesos_exec;
list_struct_t *lista_peticiones_pendientes;

pthread_cond_t * sem_all_start_cond;
pthread_mutex_t * mutex_all_start_mutex;

//variables globales

int main(int argc, char* argv[]) {
    inicializarKernel();

    return 0;
}
