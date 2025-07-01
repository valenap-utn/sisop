#include <instrucciones.h>
extern t_log *logger;

extern int socket_interrupt, socket_dispatch;

int pid;
int pc;

void* ciclo_instruccion(void * arg){
    char * instrSTR;
    instruccion_t instr;
        while ((1)){
            instrSTR = Fetch();
            instr = Decode(instrSTR);
            Execute(instr);
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
        opCode = EXIT;
    }
    else if (strcmp(opcodeStr,"NOOP") == 0) {
        opCode = NOOP;
    }
    else if (strcmp(opcodeStr,"WRITE") == 0) {
        opCode = WRITE;
    }
    else if (strcmp(opcodeStr,"READ") == 0) {
        opCode = READ;
    }
    else if (strcmp(opcodeStr,"GOTO") == 0) {
        opCode = GOTO;
    }
    return opCode;
};

char * Fetch(){
    char * opcode = "NOOP  "; // Para cambiar en el futuro por el valor posta
    int conexion = *(int *)args;

    t_paquete * paquete;
    // t_list *paquete_recv;
    
    // t_paquete *paquete_send;

    // paquete_recv = recibir_paquete(conexion);
    // pid = *(int *)list_remove(paquete_recv, 0);
    // pc = *(int *)list_remove(paquete_recv,0); //no entiendo muy bien como es esto...

    int dato_a_enviar = 1;
    paquete = crear_paquete(OBTENER_INSTRUCCION);
    agregar_a_paquete(paquete, dato_a_enviar, sizeof(int));

    enviar_paquete(paquete, socket_conexion_memoria);
    //paquete enviado

    //respuesta

    cod_op = recibir_operacion(socket_conexion_memoria);
    t_list * paquete_recv;
    paquete_recv = recibir_paquete(socket_conexion_memoria);
    int dato = *(int*)list_remove(paquete_recv, 0);
    int dato2 = list_remove(paquete_recv, 0);
    // etc
    list_destroy(paquete_recv);
    

    log_info(logger,"## PID: %d PC: %d- FETCH ",pid,pc);
    return opcode ;
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
        case EXIT:
                current_instr.tipo = SYSCALL;
        break;
        case NOOP:
                current_instr.tipo = USUARIO;
        break;
        case WRITE:
                current_instr.tipo = USUARIO;
        break;
        case READ:
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
        if (current_instr.opCode == IO || current_instr.opCode == INIT_PROC || current_instr.opCode ==  WRITE ||  current_instr.opCode ==  READ){
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
            case WRITE:
                write_(0 , 0); // Falta poner los valores posta
            break;
            case READ:
                read_(0,1);
            break;
            //----- SYSCALLS
            case IO:
                io();
            break;
            case INIT_PROC:
                init_proc();
            break;

            case DUMP_MEMORY:
                dump_memory();
            
            break;
            case EXIT:
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

void MMU(uint32_t*direccion){

};

void write_(uint32_t* direccion , uint32_t*  datos){

    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: Write ”:",pid);

};
void read_(uint32_t*direccion , int size ){
    t_paquete * paquete;

    paquete = crear_paquete(READ_MEM); //Falta hacer al traduccion
    agregar_a_paquete(paquete, &direccion, sizeof(uint32_t));
    agregar_a_paquete(paquete, &size, sizeof(int));
    enviar_paquete(paquete, socket_dispatch);
    //paquete enviado

    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: Read ”:",pid);

};
void  noop(){
    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: NOOP ”:",pid);
};

void goto_(int direccion){
    pc = direccion;
    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: GOTO ”:",pid);
};

//--- SYSCALLS
void io(){


    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: IO ”:",pid);

};
void init_proc(){
    

    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: INIT_PROC ”:",pid);
};
void dump_memory(){
    t_paquete * paquete;

    // paquete = crear_paquete(MEMORY_DUMP);
    // agregar_a_paquete(paquete, &direccion, sizeof(uint32_t));
    // enviar_paquete(paquete, socket_dispatch);
    // //paquete enviado

    //respuesta

    // cod_op = recibir_operacion(socket_conexion_memoria);
    // t_list * paquete_recv;
    // paquete_recv = recibir_paquete(socket_conexion_memoria);
    // int dato = list_remove(paquete_recv, 0);
    // int dato2 = list_remove(paquete_recv, 0);

    // list_destroy(paquete_recv);

    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: DUMP_MEMORY ”:",pid);
};
void exit_(){
    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: EXIT ”:",pid);
};