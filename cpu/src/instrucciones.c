#include <instrucciones.h>
#include <math.h>
#include "../../kernel/src/pcb.h"
extern t_log *logger;

list_struct_t * cola_interrupciones;
extern sem_t * sem_dispatch;

extern int socket_interrupt, socket_dispatch, socket_memoria;

int flag_hay_interrupcion = false;

/* ------ TLB ------ */
extern int entradas_tlb;
extern char * reemplazo_tlb;
extern TLB_t * TLB_tabla;

int reloj_lru = 0;
int fifo_index = 0;

// int cant_ocupada_TLB = 0;
 
/* ------ CACHÉ ------ */
extern int entradas_cache;
extern char * reemplazo_cache;
extern int retardo_cache;

extern cache_t* cache;

extern int puntero_cache;

int pagina_actual_cache = -1;
int marco_actual_cache = -1;

/* ------------------ */

int pid;
int pc;
t_paquete * paquete_send;
int conexion;
bool pc_actualizado = false;

//variables para MMU
int tam_pag, cant_niv, entradas_x_tabla;

void* ciclo_instruccion(void * arg){
    char * instrSTR;
    instruccion_t *instr;

    sem_wait(sem_dispatch);

    while ((1)){
        while (!flag_hay_interrupcion){
            instrSTR = Fetch();
            instr = Decode(instrSTR);
            Execute(instr);
            Actualizar_pc();
        }
        Check_Int();
    }
    return (void *)EXIT_SUCCESS;
};

int instrStringMap(char opcodeStr []){
    int opCode;

    if (strcmp(opcodeStr,"IO") == 0) {
        opCode = IO_I;
    }
    else if (strcmp(opcodeStr,"INIT_PROC") == 0) {
        opCode = INIT_PROC_I;
    }
    else if (strcmp(opcodeStr,"DUMP_MEMORY") == 0) {
        opCode = DUMP_MEMORY_I;
    }
    else if (strcmp(opcodeStr,"EXIT") == 0) {
        opCode = EXIT_I;
    }
    else if (strcmp(opcodeStr,"NOOP") == 0) {
        opCode = NOOP_I;
    }
    else if (strcmp(opcodeStr,"WRITE") == 0) {
        opCode = WRITE_I;
    }
    else if (strcmp(opcodeStr,"READ") == 0) {
        opCode = READ_I;
    }
    else if (strcmp(opcodeStr,"GOTO") == 0) {
        opCode = GOTO_I;
    }
    return opCode;
};

char * Fetch(){ // Le pasa la intruccion completa 
    char * InstruccionCompleta = "NOOP"; // Para cambiar en el futuro por el valor posta
    
    t_paquete* paquete_send = crear_paquete(PEDIR_INSTRUCCION);
    agregar_a_paquete(paquete_send, &pid, sizeof(int));
    agregar_a_paquete(paquete_send, &pc, sizeof(int));

    enviar_paquete(paquete_send, socket_memoria);

    //paquete enviado
    log_info(logger,"## PID: %d PC: %d- FETCH ",pid,pc);

    //respuesta DEVOLVER_INSTRUCCION
    protocolo_socket cod_op = recibir_operacion(socket_memoria);

    if (cod_op == DEVOLVER_INSTRUCCION){
        t_list * paquete_recv;
        paquete_recv = recibir_paquete(socket_memoria);

        // int pid =  *(int *)list_remove(paquete_recv, 0);
        // int pc  =  *(int *)list_remove(paquete_recv, 0);
        InstruccionCompleta = list_remove(paquete_recv, 0);
        log_debug(logger, "FETCH recibió instrucción: %s", InstruccionCompleta);
        // etc
        // list_destroy(paquete_recv);
        
        // eliminar_paquete(paquete_send);
        list_destroy_and_destroy_elements(paquete_recv,free); 
    };
    eliminar_paquete(paquete_send);
    
    return InstruccionCompleta ;
};

