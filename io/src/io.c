#include <io.h>

//variables globales
t_log *logger;
t_config *config;
t_log_level current_log_level;
char *nombre_modulo_io;
int segundos_espera; //esta hardcodeado pq me falta ver cuando lo recibe
//variables globales


int main(int argc, char* argv[]) {
    
    nombre_modulo_io = argv[argc-1];
    
    inicializarIo();
    log_debug(logger, nombre_modulo_io);
    dormir_IO(nombre_modulo_io,segundos_espera);
    
    return 0;
}
