#include <cpu.h>

//variables globales
t_log *logger;
t_config *config;
int socket_interrupt, socket_dispatch;
//variables globales

int main(int argc, char* argv[]) {
    inicializarCpu();
    return 0;
}