instruccion_t *Decode(char * instr){
    instruccion_t *current_instr = malloc(sizeof(instruccion_t));
    current_instr->data = string_split(instr, " ");
    current_instr->opCode = instrStringMap(current_instr->data[0]);


    switch (current_instr->opCode){
        case IO_I:
                current_instr->tipo = SYSCALL;
        break;
        case INIT_PROC_I:
                current_instr->tipo = SYSCALL;       
        break;
        case DUMP_MEMORY_I:
                current_instr->tipo = SYSCALL;
        break;
        case EXIT_I:
                current_instr->tipo = SYSCALL;
        break;
        case NOOP_I:
                current_instr->tipo = USUARIO;
        break;
        case WRITE_I:
                current_instr->tipo = USUARIO;
        break;
        case READ_I:
                current_instr->tipo = USUARIO;
        break;
        case GOTO_I:
                current_instr->tipo = USUARIO;
        break;
        default:
             log_info(logger, "Instrucción no reconocida: %s", *current_instr->data);
             exit(EXIT_FAILURE);
                break;
        }
        if (current_instr->opCode == IO_I || current_instr->opCode == INIT_PROC_I || current_instr->opCode ==  WRITE_I ||  current_instr->opCode ==  READ_I){
            if(!current_instr->data[0] || !current_instr->data[1]){
                log_info(logger, "Instrucción no tiene los 2 parametros: %s", *current_instr->data);
                exit(EXIT_FAILURE);
            }
        } else if (current_instr->opCode == GOTO_I && !current_instr->data[0]){
                log_info(logger, "Instrucción no tiene el parametro: %s", *current_instr->data);
                exit(EXIT_FAILURE);
        };
        log_debug(logger, "Instrucción Decodiada: %s (%d)  SYSCALL TIPO: %d",current_instr->data[0] ,current_instr->opCode,current_instr->tipo);
        return current_instr;    
};

void Execute(instruccion_t *instr){
    log_debug(logger, "Instrucción ejecutada: %d INSTRUCCION TIPO: %d", instr->opCode,instr->tipo);

    switch (instr->opCode){
            case NOOP_I:
                noop();
            break;
            case GOTO_I:
                log_debug(logger, "GOTO: salta a línea %s", instr->data[1]);
                goto_(atoi(instr->data[1]));
                
            break;
            case WRITE_I:
                write_(atoi(instr->data[1]) , instr->data[2]);
            break;
            case READ_I:
                read_(atoi(instr->data[1]) , atoi(instr->data[2]));
            break;
            //----- SYSCALLS
            case IO_I:
                io(instr->data[1],atoi(instr->data[2]));
            break;
            case INIT_PROC_I:
                init_proc(instr->data[1],atoi(instr->data[2]));
            break;

            case DUMP_MEMORY_I:
                dump_memory();
            break;

            case EXIT_I:
                exit_();
            break;
        default:
            log_info(logger, "Error al ejecutar la instruccion: %s", *instr->data);
            exit(EXIT_FAILURE);
            break;
        }
};

