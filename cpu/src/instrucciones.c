#include <instrucciones.h>
#include <math.h>
#include "../../kernel/src/pcb.h"
extern t_log *logger;

extern int socket_interrupt, socket_dispatch, socket_memoria;

extern int estradas_tlb;
extern char * remplazo_tlb;
 
extern int entradas_cache;
extern char * remplazo_cache;
extern int retrardo_cache;

int pid;
int pc;
t_paquete * paquete_send;
int conexion;
bool pc_actualizado = false;

//variables para MMU
int tam_pag, cant_niv, entradas_x_tabla;

void* ciclo_instruccion(void * arg){
    char * instrSTR;
    instruccion_t instr;
        while ((1)){
            // while (!flag_hay_interrupcion){
                instrSTR = Fetch();
                instr = Decode(instrSTR);
                Execute(instr);
                Actualizar_pc();
            // }
            Check_Int();
        }
        return (void *)EXIT_SUCCESS;
};

int instrStringMap(char opcodeStr []){
    int opCode;

    if (strcmp(opcodeStr,"IO") == 0) {
        opCode = IO;
    }
    else if (strcmp(opcodeStr,"INIT_PROC") == 0) {
        opCode = INIT_PROC;
    }
    else if (strcmp(opcodeStr,"DUMP_MEMORY") == 0) {
        opCode = DUMP_MEMORY;
    }
    else if (strcmp(opcodeStr,"EXIT") == 0) {
        opCode = EXIT_I;
    }
    else if (strcmp(opcodeStr,"NOOP") == 0) {
        opCode = NOOP;
    }
    else if (strcmp(opcodeStr,"WRITE") == 0) {
        opCode = WRITE_I;
    }
    else if (strcmp(opcodeStr,"READ") == 0) {
        opCode = READ_I;
    }
    else if (strcmp(opcodeStr,"GOTO") == 0) {
        opCode = GOTO;
    }
    return opCode;
};

char * Fetch(){ // Le pasa la intruccion completa 
    char * InstruccionCompleta = "NOOP"; // Para cambiar en el futuro por el valor posta
    
    // t_list *paquete_recv;
    
    // t_paquete *paquete_send;

    // paquete_recv = recibir_paquete(conexion);
    // pid = *(int *)list_remove(paquete_recv, 0);
    // pc = *(int *)list_remove(paquete_recv,0); //no entiendo muy bien como es esto...


    t_paquete* paquete_send = crear_paquete(PEDIR_INSTRUCCION);
    agregar_a_paquete(paquete_send, &pid, sizeof(int));
    agregar_a_paquete(paquete_send, &pc, sizeof(int));

    enviar_paquete(paquete_send, conexion);

    //paquete enviado

    //respuesta DEVOLVER_INSTRUCCION
    protocolo_socket cod_op = recibir_operacion(socket_memoria); //

    t_list * paquete_recv;
    paquete_recv = recibir_paquete(socket_memoria);

    int pid =  *(int *)list_remove(paquete_recv, 0);
    int pc  =  *(int *)list_remove(paquete_recv, 0);
    // etc
    eliminar_paquete(paquete_send);
    // list_destroy(paquete_recv);

    // eliminar_paquete(paquete_send);
    list_destroy_and_destroy_elements(paquete_recv,free); 

    log_info(logger,"## PID: %d PC: %d- FETCH ",pid,pc);
    return InstruccionCompleta ;
};

