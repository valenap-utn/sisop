#include <memoria_utilities.h>

extern t_log *logger;
extern t_config *config;
extern t_log_level current_log_level;
char * puerto_cpu;
char * puerto_kernel;
char * path_instrucciones;
int tam_memoria;

//variables para tdp
int  tam_pagina, entradas_por_tabla, cant_niveles;

t_memoria memoria_principal;

//variable para manejo de SWAP
char* path_swapfile;

//variables para retardos
int retardo_memoria, retardo_swap;

void inicializarMemoria(){
    
    config = config_create("./memoria.config");
    levantarConfig();
    
    logger = log_create("memoria.log", "Memoria", 1, current_log_level);
    
    inicializarListasMemoria();
    crear_directorio();
    crear_directorioSWAP();
    inicializar_mem_prin();

    pthread_t tid_cpu;
    pthread_t tid_kernel;
    
    pthread_create(&tid_cpu, NULL, conexion_server_cpu, NULL);
    pthread_create(&tid_kernel, NULL, conexion_server_kernel, NULL);
    
    
    pthread_join(tid_cpu, NULL);
    pthread_join(tid_kernel, NULL);
    // pthread_join(tid_cpu, NULL);
}

void levantarConfig(){
    puerto_cpu = config_get_string_value(config, "PUERTO_ESCUCHA_CPU");
    puerto_kernel = config_get_string_value(config, "PUERTO_ESCUCHA_KERNEL");
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    printf("%d", current_log_level);

    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");

    //Obtengo de config valores para tener en cuenta para TDPs
    tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
    cant_niveles = config_get_int_value(config, "CANTIDAD_NIVELES");

    //Obtengo path para swapfile desde config
    path_swapfile = config_get_string_value(config,"PATH_SWAPFILE");

    //Obtengo retardos desde config
    retardo_memoria = config_get_int_value(config,"RETARDO_MEMORIA");
    retardo_swap = config_get_int_value(config,"RETARDO_SWAP");

}

void inicializar_mem_prin(){
    memoria_principal.espacio = calloc(tam_memoria, 1);
    memoria_principal.tablas_por_proceso = list_create();

    memoria_principal.cantidad_marcos = tam_memoria/tam_pagina;
    memoria_principal.bitmap_marcos = calloc(memoria_principal.cantidad_marcos,sizeof(bool)); 

    memoria_principal.metadata_swap = list_create();
}

void *conexion_server_cpu(void *args) {
    
    int server_cpu = iniciar_servidor(puerto_cpu);

    /* // será error del puerto?
    if (server_cpu == -1) {
        log_error(logger, "Fallo al iniciar el servidor en el puerto %s", puerto_cpu);
        pthread_exit(NULL);
    }
    */
    int *socket_nuevo = malloc(sizeof(int));


    log_debug(logger, "Servidor escuchando en el puerto %s, esperando cliente CPU..", puerto_cpu);
    while ((*socket_nuevo = esperar_cliente(server_cpu))){
        
        // será error del cliente?
        if (*socket_nuevo == -1) {
            log_error(logger, "Fallo al aceptar conexión del cliente CPU");
            close(server_cpu);
            pthread_exit(NULL);
        }
        
        //MANEJA LAS INSTRUCCIONES
        log_info(logger, "Se conecto un nuevo CPU");
        pthread_t tid_cpu_aux;
        pthread_create(&tid_cpu_aux, NULL, cpu, (void*)socket_nuevo);
        pthread_detach(tid_cpu_aux);
        socket_nuevo = malloc(sizeof(int));
    }

    close(*socket_nuevo);
    close(server_cpu);
    pthread_exit(NULL);
}


void *conexion_server_kernel(void *args) {
    
    int server_kernel = iniciar_servidor(puerto_kernel);

    int socket_nuevo;
    // pthread_t tid_kernel;


    log_debug(logger, "Servidor escuchando en el puerto %s, esperando cliente kernel..", puerto_kernel);
    while ((socket_nuevo = esperar_cliente(server_kernel))){
        
        // será error del cliente?
        if (socket_nuevo == -1) {
            log_error(logger, "Fallo al aceptar conexión del cliente KERNEL");
            close(server_kernel);
            pthread_exit(NULL);
        }

        log_info(logger,"## Kernel Conectado - FD del socket: %d",socket_nuevo);
        peticion_kernel(socket_nuevo);
       
    }

    
    
    close(socket_nuevo);
    close(server_kernel);
    pthread_exit(NULL);
}

void inicializarListasMemoria(){
}


