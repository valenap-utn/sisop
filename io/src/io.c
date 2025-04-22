#include <io.h>

//variables globales
t_log *logger;
t_config *config;
t_log_level current_log_level;

char *nombre_modulo_io;
//variables globales

int main(int argc, char* argv[]) {
    
    nombre_modulo_io = argv[argc-1];
    
    inicializarIo();

    log_debug(logger, nombre_modulo_io);
    return 0;
}