void Check_Int(){

    log_debug(logger, "Entro a check interrupt");

    t_paquete * paquete_send;
    interrupcion_t *interrupcion = desencolar_interrupcion_generico(cola_interrupciones);

    if(interrupcion == NULL){
        return;
    }

    

    pthread_mutex_lock(cola_interrupciones->mutex);
    if(!list_is_empty(cola_interrupciones->lista)&&((interrupcion->tipo == DISPATCH_CPU_I)||(interrupcion->tipo == DESALOJO_I))){
        pc--;
    }
    pthread_mutex_unlock(cola_interrupciones->mutex);

    switch(interrupcion->tipo){
        
        case DISPATCH_CPU_I:
            enviar_paquete_ok(socket_interrupt);
            pc = interrupcion->pc;
            pid = interrupcion->pid;
        break;

        case DESALOJO_I:
            paquete_send = crear_paquete(DESALOJO_CPU);
            agregar_a_paquete (paquete_send, &interrupcion->pid, sizeof(int));
            agregar_a_paquete (paquete_send, &pc, sizeof(int));
            pc = interrupcion->pc;
            pid = interrupcion->pid;
        break;
            
        case IO_I:
            log_info(logger, "## PID: %d - Ejecutando: IO - Nombre dispositivo: %s, tiempo: %d", pid, interrupcion->paramstring, interrupcion->param1);
            paquete_send = crear_paquete(IO_CPU);
            agregar_a_paquete (paquete_send, &pid, sizeof(int));
            agregar_a_paquete (paquete_send, &pc, sizeof(int));
            agregar_a_paquete (paquete_send, interrupcion->paramstring, strlen(interrupcion->paramstring)+1);
            agregar_a_paquete (paquete_send, &interrupcion->param1, sizeof(int));
        break;

        case INIT_PROC_I:
            log_info(logger, "## PID: %d - Ejecutando: INIT_PROC - Nombre archivo: %s, tamaño: %d", pid, interrupcion->paramstring, interrupcion->param1);
            paquete_send = crear_paquete(PROCESS_INIT_CPU);
            agregar_a_paquete (paquete_send, &pid, sizeof(int));
            agregar_a_paquete (paquete_send, &pc, sizeof(int));
            agregar_a_paquete (paquete_send, interrupcion->paramstring, strlen(interrupcion->paramstring)+1);
            agregar_a_paquete (paquete_send, &interrupcion->param1, sizeof(int));
        break;

        case DUMP_MEMORY_I:
            log_info(logger, "## PID: %d - Ejecutando: DUMP_MEMORY", pid);
            paquete_send = crear_paquete(DUMP_MEM_CPU);
            agregar_a_paquete (paquete_send, &pid, sizeof(int));
            agregar_a_paquete (paquete_send, &pc, sizeof(int));
            // agregar_a_paquete (paquete_send, interrupcion->paramstring, strlen(interrupcion->paramstring)+1);
        break;

        case EXIT_I:
            log_info(logger, "## PID: %d - Ejecutando: PROCESS_EXIT", pid);
            paquete_send = crear_paquete(PROCESS_EXIT_CPU);
            agregar_a_paquete (paquete_send, &pid, sizeof(int));
        break;

    }

    vaciar_cola_interrupcion(cola_interrupciones);
    
    if((interrupcion->tipo != DISPATCH_CPU_I)&&(interrupcion->tipo != DESALOJO_I)){
        enviar_paquete(paquete_send, socket_interrupt);
        eliminar_paquete(paquete_send);
        sem_wait(sem_dispatch);
    }

    free(interrupcion);


};

void write_(int dir_logica , char * datos){
    int nro_pagina;
    int offset; //como se obtiene el offset aca? no entiendo
    traducir_DL(dir_logica,&nro_pagina,&offset);

    int marco = obtener_marco(pid,nro_pagina,offset);
    // int dir_fisica = obtener_DF(marco,offset);

    log_debug(logger, "Ejecutando WRITE: dir_logica=%d, valor=%s", dir_logica, datos);

    //Actualizar caché (si hay)
    if(entradas_cache > 0){
        pagina_actual_cache = nro_pagina;
        marco_actual_cache = marco;

        escribir_en_cache(pid,datos,nro_pagina);

        return;
    }
};

void read_(int dir_logica , int tamanio){
    int nro_pagina;
    int offset;
    traducir_DL(dir_logica,&nro_pagina,&offset);

    char * valor ;

    //Consultamos la Caché
    if(entradas_cache > 0 && buscar_en_cache(pid,nro_pagina,&valor)){
        log_info(logger,"PID: <%d> - Cache Hit - Pagina: <%d>", pid, nro_pagina);
        log_info(logger, "PID: <%d> - Acción: <LEER> - Dirección Lógica: <%d> - Valor: <%s>", pid, dir_logica, valor);
        free(valor);
        return;
    }

    log_info(logger,"PID: <%d> - Cache Miss - Pagina: <%d>", pid, nro_pagina);

    //Obtenemos marco desde TLB
    int marco = obtener_marco(pid,nro_pagina,offset);
    int dir_fisica = obtener_DF(marco,offset);

    //Enviamos a memoria
    t_paquete* paquete_send = crear_paquete(ACCEDER_A_ESPACIO_USUARIO);
    acceso_t tipo_de_acceso = LECTURA_AC;

    agregar_a_paquete(paquete_send, &pid, sizeof(int));
    agregar_a_paquete(paquete_send, &tamanio, sizeof(int));
    agregar_a_paquete(paquete_send, &dir_fisica, sizeof(int));
    agregar_a_paquete(paquete_send, &tipo_de_acceso, sizeof(int));

    enviar_paquete(paquete_send, socket_memoria);
    eliminar_paquete(paquete_send);

    protocolo_socket cod_op = recibir_operacion(socket_memoria);
    if(cod_op != DEVOLVER_VALOR){
        log_error(logger,"No se pudo obtener el valor desde memoria");
        return;
    }

    t_list* respuesta = recibir_paquete(socket_memoria);
    valor = list_remove(respuesta,0);
    list_destroy_and_destroy_elements(respuesta, free);


    log_info(logger, "PID: <%d> - Acción: <LEER> - Dirección Física: <%d> - Valor: <%s>", pid, dir_fisica, valor);

    //Actualizamos la caché
    if(entradas_cache > 0)escribir_en_cache(pid,valor,nro_pagina);

    free(valor);

};

