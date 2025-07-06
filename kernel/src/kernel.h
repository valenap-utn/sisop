#ifndef KERNEL_MAIN_
#define KERNEL_MAIN_

typedef struct{
    int socket;
    char *nombre;
}t_socket_io;

typedef struct{
    int dispatch;
    int interrupt;
}t_socket_cpu;

#include <kernel_utilities.h>
#include <utils/utils.h>
#include <commons/log.h>
#include <commons/config.h>
#include <syscalls.h>


/*
t_list* new;
t_list* ready;
t_list* blocked;
t_list* susp_block;
*/

//FUNCIONES



#endif