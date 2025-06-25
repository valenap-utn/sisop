#include <memoria_utilities.h>

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;
char * puerto_cpu;
extern list_struct_t *lista_sockets_cpu;
char* path_instrucciones;
int tam_memoria;

//variables para tdp
int  tam_pagina, entradas_por_tabla, cant_niveles;

t_memoria *memoria_principal;

void inicializarMemoria(){
    
    config = config_create("./memoria.config");
    levantarConfig();
    logger = log_create("memoria.log", "Memoria", 1, current_log_level);
    
    inicializarListasMemoria();

    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");


    //Obtengo de config valores para tener en cuenta para TDPs
    tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
    cant_niveles = config_get_int_value(config, "CANTIDAD_NIVELES");


    inicializar_mem_prin();

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
    while ((*socket_nuevo = esperar_cliente(server_cpu))){
        
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
        // cpu(socket_nuevo);(void*)socket_nuevo
        //pthread_create();
        sleep(0.5);
    }
    
    

    close(*socket_nuevo);
    close(server_cpu);
    pthread_exit(NULL);
}

void inicializarListasMemoria(){
    lista_sockets_cpu = inicializarLista();
}


void * cpu(void* args){
    //realizar handshake

    int conexion = *(int *)args;
    comu_cpu peticion;
    t_list *paquete_recv;

    while(1){
        peticion = recibir_operacion(conexion);

        //retardo para peticiones
        usleep(config_get_int_value(config,"RETARDO_MEMORIA")*1000);

        int pid;
        int pc;

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
                t_paquete *paquete_send_Dump;

                paquete_recv = recibir_paquete(conexion);

                int pidDump = *(int *)list_remove(paquete_recv, 0);

                t_tabla_proceso* procesoDump = buscar_proceso_por_pid(pidDump);
                if (!procesoDump) {
                    log_error(logger, "PID %d no encontrado al pedir instrucción", pidDump);
                    break;
                }
                log_info(logger, "Memory Dump: “## PID: <%d> - Memory Dump solicitado”",pidDump);
                cargar_archivo(pidDump);
            break;

            case PEDIR_INSTRUCCIONES:
            break;
            
            //A CHEQUEAR que esté bien
            case OBTENER_INSTRUCCION:
                t_paquete *paquete_send;
                paquete_recv = recibir_paquete(conexion);
                pid = *(int *)list_remove(paquete_recv, 0);
                pc = *(int *)list_remove(paquete_recv, 0);

                t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
                if (!proceso) {
                    log_error(logger, "PID %d no encontrado al pedir instrucción", pid);
                    break;
                }

                if (pc >= list_size(proceso->instrucciones)) {
                    log_error(logger, "Índice %d fuera de rango para PID %d", pc, pid);
                    break;
                }

                char* instruccion = list_get(proceso->instrucciones, pc);
                int len = strlen(instruccion) + 1;

                paquete_send = crear_paquete(DEVOLVER_INSTRUCCION);
                agregar_a_paquete(paquete_send, instruccion, len);

                enviar_paquete(paquete_send, conexion);

                log_info(logger, "## PID: <%d> - Obtener instrucción: <%d> - Instrucción: <INSTRUCCIÓN> <%s>", pid, pc, instruccion);

            break;
        }
    }
}

void * kernel(void* args){
    //hanshake

    int conexion = *(int *)args;
    comu_kernel peticion;
    t_list *paquete_recv;

    while(1){
        peticion = recibir_operacion(conexion);

        //retardo para peticiones 
        usleep(config_get_int_value(config,"RETARDO_MEMORIA")*1000);

        int pid;

        switch(peticion){
            case INICIALIZAR_PROCESO:

                int tamanio;

                // recv(*conexion,&pid,sizeof(int),MSG_WAITALL);
                // recv(*conexion,&tamanio,sizeof(int),MSG_WAITALL);

                t_paquete *paquete_send;
                paquete_recv = recibir_paquete(conexion);
                pid = *(int *)list_remove(paquete_recv, 0);
                tamanio = *(int *)list_remove(paquete_recv,0); //no entiendo muy bien como es esto...

                if(hay_espacio_en_mem(tamanio)) inicializar_proceso(pid,tamanio);
                else log_error(logger,"No se pudo inicializar el proceso por falta de memoria");

            break;

            case SUSPENDER_PROCESO:
                //ACA SE CARGA EN SWAP
            break;

            case DESSUPENDER_PROCESO:
            break;

            case FINALIZAR_PROCESO:
            break;
        }
    }
}

//Sería así la funcion para calcular el espacio en memoria ?
int hay_espacio_en_mem(int tamanio_proceso) {
    int paginas_necesarias = (tamanio_proceso + tam_pagina - 1) / tam_pagina;
    int marcos_libres = contar_marcos_libres(memoria_principal);

    return (paginas_necesarias <= marcos_libres);
}

