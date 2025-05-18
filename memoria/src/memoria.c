#include <memoria.h>

extern void * espacio_de_usuario;
//variables globales
t_log *logger;
t_config *config;
t_log_level current_log_level;
list_struct_t *lista_sockets_cpu;

//variables globales
char* dump_path;
char* dump_path;

int main(int argc, char* argv[]) {
    
    
    inicializarMemoria();
    free(espacio_de_usuario);
    free(espacio_de_usuario);
    return 0;
}

char* crear_directorio(char* ruta_a_agregar) {
    if (!dump_path) {
        log_info(logger, "Error: La ruta base es NULL.\n");
        exit(EXIT_FAILURE);
    }
    size_t path_length = strlen(dump_path) + strlen(ruta_a_agregar) + 1;
    char* ruta = malloc(path_length);
    if (!ruta) {
        log_info(logger, "Error: No se pudo asignar memoria para la ruta del directorio.\n");
        exit(EXIT_FAILURE);
    }

    snprintf(ruta, path_length, "%s%s", dump_path,ruta_a_agregar);

    if (mkdir(ruta, 0700) == 0) {
        log_info(logger,"Directorio '%s' creado correctamente.\n", ruta);
    } else if (errno == EEXIST) {
        log_info(logger,"El directorio '%s' ya existe.\n", ruta);
    } else {
        log_info(logger,"Error al crear el directorio");
        free(ruta); // Liberar memoria en caso de error
        exit(EXIT_FAILURE);
    }

    return ruta;
}






char* crear_directorio(char* ruta_a_agregar) {
    if (!dump_path) {
        log_info(logger, "Error: La ruta base es NULL.\n");
        exit(EXIT_FAILURE);
    }
    size_t path_length = strlen(dump_path) + strlen(ruta_a_agregar) + 1;
    char* ruta = malloc(path_length);
    if (!ruta) {
        log_info(logger, "Error: No se pudo asignar memoria para la ruta del directorio.\n");
        exit(EXIT_FAILURE);
    }

    snprintf(ruta, path_length, "%s%s", dump_path,ruta_a_agregar);

    if (mkdir(ruta, 0700) == 0) {
        log_info(logger,"Directorio '%s' creado correctamente.\n", ruta);
    } else if (errno == EEXIST) {
        log_info(logger,"El directorio '%s' ya existe.\n", ruta);
    } else {
        log_info(logger,"Error al crear el directorio");
        free(ruta); // Liberar memoria en caso de error
        exit(EXIT_FAILURE);
    }

    return ruta;
}





