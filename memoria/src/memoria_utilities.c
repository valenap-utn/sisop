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

//variable para manejo de SWAP
char* path_swapfile;

void inicializarMemoria(){
    
    config = config_create("./memoria.config");
    levantarConfig();
    logger = log_create("memoria.log", "Memoria", 1, current_log_level);
    
    inicializarListasMemoria();
    crear_directorio();
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");


    //Obtengo de config valores para tener en cuenta para TDPs
    tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
    cant_niveles = config_get_int_value(config, "CANTIDAD_NIVELES");

    //Obtengo path para swapfile desde config
    path_swapfile = config_get_string_value(config,"PATH_SWAPFILE");

    inicializar_mem_prin();

    pthread_t tid_cpu;
    pthread_create(&tid_cpu, NULL, conexion_server_cpu, NULL);
    
    pthread_join(tid_cpu, NULL);
    // pthread_join(tid_cpu, NULL);
}

void levantarConfig(){
    puerto_cpu = config_get_string_value(config, "PUERTO_ESCUCHA_CPU");
    char *value = config_get_string_value(config, "LOG_LEVEL");
    current_log_level = log_level_from_string(value);
    printf("%d", current_log_level);

}

void inicializar_mem_prin(){
    memoria_principal = malloc(sizeof(t_memoria));
    memoria_principal->espacio = malloc(sizeof(uint32_t)*tam_memoria);
    memoria_principal->tablas_por_proceso = list_create();

    memoria_principal->cantidad_marcos = tam_memoria/tam_pagina;
    memoria_principal->bitmap_marcos = calloc(memoria_principal->cantidad_marcos,sizeof(bool)); 

    memoria_principal->metadata_swap = list_create();
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
                int tipo_acceso = *(int*)list_remove(paquete_recv,0); // lectura || escritura 

                t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);

                if(tipo_acceso == 0){ //lectura
                    int valor = *(int*)(memoria_principal->espacio + dir_fisica);

                    paquete_send = crear_paquete(DEVOLVER_VALOR);
                    agregar_a_paquete(paquete_send,&valor,sizeof(int));
                    enviar_paquete(paquete_send,conexion);

                    log_info(logger,"## PID: <%d> - <Lectura> - Dir. Física: <%d> - Tamaño: <%d>",pid,dir_fisica,tamanio);
                    proceso->metricas.cant_lecturas++;

                }else{ //escritura
                    int valor_a_escribir = *(int*)list_remove(paquete_recv,0);
                    memcpy(memoria_principal->espacio + dir_fisica,&valor_a_escribir,sizeof(int));

                    paquete_send = crear_paquete(OK);
                    enviar_paquete(paquete_send,conexion);

                    log_info(logger,"## PID: <%d> - <Escritura> - Dir. Física: <%d> - Tamaño: <%d>",pid,dir_fisica,tamanio);
                    proceso->metricas.cant_escrituras++;
                }
                eliminar_paquete(paquete_send);
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

                void* contenido_pagina = memoria_principal->espacio + dir_fisica;

                paquete_send = crear_paquete(DEVOLVER_PAGINA);
                agregar_a_paquete(paquete_send,contenido_pagina,tam_pagina);
                enviar_paquete(paquete_send,conexion);

                log_info(logger,"## PID: <%d> - <Lectura> - Dir. Física: <%d> - Tamaño: <%d>",pid,dir_fisica,tam_pagina);
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
                memcpy(memoria_principal->espacio + dir_fisica, datos, tam_pagina); //escribo sobre el espacio de usuario
                free(datos);

                paquete_send = crear_paquete_ok();
                enviar_paquete(paquete_send,conexion);

                log_info(logger,"## PID: <%d> - <Escritura> - Dir. Física: <%d> - Tamaño: <%d>",pid,dir_fisica,tam_pagina);
                proceso->metricas.cant_escrituras++;

                eliminar_paquete(paquete_send);
                list_destroy_and_destroy_elements(paquete_recv,free); 
            }
            break;

            case MEMORY_DUMP:
            {
                // t_paquete *paquete_send_Dump;

                paquete_recv = recibir_paquete(conexion);

                int pidDump = *(int *)list_remove(paquete_recv, 0);
                
                t_tabla_proceso* procesoDump = buscar_proceso_por_pid(pidDump);
                if (!procesoDump) {
                    log_error(logger, "PID %d no encontrado al pedir instrucción", pidDump);
                    list_destroy_and_destroy_elements(paquete_recv,free);
                    break;
                }
                cargar_archivo(pidDump);
                log_info(logger, "Memory Dump: “## PID: <%d> - Memory Dump solicitado”",pidDump);

                list_destroy_and_destroy_elements(paquete_recv,free);
            }    
            break;

            // case OBTENER_INSTRUCCION:
            // {
            //     t_paquete* paquete_send;
            //     paquete_recv = recibir_paquete(conexion);
            //     pid = *(int*)list_remove(paquete_recv,0);
            // }
            // break;
            
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

                log_info(logger, "## PID: <%d> - Obtener instrucción: <%d> - Instrucción: <INSTRUCCIÓN> <%s>", pid, pc, instruccion);

                eliminar_paquete(paquete_send);
                list_destroy_and_destroy_elements(paquete_recv,free); 
            }    
            break;
            default: log_warning(logger,"Petición %d desconocida", peticion);
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
            {
                int tamanio;

                t_paquete* paquete_send;
                paquete_recv = recibir_paquete(conexion);
                pid = *(int *)list_remove(paquete_recv, 0);
                tamanio = *(int *)list_remove(paquete_recv,0); 

                if(hay_espacio_en_mem(tamanio)){
                    if(inicializar_proceso(pid,tamanio) == 0){
                        paquete_send = crear_paquete_ok();
                        enviar_paquete(paquete_send,conexion);
                        eliminar_paquete(paquete_send);
                        log_info(logger,"## PID: <%d> - Proceso Creado - Tamaño: <%d>", pid,tamanio);
                    } else log_error(logger,"Error al inicializar estructuras para el PID %d",pid);
                } else log_error(logger,"No se pudo inicializar el proceso %d por falta de memoria",pid);

                log_info(logger,"## PID: <%d> - Proceso Creado - Tamaño: <%d>",pid,tamanio);

                eliminar_paquete(paquete_send);
                list_destroy_and_destroy_elements(paquete_recv,free);
            }
            break;

            case SUSPENDER_PROCESO:
                t_paquete* paquete_send_suspencion_proceso;
                paquete_recv = recibir_paquete(conexion);
                pid = *(int *)list_remove(paquete_recv, 0);

                //ACA SE CARGA EN EL ARCHIVO SWAP el contenido de las páginas del proceso que fue suspendido
                suspender_proceso(pid);

                eliminar_paquete(paquete_send_suspencion_proceso);
                list_destroy_and_destroy_elements(paquete_recv,free);
            break;

            case DESSUPENDER_PROCESO:
                t_paquete* paquete_send_dessuspencion_proceso;
                paquete_recv = recibir_paquete(conexion);
                pid = *(int *)list_remove(paquete_recv, 0);

                //ACA SE SACA DE SWAP y se escribe en memoria segun dicho PID
                des_suspender_proceso(pid);

                eliminar_paquete(paquete_send_dessuspencion_proceso);
                list_destroy_and_destroy_elements(paquete_recv,free);
                //responder con un OK
            break;

            case FINALIZAR_PROCESO:
            break;
            default: log_warning(logger,"Petición %d desconocida",peticion);
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