void noop(){
    log_info(logger, "Instrucción Ejecutada: ## PID: %d - Ejecutando: NOOP :",pid);
};

void goto_(int nuevo_pc){
    pc = nuevo_pc;
    pc_actualizado = true;
    log_info(logger, "Instrucción Ejecutada: ## PID: %d - Ejecutando: GOTO :",pid);
};


//--- SYSCALLS
void io(char * Dispositivo, int tiempo){ // (Dispositivo, Tiempo)  ESTA LA HACE EL KERNEL, ACA ES REPRESENTATIVO
    interrupcion_t * interrupcion = malloc(sizeof(interrupcion_t));
    interrupcion->tipo = IO_I;
    interrupcion->paramstring = Dispositivo;
    interrupcion->param1 = tiempo;

    encolar_interrupcion_generico(cola_interrupciones, interrupcion, -1);
};
void init_proc(char * archivo, int tamaño){ //(Archivo de instrucciones, Tamaño) ESTA LA HACE EL KERNEL, ACA ES REPRESENTATIVO
    interrupcion_t * interrupcion = malloc(sizeof(interrupcion_t));
    interrupcion->tipo = INIT_PROC_I;
    interrupcion->paramstring = archivo;
    interrupcion->param1 = tamaño;

    encolar_interrupcion_generico(cola_interrupciones, interrupcion, -1);
};
void dump_memory(){ // ESTA LA HACE EL KERNEL, ACA ES REPRESENTATIVO
    interrupcion_t * interrupcion = malloc(sizeof(interrupcion_t));
    interrupcion->tipo = DUMP_MEMORY_I;

    encolar_interrupcion_generico(cola_interrupciones, interrupcion, -1);
};
void exit_(){ // ESTA LA HACE EL KERNEL, ACA ES REPRESENTATIVO
    interrupcion_t * interrupcion = malloc(sizeof(interrupcion_t));
    interrupcion->tipo = EXIT_I;
    
    encolar_interrupcion_generico(cola_interrupciones, interrupcion, -1);
};

void Actualizar_pc(){
    if (pc_actualizado == false){
        pc++;
    } else{
       pc_actualizado = false;
    }
};

/* ------ MMU ------ */

void traducir_DL(int dir_logica, int* nro_pagina, int* offset){
    *nro_pagina = dir_logica / tam_pag;
    *offset = dir_logica % tam_pag;
}

int obtener_DF(int marco, int offset){
    return marco * tam_pag + offset;
}

/* ------ TLB ------ */

int obtener_marco(int pid, int nro_pagina,int offset){
    
    // 1. Consultar la TLB (si está habilitada)

    if (entradas_tlb > 0) {
        int marco = buscar_en_tlb(pid, nro_pagina);
        if (marco != -1) {
            log_info(logger, "TLB HIT - PID <%d> - Página <%d> - Marco <%d>", pid, nro_pagina, marco);
            return marco;
        } else {
            log_info(logger, "TLB MISS - PID <%d> - Página <%d>", pid, nro_pagina);
        }
    }

    // 2. Calcular índices para acceso a TDP

    int* indices = malloc(sizeof(int) * cant_niv);
    int temp_pagina = nro_pagina;
    for (int i = cant_niv - 1; i >= 0; i--) {
        indices[i] = temp_pagina % entradas_x_tabla;
        temp_pagina /= entradas_x_tabla;
    }

    // 3. Armar y enviar el paquete a memoria

    t_paquete* paquete_send = crear_paquete(ACCEDER_A_TDP);
    agregar_a_paquete(paquete_send, &pid, sizeof(int));
    for (int i = 0; i < cant_niv; i++)
        agregar_a_paquete(paquete_send, &indices[i], sizeof(int));

    enviar_paquete(paquete_send, socket_memoria);
    eliminar_paquete(paquete_send);
    free(indices);

    // 4. Recibir el marco

    protocolo_socket op = recibir_operacion(socket_memoria);
    t_list* paquete_recv = recibir_paquete(socket_memoria);
    int marco = *(int*)list_remove(paquete_recv, 0);
    list_destroy_and_destroy_elements(paquete_recv, free);

    if (marco == -1) {
        log_error(logger, "No se pudo traducir dirección: marco no presente");
        return -1;
    }

    // 5. Actualizar TLB
    if (entradas_tlb > 0) {
        agregar_a_tlb(pid, nro_pagina, marco);
    }

    log_info(logger,"PID: <%d> - OBTENER MARCO - Página: <%d> - Marco: <%d>",pid,nro_pagina,marco);

    return marco;
}