void * cpu(void* args){
    //realizar handshake

    int conexion = *(int *)args;
    protocolo_socket peticion;
    t_list *paquete_recv;

    while(1){
        peticion = recibir_operacion(conexion);

        //retardo para peticiones
        usleep(retardo_memoria * 1000);

        int pid;
        int pc;

        switch(peticion){
            case ENVIAR_VALORES:
            {
                paquete_recv = recibir_paquete(conexion);
                list_destroy_and_destroy_elements(paquete_recv, free);
                t_paquete* paquete_send = crear_paquete(ENVIAR_VALORES);
                agregar_a_paquete(paquete_send,&tam_pagina,sizeof(int));
                agregar_a_paquete(paquete_send, &cant_niveles, sizeof(int));
                agregar_a_paquete(paquete_send, &entradas_por_tabla, sizeof(int));
                
                enviar_paquete(paquete_send,conexion);
                eliminar_paquete(paquete_send);
            }
            break;

            case ACCEDER_A_TDP:
            {
                t_paquete* paquete_send;
                paquete_recv = recibir_paquete(conexion);
                pid = *(int*)list_remove(paquete_recv,0);

                t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
                if(!proceso){
                    log_error(logger,"PID %d no encontrado", pid);
                    list_destroy_and_destroy_elements(paquete_recv, free);
                    break;
                }

                int* indices_por_nivel = malloc(sizeof(int)*cant_niveles);
                for(int i = 0; i < cant_niveles; i++){
                    int* index = list_remove(paquete_recv,0);
                    indices_por_nivel[i] = *index;
                    free(index);
                }
                int marco_correspondiente = acceder_a_tdp(pid,indices_por_nivel);

                paquete_send = crear_paquete(DEVOLVER_MARCO);
                agregar_a_paquete(paquete_send, &marco_correspondiente,sizeof(int));
                enviar_paquete(paquete_send,conexion);
                eliminar_paquete(paquete_send);

                log_info(logger,"## PID: <%d> - Acceso a TDP - Marco obtenido: <%d>",pid,marco_correspondiente);

                free(indices_por_nivel);
                list_destroy_and_destroy_elements(paquete_recv,free);
            }
            break;

            case ACCEDER_A_ESPACIO_USUARIO:
            {
                t_paquete* paquete_send;
                paquete_recv = recibir_paquete(conexion);

                pid = *(int*)list_remove(paquete_recv,0);

                //CHEQUEAR QUE LAS COSAS SE RECIBAN/ENVIEN DE FORMA ORDENADA
                int tamanio = *(int*)list_remove(paquete_recv,0);
                int dir_fisica = *(int*)list_remove(paquete_recv,0);
                acceso_t tipo_acceso = *(int*)list_remove(paquete_recv,0); // lectura || escritura 

                t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);

                if(tipo_acceso == LECTURA_AC){ //lectura
                    // char* valor = *(int*)(memoria_principal.espacio + dir_fisica);
                    char* valor = malloc(tamanio+1);
                    char* aux = "";
                    memcpy(valor,memoria_principal.espacio + dir_fisica,tamanio);
                    memcpy(valor+tamanio,aux,1);


                    paquete_send = crear_paquete(DEVOLVER_VALOR);
                    agregar_a_paquete(paquete_send,valor,tamanio);
                    enviar_paquete(paquete_send,conexion);

                    log_info(logger,"## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d",pid,dir_fisica,tamanio);
                    proceso->metricas.cant_lecturas++;

                    eliminar_paquete(paquete_send);

                }else{ //escritura
                    char * valor_a_escribir = list_remove(paquete_recv,0);

                    log_debug(logger, "MEM: Voy a escribir '%s' en DF %d", valor_a_escribir, dir_fisica);
                    log_debug(logger, "MEM: Longitud del string a escribir: %d", strlen(valor_a_escribir));

                    // memcpy(memoria_principal.espacio + dir_fisica,valor_a_escribir,strlen(valor_a_escribir));

                    void* destino = memoria_principal.espacio + dir_fisica;

                    int offset = dir_fisica % tam_pagina;

                    if (offset == 0) {
                        memset(destino, 0, tam_pagina);  // limpieza total si empieza en offset 0
                        log_debug(logger, "MEM: Escritura desde offset 0 → se limpia página completa");
                    }

                    int len = strlen(valor_a_escribir);
                    memcpy(destino, valor_a_escribir, len);

                    //Limpieza parcial del resto de la página después del string
                    // int max_erase = tam_pagina - offset - len;
                    // if (max_erase > 0) {
                    //     memset(destino + len, 0, max_erase);
                    //     log_debug(logger, "MEM: Escritura parcial → se limpian %d bytes restantes", max_erase);
                    // }

                    log_debug(logger, "MEM: Longitud del string a escribir: %d", len);
                    
                    enviar_paquete_ok(conexion);

                    log_info(logger,"## PID: <%d> - <Escritura> - Dir. Física: <%d> - Tamaño: <%d>",pid,dir_fisica,tamanio);
                    proceso->metricas.cant_escrituras++;
                }
                list_destroy_and_destroy_elements(paquete_recv,free);
            }
            break;

            case LEER_PAG_COMPLETA:
            {
                t_paquete* paquete_send;
                paquete_recv = recibir_paquete(conexion);

                pid = *(int*)list_remove(paquete_recv,0);
                int dir_fisica = *(int*)list_remove(paquete_recv,0);

                t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);

                if(dir_fisica % tam_pagina != 0){
                    log_error(logger,"La Dirección Física %d no es inicio de página",dir_fisica);
                    list_destroy_and_destroy_elements(paquete_recv,free); 
                    break;
                }

                if(dir_fisica < 0 || dir_fisica + tam_pagina > tam_memoria){
                    log_error(logger,"Dirección %d fuera de rango",dir_fisica);
                    list_destroy_and_destroy_elements(paquete_recv,free); 
                    break;
                }

                void* contenido_pagina = memoria_principal.espacio + dir_fisica;

                paquete_send = crear_paquete(DEVOLVER_PAGINA);
                agregar_a_paquete(paquete_send,contenido_pagina,tam_pagina);
                enviar_paquete(paquete_send,conexion);

                log_info(logger,"## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d",pid,dir_fisica,tam_pagina);
                proceso->metricas.cant_lecturas++;

                eliminar_paquete(paquete_send);  
                list_destroy_and_destroy_elements(paquete_recv,free);  
            }
            break;

            case ACTUALIZAR_PAG_COMPLETA:
            {
                t_paquete* paquete_send;
                paquete_recv = recibir_paquete(conexion);
                pid = *(int*)list_remove(paquete_recv,0);
                int dir_fisica = *(int*)list_remove(paquete_recv,0);

                t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);

                if(dir_fisica % tam_pagina != 0){
                    log_error(logger,"La Dirección Física %d no es inicio de página",dir_fisica);
                    list_destroy_and_destroy_elements(paquete_recv,free); 
                    break;
                }

                if(dir_fisica < 0 || dir_fisica + tam_pagina > tam_memoria){
                    log_error(logger,"Dirección %d fuera de rango",dir_fisica);
                    list_destroy_and_destroy_elements(paquete_recv,free); 
                    break;
                }

                void* datos = list_remove(paquete_recv,0);
                memcpy(memoria_principal.espacio + dir_fisica, datos, tam_pagina); //escribo sobre el espacio de usuario
                free(datos);

                paquete_send = crear_paquete_ok();
                enviar_paquete(paquete_send,conexion);

                log_info(logger,"## PID: <%d> - <Escritura> - Dir. Física: <%d> - Tamaño: <%d>",pid,dir_fisica,tam_pagina);
                proceso->metricas.cant_escrituras++;

                eliminar_paquete(paquete_send);
                list_destroy_and_destroy_elements(paquete_recv,free); 
            }
            break;
            
            //A CHEQUEAR que esté bien
            case PEDIR_INSTRUCCION:
            {
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

                proceso->metricas.cant_instr_sol++; //creo que esto va acá

                log_info(logger, "## PID: %d - Obtener instrucción: %d - Instrucción: %s", pid, pc, instruccion);

                eliminar_paquete(paquete_send);
                list_destroy_and_destroy_elements(paquete_recv,free); 
            }    
            break;
            default: log_warning(logger,"Petición %d desconocida", peticion);
            break;
        }
    }
}