int acceder_a_tdp(int pid, int* indices_por_nivel){
    t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
    if(!proceso){
        log_error(logger,"PID %d no encontrado para acceso a TDP", pid);
        return -1;
    }

    Tabla_Nivel* actual = proceso->tabla_principal->niveles[indices_por_nivel[0]];

    for(int nivel = 0; nivel < (cant_niveles-1); nivel++){
        proceso->metricas.cant_accesos_tdp++;
        usleep(config_get_int_value(config,"RETARDO_MEMORIA")*1000);
        if(!actual || actual->es_ultimo_nivel){
            log_error(logger,"Error al querer acceder al nivel %d para PID %d",nivel,pid);
            return -1;
        }
        actual = actual->sgte_nivel[indices_por_nivel[nivel+1]];
    }

    //Acceso a utlimo nivel
    proceso->metricas.cant_accesos_tdp++;
    usleep(config_get_int_value(config,"RETARDO_MEMORIA")*1000);

    if(!actual->esta_presente || actual->marco == -1){
        log_warning(logger,"Marco no presente para PID %d",pid);
        return -1;
    }

    return actual->marco;
}

//Funciones para MEMORY_DUMP

int cargar_archivo(int pid){ 
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

    log_info(logger, "## PID: <%d> - Memory Dump solicitado en %s", pid, ruta_completa);

    // esta parte del enunciado me queda retumbando en la cabeza:
    // crear un nuevo archivo con el TAMANIO TOTAL de 
    // la memoria reservada por el proceso
    // y debe escribir en dicho archivo todo el contenido 
    // actual de la memoria del mismo

    //existe algo para obtener el contenido en memoria? y tamanio? osea tipo el largo del proceso,
    //no se si me explico bien, si podemos obtener eso ya creo que estamos, y lo armamos tipo tabla 

    //lo cargamos como texto plano

    //PID? | TAMANIO TOTAL de memoria reservada | contenido de memoria
    
    // Realizar dump
    Tabla_Nivel** niveles = proceso->tabla_principal->niveles;
    


    dump_tabla_nivel_completo(f, niveles, 1);

    // Limpieza
    fclose(f);
    free(ruta_base);
    free(ruta_completa);

    return 0;
}


