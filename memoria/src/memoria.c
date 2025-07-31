#include <memoria.h>


//variables globales
t_log *logger;
t_config *config;
t_log_level current_log_level;
list_struct_t *lista_sockets_cpu;

//variables globales

int main(int argc, char* argv[]) {
    
    inicializarMemoria();

    return 0;
}

char* crear_directorio() {
    char* dump_path = config_get_string_value(config, "DUMP_PATH");
    if (!dump_path) {
        log_error(logger, "Error: La ruta base es NULL.\n");
        return NULL;
    }

    char* ruta = strdup(dump_path);
    if (!ruta) {
        log_error(logger, "Error: No se pudo asignar memoria para la ruta del directorio.\n");
        return NULL;
    }

    if (mkdir(ruta, 0700) == 0) {
        log_debug(logger,"Directorio '%s' creado correctamente.\n", ruta);
    } else if (errno == EEXIST) {
        log_debug(logger,"El directorio '%s' ya existe.\n", ruta);
    } else {
        log_error(logger,"Error al crear el directorio");
        free(ruta);
        return NULL;
    }

    return ruta;
}

char* crear_directorioSWAP() {
    char* swap_path = "./swap/";
    if (!swap_path) {
        log_error(logger, "Error: La ruta base es NULL.\n");
        return NULL;
    }

    char* ruta = strdup(swap_path);
    if (!ruta) {
        log_error(logger, "Error: No se pudo asignar memoria para la ruta del directorio.\n");
        return NULL;
    }

    if (mkdir(ruta, 0700) == 0) {
        log_debug(logger,"Directorio '%s' creado correctamente.\n", ruta);
    } else if (errno == EEXIST) {
        log_debug(logger,"El directorio '%s' ya existe.\n", ruta);
    } else {
        log_error(logger,"Error al crear el directorio");
        free(ruta);
        return NULL;
    }

    return ruta;
}





