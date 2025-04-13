#include <cpu.h>

//variables globales
t_log *logger;
t_config *config;
t_log_level current_log_level;
//variables globales

int main(int argc, char* argv[]) {
    inicializarCpu();
    return 0;
}