void peticion_kernel(int socket_kernel){
    //hanshake

    protocolo_socket peticion;
    t_list *paquete_recv;

    peticion = recibir_operacion(socket_kernel);

    //retardo para peticiones 
    usleep(retardo_memoria * 1000);

    int pid;
    char *nombreArchivo;

    switch(peticion){
        case PROCESS_CREATE_MEM:
        {
            int tamanio;

            paquete_recv = recibir_paquete(socket_kernel);
            pid = *(int *)list_remove(paquete_recv, 0);
            tamanio = *(int *)list_remove(paquete_recv,0); 
            nombreArchivo = strdup((char*)list_remove(paquete_recv,0)); 

            log_debug(logger, "Nombre de archivo recibido para PID %d: %s", pid, nombreArchivo);

            if (inicializar_proceso(pid, tamanio, nombreArchivo) == 0) {
                enviar_paquete_ok(socket_kernel);
                log_info(logger,"## PID: %d - Proceso Creado - Tamaño: %d", pid,tamanio);
            } else {
                log_error(logger,"No se pudo inicializar el proceso %d por falta de memoria",pid);
                t_paquete * paquete_error = crear_paquete(PROCESS_CREATE_MEM_FAIL);
                agregar_a_paquete(paquete_error, "ERROR", strlen("ERROR")+1);
                enviar_paquete(paquete_error, socket_kernel);
                eliminar_paquete(paquete_error);
            }

            list_destroy_and_destroy_elements(paquete_recv,free);
        }
        break;

        case SUSP_MEM:
        {
            // t_paquete* paquete_send_suspencion_proceso;
            paquete_recv = recibir_paquete(socket_kernel);
            pid = *(int *)list_remove(paquete_recv, 0);

            //ACA SE CARGA EN EL ARCHIVO SWAP el contenido de las páginas del proceso que fue suspendido
            suspender_proceso(pid);
            enviar_paquete_ok(socket_kernel);

            // eliminar_paquete(paquete_send_suspencion_proceso);
            list_destroy_and_destroy_elements(paquete_recv,free);
        }
        break;

        case UNSUSPEND_MEM:
        {
            // t_paquete* paquete_send_dessuspencion_proceso;
            paquete_recv = recibir_paquete(socket_kernel);
            pid = *(int *)list_remove(paquete_recv, 0);

            log_debug(logger, "Se recibió solicitud de des-suspensión para PID: %d", pid);

            //ACA SE SACA DE SWAP y se escribe en memoria segun dicho PID
            if(des_suspender_proceso(pid)){
                enviar_paquete_ok(socket_kernel);
            }else{
                log_error(logger,"No se pudo des suspender el proceso %d por falta de memoria",pid);
                t_paquete * paquete_error = crear_paquete(UNSUSPEND_MEM_ERROR);
                agregar_a_paquete(paquete_error, "ERROR", strlen("ERROR")+1);
                enviar_paquete(paquete_error, socket_kernel);
                eliminar_paquete(paquete_error);
            }

            // faltaria: enviar_paquete_ok(socket_kernel);
            // y el caso de error como en process create

            // eliminar_paquete(paquete_send_dessuspencion_proceso);
            list_destroy_and_destroy_elements(paquete_recv,free);
            //responder con un OK
        }
        break;

        case PROCESS_EXIT_MEM:
        {
            paquete_recv = recibir_paquete(socket_kernel);
            pid = *(int*)list_remove(paquete_recv,0);

            finalizar_proceso(pid); 

            enviar_paquete_ok(socket_kernel);

            list_destroy_and_destroy_elements(paquete_recv,free);
        }
        break;
        case DUMP_MEM:
        {
            // t_paquete *paquete_send_Dump;

            paquete_recv = recibir_paquete(socket_kernel);

            int pidDump = *(int *)list_remove(paquete_recv, 0);
            
            t_tabla_proceso* procesoDump = buscar_proceso_por_pid(pidDump);
            if (!procesoDump) {
                log_error(logger, "PID %d no encontrado al pedir instrucción", pidDump);
                list_destroy_and_destroy_elements(paquete_recv,free);
                break;
            }
            log_info(logger, "## PID: %d - Memory Dump solicitado",pidDump);
            cargar_archivo_dump(pidDump);
            
            list_destroy_and_destroy_elements(paquete_recv,free);
            enviar_paquete_ok(socket_kernel);
        }    
        break;
        default: log_warning(logger,"Petición %d desconocida",peticion);
        break;
    }
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
    for (int i = 0; i < list_size(memoria_principal.tablas_por_proceso); i++) {
        t_tabla_proceso* proceso = list_get(memoria_principal.tablas_por_proceso, i);
        if (proceso->pid == pid) {
            return proceso;
        }
    }
    return NULL;  // No se encontró el proceso
}

