#include <kernel.h>

//variables globales
t_log *logger;
t_config *config;


int flag_all_start = 0; 

int estimacion_inicial;
double alfa;

list_struct_t *lista_sockets_cpu;
list_struct_t *lista_sockets_io;

list_struct_t *lista_procesos_new;
list_struct_t *lista_procesos_new_fallidos;
list_struct_t *lista_procesos_ready;
list_struct_t *lista_exec;
list_struct_t *lista_procesos_block;
list_struct_t *lista_procesos_susp_ready;
list_struct_t *lista_procesos_susp_block;

list_struct_t *lista_peticiones_memoria_pendientes;
list_struct_t *lista_peticiones_io_pendientes;


pthread_cond_t * cond_all_start;
pthread_mutex_t * mutex_all_start;

//variables globales

int main(int argc, char* argv[]) {
    inicializarKernel();

    pthread_t tid_server_mh_cpu;
    pthread_t tid_server_mh_io;
    pthread_t tid_largoplazo;
    pthread_t tid_peticiones_memoria;
    pthread_t tid_mediano_plazo;
    
    pthread_create(&tid_server_mh_cpu, NULL, server_mh_cpu, NULL);
    pthread_create(&tid_server_mh_io, NULL, server_mh_io, NULL);
    
    //Al iniciar el proceso Kernel, el algoritmo de Largo Plazo debe estar frenado (estado STOP) y se deberá esperar un ingreso de un Enter por teclado para poder iniciar con la planificación.
    //me imagino que hay que leer teclado aca en main, y arrancar la siguiente linea cuando se presione
    // Al final, lo manejamos con el flag_all_start y las funciones de esperar y destrabar
    pthread_create(&tid_largoplazo, NULL, largoPlazo, NULL);
    pthread_create(&tid_peticiones_memoria, NULL, administrador_peticiones_memoria, NULL);
    pthread_create(&tid_mediano_plazo, NULL, medianoplazo, NULL);

    
    
    //enter to continue:
    char buffer[2];
    fgets(buffer, sizeof(buffer), stdin);
    //
    destrabar_flag_global(&flag_all_start, mutex_all_start, cond_all_start);
    log_debug(logger, "Flag global destrabado");

    // levanto proceso main
    char * path = argv[argc-2];
    char * size_str = argv[argc-1];
    int size = atoi(size_str);

    log_debug(logger, "Proceso inicial: %s, tamaño: %d", path, size);

    PROCESS_CREATE(path, size);

    pthread_join(tid_server_mh_cpu, NULL);
    pthread_join(tid_server_mh_io, NULL);
    pthread_join(tid_largoplazo, NULL);
    pthread_join(tid_peticiones_memoria, NULL);    

    return 0;
}
