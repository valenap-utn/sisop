#ifndef KERNEL_MAIN_
#define KERNEL_MAIN_

#include <utils/utils.h>

typedef struct{
    int dispatch;
    int interrupt;
}t_socket_cpu;

typedef struct{
    int socket;
    char *nombre;
    int libre;
}t_socket_io;


#include <kernel_utilities.h>

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