void dump_tabla_nivel(FILE* f, Tabla_Nivel** niveles, int nivel_actual){ //recorre recursivamente los niveles y guarda el contenido de marcos presentes
    for(int i = 0 ; i < entradas_por_tabla ; i++){
        Tabla_Nivel* entrada = niveles[i];
        if(entrada == NULL) continue;

        if(entrada->es_ultimo_nivel){
            if(entrada->esta_presente && entrada->marco != -1){
                int offset = entrada->marco * tam_pagina;
                fwrite((char*)memoria_principal->espacio + offset, 1, tam_pagina,f);
            }
        }else{
            dump_tabla_nivel(f,entrada->sgte_nivel,nivel_actual+1);
        }
    }
}

//Guarda TODAS las páginas reservadas del proceso, incluso si la 
//página no está presente, escribe ceros
void dump_tabla_nivel_completo(FILE* f, Tabla_Nivel** niveles, int nivel_actual){
    for(int i = 0; i < entradas_por_tabla; i++){
        Tabla_Nivel* entrada = niveles[i];
        if(entrada == NULL){ //si falta la entrada, igual "reserva" espacio con ceros
            char buffer_ceros[tam_pagina];
            memset(buffer_ceros, 0, tam_pagina);
            fwrite(buffer_ceros,1,tam_pagina,f);
            continue;
        }

        if(entrada->es_ultimo_nivel){
            if(entrada->marco != -1 && entrada->esta_presente){
                int offset = entrada->marco * tam_pagina;
                fwrite((char*)memoria_principal->espacio + offset, 1, tam_pagina,f);
            }else{ //pagina no presente o sin marco
                char buffer_ceros[tam_pagina];
                memset(buffer_ceros,0,tam_pagina);
                fwrite(buffer_ceros,1,tam_pagina,f);
            }
        }else{//siguiente nivel
            dump_tabla_nivel_completo(f,entrada->sgte_nivel, nivel_actual + 1);
        }
    }
}

/* ------- + PROPUESTA by valucha ------- */

int inicializar_proceso(int pid, int tamanio){
    int paginas_necesarias = (tamanio + tam_pagina - 1) / tam_pagina;

    if(contar_marcos_libres() < paginas_necesarias) return -1;

    t_tabla_proceso* nueva_tabla = malloc(sizeof(t_tabla_proceso));
    nueva_tabla->pid = pid;
    nueva_tabla->tabla_principal = crear_tabla_principal();
    nueva_tabla->cantidad_paginas = paginas_necesarias;

    if (nueva_tabla->tabla_principal == NULL) {
        log_error(logger, "Error al crear tabla principal");
        free(nueva_tabla);
        return -1;
    }

    nueva_tabla->instrucciones = cargar_instrucciones_desde_archivo(path_instrucciones);
    if(nueva_tabla->instrucciones == NULL){
        log_error(logger,"Error al cargar instrucciones del proceso %d", pid);
        liberar_tabla_principal(nueva_tabla->tabla_principal);
        free(nueva_tabla);
        return -1;
    }

    list_add(memoria_principal->tablas_por_proceso, nueva_tabla);

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
    if(marco >= 0 && marco < memoria_principal->cantidad_marcos){
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
        int marco = asignar_marco_libre();
        if(marco == -1){
            free(tabla);
            return NULL;
        }
        tabla->marco = marco;
        tabla->esta_presente = true;
    }else{
        tabla->sgte_nivel = malloc(sizeof(Tabla_Nivel*)* entradas_por_tabla);
        for(int i = 0; i < entradas_por_tabla; i++){
            tabla->sgte_nivel[i] = crear_tabla_nivel((nivel_actual + 1),i);
            if(!tabla->sgte_nivel[i]){ //liberamos lo creado hasta ahora, si hay fallo en la rama
                for(int j = 0; j < i; j++){
                    liberar_tabla_nivel(tabla->sgte_nivel[j]);
                }
                free(tabla->sgte_nivel);
                free(tabla);
                return NULL;
            }
        }
    }

    return tabla;
}

