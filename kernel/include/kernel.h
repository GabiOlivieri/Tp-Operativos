#ifndef PROJECT_KERNEL_H_
#define PROJECT_KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <shared/socket.h>
#include <shared/server.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include<unistd.h>
#include <semaphore.h>


typedef struct configuraciones {
    char *ip_memoria;
    u_int16_t puerto_memoria;
    char *ip_cpu;
    u_int16_t puerto_cpu_dispatch;
    u_int16_t puerto_cpu_interrupt;
    char *puerto_escucha;
    char *algoritmo_planificacion;
    u_int16_t estimacion_inicial;
    double alfa;
    u_int16_t grado_multiprogramacion;
    u_int16_t tiempo_max_bloqueado;
} t_configuraciones;

typedef struct hilo_struct {
    int socket;
    t_log* logger;
    t_configuraciones* configuraciones;
    t_queue* cola_new;
} t_hilo_struct;

typedef struct planificador_struct {
    t_log* logger;
    t_configuraciones* configuraciones;
    t_queue* cola_new;
    t_queue* cola_ready;
} t_planificador_struct;

/**
* @NAME: crear_pcb
* @DESC: lee los datos del paquete e interpreta las instrucciones para crear el pcb correspondiente
*/
t_pcb* crear_pcb(char* buffer,t_configuraciones* configuraciones);

/**
* @NAME: manejar_conexion
* @DESC: espera clientes y deriva la tarea de atenderlos en un nuevo hilo
*/
void manejar_conexion(void* hilo_struct);

/**
* @NAME: atender_cliente
* @DESC: recibe la operacion del cliente y la ejecuta
*/
int atender_cliente(void* hilo_struct);

/**
* @NAME: atender_cliente
* @DESC: recibe la informacion del proceso y crea su pcb correspondiente
*/
void iniciar_proceso(int client_socket, t_configuraciones* configuraciones, t_queue* cola_new);

/**
* @NAME: planificador_largo_plazo
* @DESC: planifica los procesos que entran y salen del sistema
*/
void planificador_largo_plazo(void* arg);

/**
* @NAME: leer_config
* @DESC: lee los datos del config.
*/
void leer_config(t_config* config, t_configuraciones* configuraciones);

/**
* @NAME: liberar_memoria
* @DESC: libera la memoria usada en este modulo.
*/
void liberar_memoria(t_log* logger, t_config* config , t_configuraciones* configuraciones , int servidor);

/**
* @NAME: nombre_free
* @DESC: libera la memoria usada de nombre structura para almacenar los valores obtenidos del config.
*/
void configuraciones_free(t_configuraciones* configuraciones);


#endif