int buscar_en_tlb(int pid_actual, int pagina_buscada){
    for(int i = 0; i < entradas_tlb; i++){
        if(TLB_tabla[i].pid == pid_actual && TLB_tabla[i].ocupado && TLB_tabla[i].pagina == pagina_buscada){
            if(strcmp(reemplazo_tlb, "LRU") == 0){
                TLB_tabla[i].timestamp = get_timestamp();
            }
            return TLB_tabla[i].marco; //TLB Hit
        }
    }
    return -1; //TLB Miss
}

int get_timestamp() {
    return reloj_lru++;
}

void agregar_a_tlb(int pid_actual, int pagina, int marco){
    // Buscar espacio libre
    for (int i = 0; i < entradas_tlb; i++) {
        if (!TLB_tabla[i].ocupado) {
            TLB_tabla[i].pid = pid_actual;
            TLB_tabla[i].pagina = pagina;
            TLB_tabla[i].marco = marco;
            TLB_tabla[i].ocupado = true;
            TLB_tabla[i].timestamp = get_timestamp();
            return;
        }
    }

    // Si no hay lugar, aplicar reemplazo
    int victima = 0;
    if (strcmp(reemplazo_tlb, "FIFO") == 0) {
        victima = buscar_victima_FIFO(); // mantenés un puntero o índice global
    } else if (strcmp(reemplazo_tlb, "LRU") == 0) {
        victima = buscar_victima_LRU(); // comparás timestamps
    }

    TLB_tabla[victima].pid = pid_actual;
    TLB_tabla[victima].pagina = pagina;
    TLB_tabla[victima].marco = marco;
    TLB_tabla[victima].ocupado = true;
    TLB_tabla[victima].timestamp = get_timestamp();
}

int buscar_victima_LRU(){
    int min = TLB_tabla[0].timestamp;
    int pos = 0;

    for(int i = 1; i < entradas_tlb;i++){
        if(TLB_tabla[i].timestamp < min){
            min = TLB_tabla[i].timestamp;
            pos = i;
        }
    }
    return pos;
}

int buscar_victima_FIFO(){
    int victima = fifo_index;
    fifo_index = (fifo_index + 1) % entradas_tlb;
    return victima;
}

void limpiar_entradas_tlb(int pid_a_eliminar){
    for(int i = 0; i < entradas_tlb; i++){
        if(TLB_tabla[i].pid == pid_a_eliminar && TLB_tabla[i].ocupado){
            TLB_tabla[i].ocupado = false;
        }
    }
}

/* ------ CACHÉ ------ */

int buscar_en_cache(int pid_actual, int nro_pagina, char** contenido_out){
    if(entradas_cache <= 0)return 0;

    for(int i = 0; i < entradas_cache; i++){
        if(cache[i].pid == pid_actual && cache[i].nro_pagina == nro_pagina && cache[i].ocupado){
            usleep(retardo_cache * 1000);
            cache[i].uso = 1;
            *contenido_out = cache[i].contenido;
            return 1;
        }
    }
    
    return 0;
}