//Para cargar instrucciones desde path de config en nuevo_proceso->instrucciones
t_list* cargar_instrucciones_desde_archivo(char* path) {
    FILE* archivo = fopen(path, "r");
    if (!archivo) {
        log_error(logger, "No se pudo abrir el archivo de instrucciones en %s", path);
        return NULL;
    }

    t_list* instrucciones = list_create();
    char* linea = NULL;
    size_t len = 0;

    while (getline(&linea, &len, archivo) != -1) {
        // Eliminar salto de línea si existe
        linea[strcspn(linea, "\n")] = '\0';
        list_add(instrucciones, strdup(linea));
    }

    free(linea);
    fclose(archivo);
    return instrucciones;
}


struct t_tabla_proceso* buscar_proceso_por_pid(int pid) {
    for (int i = 0; i < list_size(memoria_principal->tablas_por_proceso); i++) {
        t_tabla_proceso* proceso = list_get(memoria_principal->tablas_por_proceso, i);
        if (proceso->pid == pid) {
            return proceso;
        }
    }
    return NULL;  // No se encontró el proceso
}

void inicializar_mem_prin(){
    memoria_principal = malloc(sizeof(t_memoria));
    memoria_principal->espacio = malloc(sizeof(uint32_t)*tam_memoria);
    memoria_principal->tablas_por_proceso = list_create();

    memoria_principal->cantidad_marcos = tam_memoria/tam_pagina;
    memoria_principal->bitmap_marcos = calloc(memoria_principal->cantidad_marcos,sizeof(bool)); 
}


int cargar_archivo(int pid /*,PCB* proceso */){ 
    struct timeval tiempo_actual;
    gettimeofday(&tiempo_actual, NULL);
    struct tm *tiempo_local = localtime(&tiempo_actual.tv_sec);

    char *nombre_archivo = malloc(60);
    if (nombre_archivo == NULL) {
        perror("Error al asignar memoria");
        return EXIT_FAILURE;
    }
    snprintf(nombre_archivo, 60,
            "%d-%02d:%02d:%02d:%03ld.dmp",
            pid,
            tiempo_local->tm_hour,
            tiempo_local->tm_min,
            tiempo_local->tm_sec,
            tiempo_actual.tv_usec / 1000);

    log_info(logger, "Nombre del archivo de dump: %s", nombre_archivo);
    return 0;
}

/* ------- + PROPUESTA by valucha ------- */

int inicializar_proceso(int pid, int tamanio){
    t_tabla_proceso* nueva_tabla = malloc(sizeof(t_tabla_proceso));
    nueva_tabla->pid = pid;
    nueva_tabla->tabla_principal = crear_tabla_principal();

    nueva_tabla->instrucciones = cargar_instrucciones_desde_archivo(path_instrucciones);
    if(nueva_tabla->instrucciones == NULL){
        log_error(logger,"Error al cargar instrucciones del proceso %d", pid);
        free(nueva_tabla);
        return -1;
    }

    list_add(memoria_principal->tablas_por_proceso, nueva_tabla);

    log_info(logger,"## PID: <%d> - Proceso Creado - Tamaño: <%d>", pid,tamanio);

    return 0;
}

//BITMAP

int contar_marcos_libres(){
    int m_libres = 0;
    for(int i = 0; i < memoria_principal->cantidad_marcos; i++){
        if(!memoria_principal->bitmap_marcos[i]){
            m_libres++;
        }
    }
    return m_libres;
}

int asignar_marco_libre(){
    for(int i = 0; i < memoria_principal->cantidad_marcos; i++){
        if(!memoria_principal->bitmap_marcos[i]){
            memoria_principal->bitmap_marcos[i] = true;
            return i; //devuelve el marco asignado
        }
    }
    return -1; //si no hay libres
}

void liberar_marco(int marco){
    if(marco <= 0 && marco < memoria_principal->cantidad_marcos){
        memoria_principal->bitmap_marcos[marco] = false;
    }
}

/* ------- PROPUESTA by valucha para TDP ------- */

struct Tabla_Nivel* crear_tabla_nivel(int nivel_actual, int nro_pagina){
    Tabla_Nivel* tabla = malloc(sizeof(Tabla_Nivel));
    tabla->nro_pagina = nro_pagina;
    tabla->esta_presente = false;
    tabla->es_ultimo_nivel = (nivel_actual == cant_niveles);

    if(tabla->es_ultimo_nivel){
        tabla->marco = -1;
    }else{
        tabla->sgte_nivel = malloc(sizeof(Tabla_Nivel*)* entradas_por_tabla);
        for(int i = 0; i < entradas_por_tabla; i++){
            tabla->sgte_nivel[i] = crear_tabla_nivel((nivel_actual + 1),i);
        }
    }
    return tabla;
}

struct Tabla_Principal* crear_tabla_principal(){
    Tabla_Principal* tabla = malloc(sizeof(Tabla_Principal));
    tabla->niveles = malloc(sizeof(Tabla_Nivel*)*entradas_por_tabla);

    for(int i = 0; i < entradas_por_tabla; i++){
        tabla->niveles[i] = crear_tabla_nivel(2,i);
    }

    return tabla;
}