//MANEJO DE ACCESO A TDP ON-DEMAND
int acceder_a_tdp(int pid, int* indices_por_nivel){ 
    t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
    if(!proceso){
        log_error(logger,"PID %d no encontrado para acceso a TDP", pid);
        return -1;
    }

    Tabla_Nivel* actual = proceso->tabla_principal->niveles[indices_por_nivel[0]];
    if (!actual) {
        log_error(logger, "Entrada de nivel 1 [%d] es NULL", indices_por_nivel[0]);
        return -1;
    }

    for(int nivel = 0; nivel < cant_niveles - 1; nivel++){
        proceso->metricas.cant_accesos_tdp++;
        usleep(retardo_memoria * 1000);
        if(!actual || actual->es_ultimo_nivel){
            log_error(logger,"Error al querer acceder al nivel %d para PID %d",nivel,pid);
            return -1;
        }
        actual = actual->sgte_nivel[indices_por_nivel[nivel+1]];
        if (!actual) {
            log_error(logger, "Entrada NULL en nivel %d para PID %d", nivel+1, pid);
            return -1;
        }
    }

    // Acceso a último nivel
    proceso->metricas.cant_accesos_tdp++;
    usleep(retardo_memoria * 1000);

    if (actual->marco == -1 || !actual->esta_presente) {
        // On-demand: asignar marco ahora
        int nuevo_marco = asignar_marco_libre();
        if (nuevo_marco == -1) {
            log_error(logger, "No hay marcos libres para PID %d", pid);
            return -1;
        }
        actual->marco = nuevo_marco;
        actual->esta_presente = true;
        log_debug(logger, "## PID: <%d> - Page Fault: asignado marco <%d>", pid, nuevo_marco);
    }

    return actual->marco;
}

//Funciones para MEMORY_DUMP