instruccion_t Decode(char * instr){
    instruccion_t current_instr;
    current_instr.data = string_split(instr, " ");
    current_instr.opCode = instrStringMap(current_instr.data[0]);


    switch (current_instr.opCode){
        case IO:
                current_instr.tipo = SYSCALL;
        break;
        case INIT_PROC:
                current_instr.tipo = SYSCALL;       
        break;
        case DUMP_MEMORY:
                current_instr.tipo = SYSCALL;
        break;
        case EXIT_I:
                current_instr.tipo = SYSCALL;
        break;
        case NOOP:
                current_instr.tipo = USUARIO;
        break;
        case WRITE_I:
                current_instr.tipo = USUARIO;
        break;
        case READ_I:
                current_instr.tipo = USUARIO;
        break;
        case GOTO:
                current_instr.tipo = USUARIO;
        break;
        default:
             log_info(logger, "Instrucción no reconocida: %s", *current_instr.data);
             exit(EXIT_FAILURE);
                break;
        }
        if (current_instr.opCode == IO || current_instr.opCode == INIT_PROC || current_instr.opCode ==  WRITE_I ||  current_instr.opCode ==  READ_I){
            if(!current_instr.data[0] || !current_instr.data[1]){                 
                log_info(logger, "Instrucción no tiene los 2 parametros: %s", *current_instr.data);
                exit(EXIT_FAILURE);
            }
        } else if (current_instr.opCode == GOTO && !current_instr.data[0]){
                log_info(logger, "Instrucción no tiene el parametro: %s", *current_instr.data);
                exit(EXIT_FAILURE);
        };
        log_debug(logger, "Instrucción Decodiada: %s (%d)  SYSCALL TIPO: %d",current_instr.data[0] ,current_instr.opCode,current_instr.tipo);
        return current_instr;    
};

void Execute(instruccion_t instr){
    log_debug(logger, "Instrucción ejecutada: %d SYSCALL TIPO: %d", instr.opCode,instr.tipo);

    switch (instr.opCode){
            case NOOP:
                noop();
            break;
            case GOTO:
                goto_(atoi(instr.data[0]));
            break;
            case WRITE_I:
                write_(atoi(instr.data[1]) , atoi(instr.data[0]));
            break;
            case READ_I:
                read_(atoi(instr.data[1]) , atoi(instr.data[0]));
            break;
            //----- SYSCALLS
            case IO:
                io(instr.data[0],atoi(instr.data[1]));
            break;
            case INIT_PROC:
                init_proc(instr.data[0],atoi(instr.data[1]));
            break;

            case DUMP_MEMORY:
                dump_memory();
            
            break;
            case EXIT_I:
                exit_();
            break;
        default:
            log_info(logger, "Error al ejecutar la instruccion: %s", *instr.data);
            exit(EXIT_FAILURE);
            break;
        }
};

void Check_Int(){

};

void write_(int dir_logica , int datos){
    uint32_t dir_fisica = MMU(dir_logica);

    t_paquete* paquete_send = crear_paquete(ACCEDER_A_ESPACIO_USUARIO);
    
    int tamanio = sizeof(int);
    int tipo_de_acceso = 1; // 1 = escritura 

    agregar_a_paquete(paquete_send, &pid, sizeof(int));
    agregar_a_paquete(paquete_send, &tamanio, sizeof(int));
    agregar_a_paquete(paquete_send, &dir_fisica, sizeof(uint32_t)); //cambiar en memoria el tipo int -> uint32_t
    agregar_a_paquete(paquete_send, &tipo_de_acceso, sizeof(int));
    agregar_a_paquete(paquete_send, &datos, sizeof(int));

    enviar_paquete(paquete_send,socket_memoria);
    eliminar_paquete(paquete_send);

    //Espera de OK
    protocolo_socket cod_op = recibir_operacion(socket_memoria);
    if(cod_op != OK){
        log_error(logger,"Memoria no confirmó escritura.");
        return;
    }


    log_info(logger, "PID: <%d> - Acción: <ESCRIBIR> - Dirección Física: <%d> - Valor: <%d>",pid,dir_fisica,datos);

};

