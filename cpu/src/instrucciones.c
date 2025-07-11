#include <instrucciones.h>
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
                write_((uint32_t)atoi(instr.data[1]) , atoi(instr.data[0])); // Falta poner los valores posta
            break;
            case READ_I:

                read_((uint32_t)atoi(instr.data[1]) , atoi(instr.data[0]));
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

void write_(uint32_t direccion , int datos){

    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: Write DIR = %ls TAM = %d”: ",pid,direccion,datos);

};
void read_(uint32_t direccion , int size ){
    t_paquete * paquete;

    paquete = crear_paquete(READ_MEM); //Falta hacer al traduccion
    agregar_a_paquete(paquete, &direccion, sizeof(uint32_t));
    agregar_a_paquete(paquete, &size, sizeof(int));
    enviar_paquete(paquete, socket_dispatch);
    //paquete enviado

    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: Read DIR = %d TAM = %d”:",pid,(int *)direccion,size);

};
void  noop(){
    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: NOOP ”:",pid);
};

void goto_(int direccion){
    // buscar_pid();
    pc = direccion;
    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: GOTO ”:",pid);
};

// int buscar_pid(t_list lista, int pid){
//     PCB * elemento;
//     t_list_iterator * iterator = list_iterator_create(lista);
    

//     while(list_iterator_has_next(iterator)){
//         elemento = list_iterator_next(iterator);
//         if(elemento->pid == pid){

//             elemento->pid = ;
//             return list_iterator_index(iterator);
//         }
//     }
//     list_iterator_destroy(iterator);
//     return -1;
// };

//--- SYSCALLS
void io(char * Dispositivo, int tiempo){ // (Dispositivo, Tiempo) 


    log_info(logger, "Instrucción Ejecutada: “## PID: %d - Ejecutando: IO ”:",pid);

};
void init_proc(char * Dispositivo, int tamanno){ //(Archivo de instrucciones, Tamaño)
    

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


void MMU(uint32_t direccion){
    
};


uint32_t TLB(uint32_t  Direccion){
    int * tabla = (int *) malloc(sizeof(uint32_t) * estradas_tlb);
    return 0;
};