int cargar_archivo_dump(int pid){ 
    struct timeval tiempo_actual;
    gettimeofday(&tiempo_actual, NULL);
    struct tm *tiempo_local = localtime(&tiempo_actual.tv_sec);

    char* ruta_base = crear_directorio();
    if (ruta_base == NULL) {
        log_error(logger, "No se pudo obtener la ruta base para el dump");
        return -1;
    }

    char nombre_archivo[64];
    snprintf(nombre_archivo, sizeof(nombre_archivo),
             "%d-%02d:%02d:%02d:%03ld.dmp",
             pid,
             tiempo_local->tm_hour,
             tiempo_local->tm_min,
             tiempo_local->tm_sec,
             tiempo_actual.tv_usec / 1000);

    size_t ruta_completa_len = strlen(ruta_base) + strlen(nombre_archivo) + 2;
    char* ruta_completa = malloc(ruta_completa_len);
    if (!ruta_completa) {
        log_error(logger, "No se pudo asignar memoria para la ruta completa");
        free(ruta_base);
        return -1;
    }

    snprintf(ruta_completa, ruta_completa_len, "%s/%s", ruta_base, nombre_archivo);

    t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
    if (proceso == NULL) {
        log_error(logger, "No se encontró el proceso PID %d para el dump", pid);
        free(ruta_base);
        free(ruta_completa);
        return -1;
    }


    FILE* f = fopen(ruta_completa, "w");
    if (!f) {
        log_error(logger, "No se pudo abrir archivo de dump para PID %d", pid);
        free(ruta_base);
        free(ruta_completa);
        return -1;
    }

    log_debug(logger, "## PID: <%d> - Memory Dump solicitado en %s", pid, ruta_completa);
    
    // Realizar dump
    Tabla_Nivel** niveles = proceso->tabla_principal->niveles;
    
    dump_tabla_nivel_completo(f, niveles, 1);

    // Limpieza
    fclose(f);
    free(ruta_base);
    free(ruta_completa);

    return 0;
}

//Guarda TODAS las páginas reservadas del proceso, incluso si la 
//página no está presente, escribe ceros
void dump_tabla_nivel_completo(FILE *f, Tabla_Nivel **niveles, int nivel_actual)
{
    for (int i = 0; i < entradas_por_tabla; i++)
    {
        Tabla_Nivel *entrada = niveles[i];
        if (entrada == NULL)
        { // si falta la entrada, igual "reserva" espacio con ceros
            char buffer_ceros[tam_pagina];
            memset(buffer_ceros, 0, tam_pagina);
            fwrite(buffer_ceros, 1, tam_pagina, f);
            continue;
        }

        if (entrada->es_ultimo_nivel)
        {
            log_debug(logger, "[Dump] Verificando presencia de página en Nivel %d - Entrada %d: presente = %d", nivel_actual, i, entrada->esta_presente);
            if (entrada->esta_presente)
            {
                int offset = entrada->marco * tam_pagina;
                fwrite(memoria_principal.espacio + offset, 1, tam_pagina, f);
                log_debug(logger, "[Dump] Nivel %d - Entrada %d - Marco %d - Pagina presente", nivel_actual, i, entrada->marco);
            }
            else
            {
                char buffer_ceros[tam_pagina];
                memset(buffer_ceros, 0, tam_pagina);
                fwrite(buffer_ceros, 1, tam_pagina, f);
                log_debug(logger, "[Dump] Nivel %d - Entrada %d - Pagina ausente", nivel_actual, i);
            }
        }else{ // siguiente nivel
            dump_tabla_nivel_completo(f, entrada->sgte_nivel, nivel_actual + 1);
        }
    }
}


int inicializar_proceso(int pid, int tamanio, char* nombreArchivo) {
    int paginas_necesarias = (tamanio + tam_pagina - 1) / tam_pagina;

    if (contar_marcos_libres() < paginas_necesarias) return -1;

    t_tabla_proceso* nueva_tabla = calloc(1,sizeof(t_tabla_proceso));
    nueva_tabla->pid = pid;
    nueva_tabla->tabla_principal = crear_tabla_principal(paginas_necesarias);
    nueva_tabla->cantidad_paginas = paginas_necesarias;
    if (nueva_tabla->tabla_principal == NULL) {
        log_error(logger, "Error al crear tabla principal");
        free(nueva_tabla);
        return -1;
    }

    //Reservamos marcos y marcamos en tabla
    t_list* marcos_reservados = list_create();

    for (int i = 0; i < paginas_necesarias; i++){
        int marco = asignar_marco_libre();
        if(marco == -1){
            log_error(logger,"Error al reservar marxo para pǻgina %d  del proceso %d",i,pid);

            //rollback
            for(int j = 0; j < list_size(marcos_reservados);j++){
                int* m = list_get(marcos_reservados,j);
                liberar_marco(*m);
                free(m);
            }
            list_destroy(marcos_reservados);

            liberar_tabla_principal(nueva_tabla->tabla_principal);
            free(nueva_tabla);
            return -1;
        }

        //Asigno en tabla
        marcar_marco_en_tabla(nueva_tabla->tabla_principal, i , marco);

        int* marco_ptr = malloc(sizeof(int));
        *marco_ptr = marco;
        list_add(marcos_reservados,marco_ptr);
    }

    list_destroy_and_destroy_elements(marcos_reservados,free);

    char *path_completo = malloc(strlen(path_instrucciones) + strlen(nombreArchivo) + 1);
    sprintf(path_completo, "%s%s", path_instrucciones, nombreArchivo);
    nueva_tabla->instrucciones = cargar_instrucciones_desde_archivo(path_completo);
    if (nueva_tabla->instrucciones == NULL) {
        log_error(logger, "Error al cargar instrucciones del proceso %d", pid);
        
        // liberar marcos si hubo error
        for (int i = 0; i < paginas_necesarias; i++) {
            int marco = obtener_marco_por_indice(nueva_tabla->tabla_principal, i);
            if (marco != -1) liberar_marco(marco);
        }
        
        liberar_tabla_principal(nueva_tabla->tabla_principal);
        free(nueva_tabla);
        return -1;
    }

    list_add(memoria_principal.tablas_por_proceso, nueva_tabla);

    log_debug(logger,"Nombre del path completo %s",path_completo);

    free(path_completo);
    return 0;
}