void read_(int dir_logica , int tamanio){
    int dir_fisica = MMU(dir_logica);

    t_paquete * paquete_send;

    paquete_send = crear_paquete(READ_MEM); //Falta hacer al traduccion

    int tipo_de_acceso = 0; // 0 = lectura

    agregar_a_paquete(paquete_send, &pid,sizeof(int));
    agregar_a_paquete(paquete_send, &tamanio, sizeof(int));
    agregar_a_paquete(paquete_send, &dir_fisica, sizeof(int));
    agregar_a_paquete(paquete_send, &tipo_de_acceso, sizeof(int));

    enviar_paquete(paquete_send,socket_memoria);
    eliminar_paquete(paquete_send);

    //Recibir valor
    protocolo_socket cod_op = recibir_operacion(socket_memoria);
    if(cod_op != DEVOLVER_VALOR){
        log_error(logger,"No se pudo obtener el valor desde memoria");
        return;
    }

    t_list* respuesta = recibir_paquete(socket_memoria);
    int valor = *(int*)list_remove(respuesta,0);
    list_destroy_and_destroy_elements(respuesta,free);

    log_info(logger, "PID: <%d> - Acción: <LEER> - Dirección Física: <%d> - Valor: <%d>",pid,dir_fisica,valor);

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
    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: IO ”:",pid);
};
void init_proc(char * Dispositivo, int tamanno){ //(Archivo de instrucciones, Tamaño) ESTA LA HACE EL KERNEL, ACA ES REPRESENTATIVO
    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: INIT_PROC ”:",pid);
};
void dump_memory(){ // ESTA LA HACE EL KERNEL, ACA ES REPRESENTATIVO
    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: DUMP_MEMORY ”:",pid);
};
void exit_(){ // ESTA LA HACE EL KERNEL, ACA ES REPRESENTATIVO
    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: EXIT ”:",pid);
};

void Actualizar_pc(){
    if (pc_actualizado == false){
        pc++;
    } else{
       pc_actualizado = false;
    }
};

void recibir_valores_memoria(int socket_memoria){
    t_paquete* paquete_send = crear_paquete(ENVIAR_VALORES);
    enviar_paquete(paquete_send,socket_memoria);
    eliminar_paquete(paquete_send);

    protocolo_socket cod_op = recibir_operacion(socket_memoria);
    if(cod_op != ENVIAR_VALORES){
        log_error(logger,"Error al recibir valores desde memoria");
        return;
    }

    t_list* paquete_recv = recibir_paquete(socket_memoria);

    int* tam_pagina_ptr = list_remove(paquete_recv,0);
    int* cant_niveles_ptr = list_remove(paquete_recv,0);
    int* entradas_por_tabla_ptr = list_remove(paquete_recv,0);

    tam_pag = *tam_pagina_ptr;
    cant_niv = *cant_niveles_ptr;
    entradas_x_tabla = *entradas_por_tabla_ptr;

    free(tam_pagina_ptr);
    free(cant_niveles_ptr);
    free(entradas_por_tabla_ptr);
    list_destroy(paquete_recv);
}

int MMU(int dir_logica){
    int nro_pagina = dir_logica / tam_pag;
    int offset = dir_logica % tam_pag;

    //Calculamos indices por nivel
    int* indices_por_nivel = malloc(sizeof(int) * cant_niv);
    for(int i = 0; i < cant_niv; i++){
        indices_por_nivel[i] = (nro_pagina / (int)pow(entradas_x_tabla,cant_niv - 1 - i)) % entradas_x_tabla;
    }

    //Armar paquete para pedir marco 
    t_paquete* paquete_send = crear_paquete(ACCEDER_A_TDP);
    agregar_a_paquete(paquete_send, &pid, sizeof(int));
    for(int i = 0; i < cant_niv; i++){
        agregar_a_paquete(paquete_send,&indices_por_nivel[i],sizeof(int));
    }

    enviar_paquete(paquete_send,socket_memoria);
    eliminar_paquete(paquete_send);
    free(indices_por_nivel);

    //Esperar rta
    t_paquete* paquete_recv = recibir_paquete(socket_memoria);
    int marco = *(int*)list_remove(paquete_recv,0);
    list_destroy_and_destroy_elements(paquete_recv,free);

    if(marco == -1){
        log_warning(logger,"Marco no presente para PID <%d>", pid);
        return -1;
    }

    int dir_fisica = (marco * tam_pag + offset);

    return dir_fisica;
};


int TLB(int Direccion){
    TLB_t * tabla = (TLB_t *) malloc(sizeof(TLB_t) * estradas_tlb);

    for (int i = 0; i < sizeof(tabla); i++){
        
    }
    
    return 0;
};
