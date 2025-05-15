#include <memoria_utilities.h>

void* espacio_de_usuario;

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;
char * puerto_cpu;
extern list_struct_t *lista_sockets_cpu;

int tam_memoria,tam_pagina,cant_entradas_x_tabla,cant_niveles;
extern char* dump_path;
char* swapfile_path;

void inicializarMemoria(){
    
    config = config_create("./memoria.config");
    levantarConfig();
    logger = log_create("memoria.log", "Memoria", 1, current_log_level);

    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    cant_entradas_x_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
    cant_niveles = config_get_int_value(config,"CANTIDAD_NIVELES");
    swapfile_path = config_get_string_value(config,"PATH_SWAPFILE");
    dump_path = config_get_string_value(config,"DUMP_PATH");

    espacio_de_usuario = calloc(1,tam_memoria); //RECORDAR HACER UN FREE DE ESTO (no lo puse)
    
    inicializarListasMemoria();

    pthread_t tid_cpu;
    pthread_create(&tid_cpu, NULL, conexion_server_cpu, NULL);
    
    pthread_join(tid_cpu, NULL);
    pthread_join(tid_cpu, NULL);
}

void levantarConfig(){
    puerto_cpu = config_get_string_value(config, "PUERTO_ESCUCHA_CPU");
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    printf("%d", current_log_level);

}

/*
void *conexion_server_cpu(void *args){
    int server_cpu = iniciar_servidor(puerto_cpu);
    log_info(logger, "Servidor listo para recibir al cliente CPU");
    socket_cliente_cpu = esperar_cliente(server_cpu);
   	close(server_cpu);
	close(socket_cliente_cpu);
    pthread_exit(EXIT_SUCCESS);
}
*/

//CONEXION CPU-MEMORIA

void *conexion_server_cpu(void *args) {
    
    int server_cpu = iniciar_servidor(puerto_cpu);

    /* // será error del puerto?
    if (server_cpu == -1) {
        log_error(logger, "Fallo al iniciar el servidor en el puerto %s", puerto_cpu);
        pthread_exit(NULL);
    }
    */
    int *socket_nuevo = malloc(sizeof(int));


    log_info(logger, "Servidor escuchando en el puerto %s, esperando cliente CPU..", puerto_cpu);
    while (*socket_nuevo = esperar_cliente(server_cpu)){
        
        // será error del cliente?
        if (*socket_nuevo == -1) {
            log_error(logger, "Fallo al aceptar conexión del cliente CPU");
            close(server_cpu);
            pthread_exit(NULL);
        }
        
        pthread_mutex_lock(lista_sockets_cpu->mutex);
        list_add(lista_sockets_cpu->lista, socket_nuevo);
        pthread_mutex_unlock(lista_sockets_cpu->mutex);
        
        //MANEJA LAS INSTRUCCIONES
        cpu(server_cpu);
    }

    close(*socket_nuevo);
    close(server_cpu);
    pthread_exit(NULL);
}

void inicializarListasMemoria(){
    lista_sockets_cpu = inicializarLista();
}


void cpu(int* conexion){
    //realizar handshake

    while(1){
        int peticion;
        recv(*conexion,&peticion,sizeof(int),MSG_WAITALL);

        //retardo para peticiones
        usleep(config_get_int_value(config,"RETARDO_MEMORIA")*1000);

        int pid;

        switch(peticion){
            case ACCEDER_A_TDP:
            break;

            case ACCEDER_A_ESPACIO_USUARIO:
            break;

            case LEER_PAG_COMPLETA:
            break;

            case ACTUALIZAR_PAG_COMPLETA:
            break;

            case MEMORY_DUMP:
            break;
        }
    }
}

void kernel(int* conexion){
    //hanshake ???
    while(1){
        int peticion;
        recv(*conexion,&peticion,sizeof(int),MSG_WAITALL);

        //retardo en peticiones
        usleep(config_get_int_value(config,"RETARDO_MEMORIA")*1000);

        int pid;

        switch(peticion){
            case INICIALIZAR_PROCESO:

                int tamanio;

                recv(*conexion,&pid,sizeof(int),MSG_WAITALL);
                recv(*conexion,&tamanio,sizeof(int),MSG_WAITALL);

                if(hay_espacio_en_mem(tamanio)) inicializar_proceso(pid);
                else log_error(logger,"No se pudo inicializar el proceso por falta de memoria");

            break;

            case SUSPENDER_PROCESO:
            break;

            case DESSUPENDER_PROCESO:
            break;

            case FINALIZAR_PROCESO:
            break;
        }
    }
}

int hay_espacio_en_mem(int tamanio){
    int tam_disp = 0; //CALCULAR EL TAMANIO DISPONIBLE (ver despues)
    (tamanio > tam_disp) ? 0 : 1;
}

            


//CONEXION KERNEL-MEMORIA


//OTRAS COSAS DE MEMORIA

//FUNCIONES PARA TDP

// t_tabla_nivel* crear_tabla_nivel(int nivel_actual){
//     t_tabla_nivel* tabla = malloc(sizeof(t_tabla_nivel));
//     tabla->entradas = malloc(sizeof(t_entrada_tabla*) * cant_entradas_x_tabla);

//     for (int i = 0; i < cant_entradas_x_tabla; i++) {
//         tabla->entradas[i] = malloc(sizeof(t_entrada_tabla));
//         tabla->entradas[i]->presente = false;
//         tabla->entradas[i]->es_ultimo_nivel = (nivel_actual == cant_niveles - 1);
//         tabla->entradas[i]->siguiente_nivel = NULL;
//         tabla->entradas[i]->marco_fisico = 0;
//     }

//     return tabla;
// }



int inicializar_proceso(int pid){
    t_proceso* nuevo_proceso = malloc(sizeof(t_proceso));
    nuevo_proceso->pid = pid;
    // nuevo_proceso->tabla_raiz = crear_tabla_nivel(0); // nivel 0 == raíz

    int tamanio = 0; //HAY QUE CALCULAR ESTO

    // Agregalo a tu lista global de procesos
    //list_add(procesos_memoria, nuevo_proceso);

    log_info(logger, "Creación de Proceso: ## PID: <%d> - Proceso Creado - Tamaño: <%d>", pid,tamanio);

    return 0; // o retornar "OK" si se espera mensaje
}

// void liberar_tabla_nivel(t_tabla_nivel* tabla, int nivel_actual) {
//     for (int i = 0; i < cant_entradas_x_tabla; i++) {
//         t_entrada_tabla* entrada = tabla->entradas[i];

//         if (entrada->presente && !entrada->es_ultimo_nivel && entrada->siguiente_nivel != NULL) {
//             // Liberar la tabla del siguiente nivel recursivamente
//             liberar_tabla_nivel(entrada->siguiente_nivel, nivel_actual + 1);
//         }

//         free(entrada);
//     }

//     free(tabla->entradas);
//     free(tabla);
// }

/* HAY QUE VER COMO IMPLEMENTARLA CON SWAP

void finalizar_proceso(int pid) {
    Proceso* proceso = buscar_proceso_por_pid(pid);
    if (proceso == NULL) return;

    liberar_tabla_nivel(proceso->tabla_raiz, 0);

    list_remove_element(procesos_memoria, proceso);
    free(proceso);

    log_info(logger, "Proceso PID %d finalizado y estructuras liberadas", pid);
}
*/