//BITMAP

int contar_marcos_libres(){
    int m_libres = 0;
    for(int i = 0; i < memoria_principal.cantidad_marcos; i++){
        if(!memoria_principal.bitmap_marcos[i]){
            m_libres++;
        }
    }
    return m_libres;
}

int asignar_marco_libre(){
    for(int i = 0; i < memoria_principal.cantidad_marcos; i++){
        if(!memoria_principal.bitmap_marcos[i]){
            memoria_principal.bitmap_marcos[i] = true;
            return i; //devuelve el marco asignado
        }
    }
    return -1; //si no hay libres
}

void liberar_marco(int marco){
    if(marco >= 0 && marco < memoria_principal.cantidad_marcos){
        memoria_principal.bitmap_marcos[marco] = false;
    }
}

/* ------- TDP ------- */

Tabla_Nivel* crear_tabla_nivel(int nivel_actual, int paginas_necesarias) {
    Tabla_Nivel* tabla = calloc(1, sizeof(Tabla_Nivel));
    if (!tabla) return NULL;

    tabla->paginas_contenidas = 0;
    tabla->esta_presente = false;
    tabla->es_ultimo_nivel = (nivel_actual == cant_niveles);

    if (tabla->es_ultimo_nivel) {
        tabla->marco = -1;
        tabla->esta_presente = false;
        tabla->paginas_contenidas = 1;
        tabla->sgte_nivel = NULL;
    } else {
        tabla->sgte_nivel = calloc(entradas_por_tabla, sizeof(Tabla_Nivel*));
        if (!tabla->sgte_nivel) {
            free(tabla);
            return NULL;
        }

        int paginas_restantes = paginas_necesarias;
        for (int i = 0; i < entradas_por_tabla && paginas_restantes > 0; i++) {
            tabla->sgte_nivel[i] = crear_tabla_nivel(nivel_actual + 1, paginas_restantes);
            if (!tabla->sgte_nivel[i]) {
                for (int j = 0; j < i; j++) {
                    if (tabla->sgte_nivel[j])
                        liberar_tabla_nivel(tabla->sgte_nivel[j]);
                }
                free(tabla->sgte_nivel);
                free(tabla);
                return NULL;
            }
            paginas_restantes -= tabla->sgte_nivel[i]->paginas_contenidas;
            tabla->paginas_contenidas += tabla->sgte_nivel[i]->paginas_contenidas;
        }
    }

    return tabla;
}

Tabla_Principal* crear_tabla_principal(int paginas_necesarias) {
    Tabla_Principal* tabla = malloc(sizeof(Tabla_Principal));
    if (!tabla) return NULL;

    tabla->niveles = calloc(entradas_por_tabla, sizeof(Tabla_Nivel*));
    if (!tabla->niveles) {
        free(tabla);
        return NULL;
    }

    int paginas_restantes = paginas_necesarias;

    for (int i = 0; i < entradas_por_tabla && paginas_restantes > 0; i++) {
        tabla->niveles[i] = crear_tabla_nivel(1, paginas_restantes);
        if (!tabla->niveles[i]) {
            for (int j = 0; j < i; j++) {
                if (tabla->niveles[j])
                    liberar_tabla_nivel(tabla->niveles[j]);
            }
            free(tabla->niveles);
            free(tabla);
            return NULL;
        }
        paginas_restantes -= tabla->niveles[i]->paginas_contenidas;
    }

    return tabla;
}

void liberar_tabla_nivel(Tabla_Nivel* tabla) {
    if (!tabla) return;

    if (tabla->es_ultimo_nivel) {
        if (tabla->marco != -1) {
            liberar_marco(tabla->marco);
        }
    } else if (tabla->sgte_nivel) {
        for (int i = 0; i < entradas_por_tabla; i++) {
            liberar_tabla_nivel(tabla->sgte_nivel[i]);
        }
        free(tabla->sgte_nivel);
    }

    free(tabla);
}

void liberar_tabla_principal(Tabla_Principal* tabla){
    if(tabla == NULL)return;

    for(int i = 0; i < entradas_por_tabla; i++){
        liberar_tabla_nivel(tabla->niveles[i]);
    }
    free(tabla->niveles);
    free(tabla);
}

/* ------- SWAP ------- */

