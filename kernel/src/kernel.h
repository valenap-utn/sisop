#ifndef KERNEL_MAIN_
#define KERNEL_MAIN_

#include <kernel_utilities.h>
#include <utils/utils.h>
#include <commons/log.h>
#include <commons/config.h>

typedef struct{
    int socket;
    char *nombre;
}t_socket_io;

typedef struct{
    int dispatch;
    int interrupt;
    int flag_libre;
}t_socket_cpu;

typedef struct{
    int PID;
    int PC;
    t_list ME;
    t_list MT;    
}PCB;

#endif