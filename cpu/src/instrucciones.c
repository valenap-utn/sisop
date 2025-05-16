#include <instrucciones.h>




void ciclo_instruccion(){
    char * instrSTR;
    instruccion_t instr;
        while ((1)){
            instrSTR = Fetch();
            instr = Decode(instrSTR);
            Execute(instr);
            Check_Int();
        }
}

int instrStringMap(char opcodeStr []){
    int opCode;

    if (strcmp(opcodeStr,"IO") ) {
        opCode = 0;
    }
    else if (strcmp(opcodeStr,"INIT_PROC")) {
        opCode = 1;
    }
    else if (strcmp(opcodeStr,"DUMP_MEMORY")) {
        opCode = 2;
    }
    else if (strcmp(opcodeStr,"EXIT")) {
        opCode = 3;
    }
    else if (strcmp(opcodeStr,"NOOP")) {
        opCode = 4;
    }
    else if (strcmp(opcodeStr,"WRITE")) {
        opCode = 5;
    }
    else if (strcmp(opcodeStr,"READ")) {
        opCode = 6;
    }
    else if (strcmp(opcodeStr,"GOTO")) {
        opCode = 7;
    }
    return opCode;
};

char * Fetch(){
    log_info(logger,"## PID: %d TID: %d PC: %d- FETCH ",, , );
    return "NOOP"; // Para cambiar en el futuro por el valor posta
};

instruccion_t Decode(char * instr){
    instruccion_t current_instr;
    current_instr.data = string_split(instr, " ");
    current_instr.opcode = instrStringMap(current_instr.data[0]);


    switch (current_instr.opcode){
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
             log_info(logger, "Instrucción no reconocida: %s", *instrPartida);
             exit(EXIT_FAILURE);
                break;
        }
        if (current_instr.opCode == IO || current_instr.opCode == INIT_PROC || current_instr.opCode ==  WRITE || 
            current_instr.opCode ==  READ && (!current_instr.data[0] || !current_instr.data[1])){

                log_info(logger, "Instrucción no tiene los 2 parametros: %s", *instrPartida);
                exit(EXIT_FAILURE);
        } else if (current_instr.opCode == GOTO && !current_instr.data[0]){

                log_info(logger, "Instrucción no tiene el parametro: %s", *instrPartida);
                exit(EXIT_FAILURE);
        };
        return current_instr;    
};

void Execute(instruccion_t instr){
    switch (instr.tipo){
        case SYSCALL:

        break;
        case USUARIO:
                    
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