void suspender_proceso(int pid){
    t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
    if(!proceso){
        log_error(logger, "Error al buscar el proceso con PID <%d>",pid);
        return;
    }

    FILE* f = fopen(path_swapfile, "rb+");
    if (f == NULL) {
        f = fopen(path_swapfile, "wb+");
        if (f == NULL) {
            perror("Error al crear el archivo swap");
            return;
        }
    }

    int offset_en_paginas = list_size(memoria_principal.metadata_swap);
    int offset_en_bytes = offset_en_paginas * tam_pagina;

    usleep(retardo_swap * 1000);
    for(int i = 0; i < proceso->cantidad_paginas; i++){
        int marco = obtener_marco_por_indice(proceso->tabla_principal, i);
        
        if(marco == -1){
            log_warning(logger,"PID <%d> Página <%d> no tiene marco asignado", pid, i);
            continue;
        }
        
        void* origen = memoria_principal.espacio + (marco * tam_pagina);

        fseek(f,offset_en_bytes + i * tam_pagina, SEEK_SET);

        // usleep(retardo_swap * 1000);

        fwrite(origen,1,tam_pagina,f);
        liberar_marco(marco);

        Tabla_Nivel* entrada = buscar_entrada_por_indice(proceso->tabla_principal, i);
        if (!entrada) {
            log_error(logger, "PID %d - No se encontró entrada de tabla para página lógica %d", pid, i);
        } else {
            log_debug(logger, "PID %d - Página lógica %d marcada como ausente (marco %d)", pid, i, marco);
            entrada->esta_presente = false;
            entrada->marco = -1;
        }

    }

    t_swap* nueva_entrada = malloc(sizeof(t_swap));
    nueva_entrada->pid = pid;
    nueva_entrada->pagina_inicio = offset_en_paginas;
    nueva_entrada->cantidad_paginas = proceso->cantidad_paginas;

    list_add(memoria_principal.metadata_swap, nueva_entrada);

    fclose(f);
    proceso->metricas.cant_bajadas_swap++;
    log_info(logger,"## PID: <%d> - Proceso suspendido, páginas guardadas en SWAP",pid);

}

bool des_suspender_proceso(int pid){
    t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
    if(!proceso){
        log_error(logger, "Error al buscar el proceso con PID <%d>",pid);
        return false;
    }

    //Busco la entrada
    t_swap* entrada = NULL;

    for(int i = 0; i < list_size(memoria_principal.metadata_swap);i++){
        t_swap* entrada_buscada = list_get(memoria_principal.metadata_swap,i);
        if(entrada_buscada->pid == pid){
            entrada = entrada_buscada;
            break;
        }
    }

    if(!entrada){
        log_error(logger,"No se encontró al proceso con pid %d en SWAP", pid);
        return false;
    }

    FILE* f = fopen(path_swapfile,"rb+");
    if(!f){
        log_error(logger,"No se pudo abrir el archivo de SWAP");
        // free(entrada);
        return false;
    }

    int libres = contar_marcos_libres();
    log_debug(logger, "PID %d necesita %d páginas. Marcos libres: %d", pid, entrada->cantidad_paginas, libres);
    if (libres < entrada->cantidad_paginas) {
        log_error(logger, "No hay suficientes marcos para des-suspender PID %d", pid);
        fclose(f);
        return false;
    }

    //Backup de marcos asingados por si hay que hacer rollback
    t_list* marcos_asignados = list_create();

    usleep(retardo_swap * 1000);
    for(int i = 0; i < entrada->cantidad_paginas ;i++){
        int marco = asignar_marco_libre();
        if(marco == -1){
            log_error(logger,"No hay marcos disponibles para des-suspender al proceso con pid %d", pid);
            
            //Rollback
            for(int j = 0; j < list_size(marcos_asignados); j++){
                int* m = list_get(marcos_asignados,j);
                liberar_marco(*m);
                free(m);
            }
            list_destroy(marcos_asignados);
            fclose(f);
            // free(entrada);
            return false;
        }

        int* m_copia = malloc(sizeof(int));
        *m_copia = marco;
        list_add(marcos_asignados,m_copia);

        fseek(f, (entrada->pagina_inicio + i) * tam_pagina, SEEK_SET);

        // usleep(retardo_swap * 1000);

        void* destino = memoria_principal.espacio + marco * tam_pagina;
        fread(destino,1,tam_pagina,f);

        marcar_marco_en_tabla(proceso->tabla_principal,i,marco);
        log_debug(logger, "PID %d - Página lógica %d restaurada al marco %d", pid, i, marco);
    }

    //Si todo fue bien => elimino entrada de swap
    // list_remove(memoria_principal.metadata_swap, index);
    // free(entrada);
    fclose(f);
    list_destroy_and_destroy_elements(marcos_asignados, free);

    eliminar_de_lista_por_criterio(pid,memoria_principal.metadata_swap,criterio_para_swap,free);

    proceso->metricas.cant_subidas_memoria++;
    log_info(logger,"## PID: <%d> - Proceso des-suspendido desde SWAP", pid);
    return true;
}

