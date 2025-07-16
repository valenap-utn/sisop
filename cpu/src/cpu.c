#include <cpu.h>

//variables globales
t_log *logger;
t_config *config;
int socket_interrupt, socket_dispatch;

sem_t * sem_dispatch_inicial;
//variables globales

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], " ") == 0){
        printf("No se pudo cargar los logs de cpu\n");
        exit(EXIT_FAILURE);
    };
    inicializarCpu(argv[1]);
    return 0;
}