struct Tabla_Principal* crear_tabla_principal(){
    Tabla_Principal* tabla = malloc(sizeof(Tabla_Principal));
    tabla->niveles = malloc(sizeof(Tabla_Nivel*)*entradas_por_tabla);

    for(int i = 0; i < entradas_por_tabla; i++){
        tabla->niveles[i] = crear_tabla_nivel(2,i);
        if(!tabla->niveles[i]){ //liberamos lo creado hasta ahora, si hay fallo en la rama
                for(int j = 0; j < i; j++){
                    liberar_tabla_nivel(tabla->niveles[j]);
                }
                free(tabla->niveles);
                free(tabla);
                return NULL;
            }
    }

    return tabla;
}

void liberar_tabla_nivel(Tabla_Nivel* tabla){
    if(tabla == NULL) return;

    if(tabla->es_ultimo_nivel){
        if(tabla->marco != -1){
            liberar_marco(tabla->marco);
        }
    }else{
        for(int i = 0; i < entradas_por_tabla; i++){
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

/* ------- PROPUESTA para Manejo de SWAP ------- */

void suspender_proceso(int pid){
    t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
    if(!proceso){
        log_error(logger, "Error al buscar el proceso con PID <%d>",pid);
        return;
    }

    FILE* f = fopen(path_swapfile,"rb+");
    if(!f){
        log_error(logger,"No se pudo abrir el archivo de SWAP");
        return;
    }

    int offset_en_paginas = list_size(memoria_principal->metadata_swap);
    int offset_en_bytes = offset_en_paginas * tam_pagina;

    for(int i = 0; i < proceso->cantidad_paginas; i++){
        int marco = obtener_marco_por_indice(proceso->tabla_principal, i);
        void* origen = memoria_principal->espacio + (marco * tam_pagina);

        fseek(f,offset_en_bytes + i * tam_pagina, SEEK_SET);
        fwrite(origen,1,tam_pagina,f);
        liberar_marco(marco);
    }

    t_swap* nueva_entrada = malloc(sizeof(t_swap));
    nueva_entrada->pid = pid;
    nueva_entrada->pagina_inicio = offset_en_paginas;
    nueva_entrada->cantidad_paginas = proceso->cantidad_paginas;

    list_add(memoria_principal->metadata_swap, nueva_entrada);

    fclose(f);

    log_info(logger,"## PID: <%d> - Proceso suspendido, páginas guardadas en SWAP",pid);

}

void des_suspender_proceso(int pid){
    t_tabla_proceso* proceso = buscar_proceso_por_pid(pid);
    if(!proceso){
        log_error(logger, "Error al buscar el proceso con PID <%d>",pid);
        return;
    }

    t_swap* entrada = NULL;

    for(int i = 0; i < list_size(memoria_principal->metadata_swap);i++){
        t_swap* entrada_buscada = list_get(memoria_principal->metadata_swap,i);
        if(entrada_buscada->pid == pid){
            entrada = entrada_buscada;
            list_remove(memoria_principal->metadata_swap,i);
            break;
        }
    }

    if(!entrada){
        log_error(logger,"No se encontró al proceso con pid %d en SWAP", pid);
        return;
    }

    FILE* f = fopen(path_swapfile,"rb+");
    if(!f){
        log_error(logger,"No se pudo abrir el archivo de SWAP");
        free(entrada);
        return;
    }

    for(int i = 0; i < entrada->cantidad_paginas ;i++){
        int marco = asignar_marco_libre();
        if(marco == -1){
            log_error(logger,"No hay marcos disponibles para des-suspender al proceso con pid %d", pid);
            break;
        }

        fseek(f, (entrada->pagina_inicio + i) * tam_pagina, SEEK_SET);
        void* destino = memoria_principal->espacio + marco * tam_pagina;
        fread(destino,1,tam_pagina,f);

        marcar_marco_en_tabla(proceso->tabla_principal,i,marco);
    }
    fclose(f);
    free(entrada);
    log_info(logger,"## PID: <%d> - Proceso des-suspendido desde SWAP", pid);
}

int obtener_marco_por_indice(Tabla_Principal* tabla, int nro_pagina_logica){
    return 1;
}

void marcar_marco_en_tabla(t_tabla_proceso* proceso,int nro_pagina_logica,int marco){
    return;
}