void finalizar_proceso(int pid){
    t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
    if(!proceso){
        log_error(logger,"No se pudo encontrar al proceso con PID <%d>",pid);
        return;
    }

    //Libero marcos ocupados por el proceso
    for(int i = 0; i < proceso->cantidad_paginas; i++){
        int marco = obtener_marco_por_indice(proceso->tabla_principal, i);
        if(marco != -1){
            liberar_marco(marco);
            log_debug(logger, "Liberado marco %d del PID %d", marco, pid);
        }
    }

    log_info(logger, "## PID: <%d> - Proceso Destruido - Métricas - Acc.T.Pag: <%d>; Inst.Sol.: <%d>; SWAP: <%d>; Mem.Prin.: <%d>; Lec.Mem.: <%d>; Esc.Mem.: <%d>",
         pid,
         proceso->metricas.cant_accesos_tdp,
         proceso->metricas.cant_instr_sol,
         proceso->metricas.cant_bajadas_swap,
         proceso->metricas.cant_subidas_memoria,
         proceso->metricas.cant_lecturas,
         proceso->metricas.cant_escrituras);


    liberar_tabla_principal(proceso->tabla_principal);
    
    list_destroy_and_destroy_elements(proceso->instrucciones, free);

    eliminar_de_lista_por_criterio(pid,memoria_principal.metadata_swap,criterio_para_swap,free); //libera swap

    eliminar_de_lista_por_criterio(pid,memoria_principal.tablas_por_proceso,criterio_para_proceso,free); //libera proceso

    log_info(logger,"## PID: <%d> - Proceso finalizado y recursos liberados",pid);

    log_debug(logger, "Cantidad de marcos libres tras EXIT: %d", contar_marcos_libres());
}

//funciones auxiliares - swap

void eliminar_de_lista_por_criterio(int pid, t_list* lista, bool (*criterio)(void*, int), void (*destructor)(void*)){
    for(int i = 0; i < list_size(lista);i++){
        void* buscado = list_get(lista,i);
        if(criterio(buscado,pid)){
            list_remove_and_destroy_element(lista,i,destructor);
            break;
        }
    }
}

bool criterio_para_swap(void* elemento, int pid){
    t_swap* buscado = (t_swap*)elemento;
    return buscado->pid == pid;
}

bool criterio_para_proceso(void* elemento, int pid){
    t_tabla_proceso* buscado = (t_tabla_proceso*) elemento;
    return buscado->pid == pid;
}

void obtener_indices_por_nivel(int nro_pagina_logica, int* indices){
    for(int i = cant_niveles - 1; i >= 0; i--){
        indices[i] = nro_pagina_logica % entradas_por_tabla;
        nro_pagina_logica /= entradas_por_tabla;
    }
}

int obtener_marco_por_indice(Tabla_Principal* tabla, int nro_pagina_logica){
    int indices[cant_niveles];
    obtener_indices_por_nivel(nro_pagina_logica,indices); // Descompongo num. de pag. lógica en índices por nivel

    Tabla_Nivel* actual = tabla->niveles[indices[0]];
    for(int i = 1; i < cant_niveles; i++){
        if(!actual || actual->es_ultimo_nivel) return -1;
        actual = actual->sgte_nivel[indices[i]];
    }

    if(!actual || !actual->esta_presente) return -1;

    return actual->marco;
}

void marcar_marco_en_tabla(Tabla_Principal* tabla,int nro_pagina_logica,int marco){
    int indices[cant_niveles];
    obtener_indices_por_nivel(nro_pagina_logica, indices); // Descompongo num. de pag. lógica en índices por nivel

    //chequeo que existe el primer nivel
    if(tabla->niveles[indices[0]] == NULL){
        tabla->niveles[indices[0]] = crear_tabla_nivel(2,indices[0]);
        if(tabla->niveles[indices[0]]==NULL){
            log_error(logger,"Error al crear el nivel 2 de la tabla");
            return;
        }
    }

    Tabla_Nivel* actual = tabla->niveles[indices[0]];
    for(int i = 1; i < cant_niveles; i++){
        if(actual->sgte_nivel == NULL){
            actual->sgte_nivel = malloc(sizeof(Tabla_Nivel*) * entradas_por_tabla);
            for(int j = 0; j < entradas_por_tabla; j++){
                actual->sgte_nivel[j] = NULL;
            }
        }

        if(actual->sgte_nivel[indices[i]]==NULL){
            actual->sgte_nivel[indices[i]] = crear_tabla_nivel(2 + i, indices[i]);
            if(!actual->sgte_nivel[indices[i]]) return;
        }

        actual = actual->sgte_nivel[indices[i]];
    }

    actual->marco = marco;
    actual->esta_presente = true;
}



Tabla_Nivel* buscar_entrada_por_indice(Tabla_Principal* tabla, int nro_pagina_logica) {
    int indices[cant_niveles];
    obtener_indices_por_nivel(nro_pagina_logica, indices);

    Tabla_Nivel* actual = tabla->niveles[indices[0]];
    for (int i = 1; i < cant_niveles; i++) {
        if (!actual || !actual->sgte_nivel) return NULL;
        actual = actual->sgte_nivel[indices[i]];
    }
    return actual;
}
