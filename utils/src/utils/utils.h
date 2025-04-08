#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <readline/readline.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>

//Conexiones
//Cliente -> Server
//Kernel -> CPU
//Kernel -> Memoria
//CPU -> Memoria
//Memoria -> Filesystem


//colas planificador
extern t_list *sch_cola_ready,*sch_cola_new,*sch_cola_new_plus,*sch_cola_block,*sch_cola_exec;

//semaforos
extern sem_t sem_p_ready;
extern pthread_mutex_t m_cola_new, m_cola_ready, m_cola_new_plus, m_cola_exec;

typedef struct arg_struct {
    char * puerto;
    char * ip;
}argumentos_thread;


typedef enum
{
  A
}protocolo_socket;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	protocolo_socket codigo_operacion;
	t_buffer* buffer;
} t_paquete;

extern t_log* logger;

//socket
    int iniciar_servidor(char *puerto);
    int esperar_cliente(int socket_servidor);
    int recibir_operacion(int socket_cliente);
    void* recibir_buffer(int* size, int socket_cliente);
    t_list* recibir_paquete(int socket_cliente);
    void* serializar_paquete(t_paquete* paquete, int bytes);
    int crear_conexion(char* ip, char* puerto);
    void crear_buffer(t_paquete* paquete);
    t_paquete* crear_paquete(protocolo_socket cod_op); 
    void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
    void enviar_paquete(t_paquete* paquete, int socket_cliente);
    void eliminar_paquete(t_paquete* paquete);
    void liberar_conexion(int socket_cliente);
    
//socket
    void leer_consola(void);
    void terminar_programa(int conexion, t_log* logger, t_config* config);
    void iterator(char* value);

#endif