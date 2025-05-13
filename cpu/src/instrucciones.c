#include <instrucciones.h>




void ciclo_instruccion(){
	while ((1)){
        Feth();
        Decode();
        Execute();
        Execute();
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

void Feth(){
        log_info(logger,"## PID: %d TID: %d PC: %d- FETCH ",, , );
};
void Decode(char * instr){
    char ** instrPartida = string_split(instr, " ");
    int opCode = instrStringMap(instrinstrPartida[0]);

    switch (opCode){
        case IO:

        break;
        case INIT_PROC:

        
        break;

        case DUMP_MEMORY:

        
        break;
        case EXIT:

        
        break;
        case NOOP:

        
        break;
        case WRITE:

        
        break;
        case READ:

        
        break;
        case GOTO:

        break;
        default:
             log_info(logger, "Instrucción no reconocida: %s", *instrPartida);
             exit(EXIT_FAILURE);
                break;
        }
        if (opCode == IO || opCode == INIT_PROC || opCode ==  WRITE || opCode ==  READ && (!instrinstrPartida[1] || !instrinstrPartida[2])){
            log_info(logger, "Instrucción no tiene los 2 parametros: %s", *instrPartida);
            exit(EXIT_FAILURE);
        } else if (opCode == GOTO && !instrinstrPartida[1]){
            log_info(logger, "Instrucción no tiene el parametro: %s", *instrPartida);
            exit(EXIT_FAILURE);
        }
        
};
void Execute(){

};
void Execute(){

};
void Check_Int(){

};