void escribir_en_cache(int pid_actual, char * nuevo_valor, int nro_pagina){
    if(entradas_cache <= 0)return;

    //Verificamos, si ya existe => modificamos
    for(int i = 0; i < entradas_cache; i++){
        if(cache[i].pid == pid_actual && cache[i].nro_pagina == nro_pagina && cache[i].ocupado){
            usleep(retardo_cache * 1000);

            free(cache[i].contenido);
            cache[i].contenido = strdup(nuevo_valor);

            cache[i].uso = 1;
            cache[i].modificado = 1;

            return;
        }
    }

    //Sino => Buscamos espacio libre
    for(int i = 0 ; i < entradas_cache ; i++){
        if(!cache[i].ocupado){
            usleep(retardo_cache * 1000);
            cache[i] = (cache_t){pid_actual,nro_pagina,strdup(nuevo_valor),1,1,1};

            log_info(logger,"PID: <%d> - Cache Add - Pagina: <%d>", pid_actual, nro_pagina);
            return;
        }
    }

    //Si no hay espacio libre => reemplazamos
    int victima = (strcmp(reemplazo_cache,"CLOCK") == 0) ? reemplazo_clock() : reemplazo_clock_M();


    //Si la victima está modificada => escribir en memoria
    if(cache[victima].modificado){
        escribir_cache_en_memoria(cache[victima]);
    }
    free(cache[victima].contenido);

    //Reemplazamos
    usleep(retardo_cache * 1000);
    cache[victima] = (cache_t){pid_actual, nro_pagina,strdup(nuevo_valor),1,1,1};

    log_info(logger,"PID: <%d> - Cache Add - Pagina: <%d>", pid_actual, nro_pagina);
    
    return;
}

//Algoritmos de reemplazo
//CLOCK
int reemplazo_clock(){
    while(1){
        if(!cache[puntero_cache].uso){
            int posicion = puntero_cache;
            puntero_cache = (puntero_cache + 1) % entradas_cache;
            return posicion;
        }else{
            cache[puntero_cache].uso = 0;
            puntero_cache = (puntero_cache + 1) % entradas_cache;
        }
    }
}

//CLOCK-M
int reemplazo_clock_M(){
    //Primera vuelta => u = 0  &&  m = 0
    for(int i = 0; i < entradas_cache; i++){
        int index = (puntero_cache + i) % entradas_cache;
        if(!cache[index].uso && !cache[index].modificado){
            return avanzar_puntero(index);
        }
    }

    //Segunda vuelta => u = 0  &&  m = 1
    for(int i = 0; i < entradas_cache; i++){
        int index = (puntero_cache + i) % entradas_cache;
        if(!cache[index].uso && cache[index].modificado){
            return avanzar_puntero(index);
        }
        cache[index].uso = 0;
    }

    //Reintentamos despues de cambiar bits de uso
    return reemplazo_clock_M();
}

int avanzar_puntero(int index){
    puntero_cache = (index + 1) % entradas_cache;
    return index;
}


// + funciones auxiliares para caché
void escribir_cache_en_memoria(cache_t entrada){
    //Obtenemos marco y dir_fisica para escribir en memoria
    int marco = obtener_marco(entrada.pid,entrada.nro_pagina,0);
    int dir_fisica = obtener_DF(marco, 0); // Offset = 0 , porque escribimos la página entera

    log_info(logger,"PID: <%d> - Acción: <ESCRIBIR> - Dirección Física: <%d> - Valor: <%s>", entrada.pid, dir_fisica, entrada.contenido);

    t_paquete* paquete_send = crear_paquete(ACCEDER_A_ESPACIO_USUARIO);
    int tam = tam_pag;
    int tipo_de_acceso = ESCRITURA_AC;

    agregar_a_paquete(paquete_send, &entrada.pid , sizeof(int));
    agregar_a_paquete(paquete_send, &tam , sizeof(int));
    agregar_a_paquete(paquete_send, &dir_fisica , sizeof(int));
    agregar_a_paquete(paquete_send, &tipo_de_acceso , sizeof(int));
    agregar_a_paquete(paquete_send, entrada.contenido , strlen(entrada.contenido)+1);


    enviar_paquete(paquete_send, socket_memoria);
    eliminar_paquete(paquete_send);

    protocolo_socket cod_op = recibir_paquete_ok(socket_memoria);
    if(cod_op != OK){
        log_error(logger,"No se pudo escribir la página modificada al desalojar la caché");
    }

    log_info(logger,"PID: <%d> - Memory Update - Página: <%d> - Frame: <%d>",entrada.pid, entrada.nro_pagina, marco);

}

void limpiar_cache_de_proceso(int pid_a_eliminar){
    for(int i = 0; i < entradas_cache ; i++){
        if(cache[i].ocupado && cache[i].pid == pid_a_eliminar){
            if(cache[i].modificado){
                escribir_cache_en_memoria(cache[i]);
            }
            cache[i].ocupado = 0;
            cache[i].uso = 0;
            cache[i].modificado = 0;
            free(cache[i].contenido);
        }
    }
}
