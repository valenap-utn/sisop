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

    pthread_t tid_server_mh_cpu;
    pthread_t tid_server_mh_io;
    pthread_t tid_largoplazo, tid_cortoplazo;
    
    pthread_create(&tid_server_mh_cpu, NULL, server_mh_cpu, NULL);
    pthread_create(&tid_server_mh_io, NULL, server_mh_io, NULL);
    
    //Al iniciar el proceso Kernel, el algoritmo de Largo Plazo debe estar frenado (estado STOP) y se deberá esperar un ingreso de un Enter por teclado para poder iniciar con la planificación.
    //me imagino que hay que leer teclado aca en main, y arrancar la siguiente linea cuando se presione
    // Al final, lo manejamos con el flag_all_start y las funciones de esperar y destrabar
    pthread_create(&tid_largoplazo, NULL, largoPlazo, NULL);
    
    //enter to continue:
    char buffer[2];
    fgets(buffer, sizeof(buffer), stdin);
    //
    destrabar_flag_global(&flag_all_start);
    log_debug(logger, "Flag global destrabado");

    pthread_join(tid_server_mh_cpu, NULL);
    pthread_join(tid_server_mh_io, NULL);
    pthread_join(tid_largoplazo, NULL);

    return 0;
}
