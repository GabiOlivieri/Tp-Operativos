#ifndef PROJECT_CPU_H_
#define PROJECT_CPU_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <shared/utils.h>
#include <shared/socket.h>
#include <shared/client.h>
#include <shared/server.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

typedef struct configuraciones {
    char *ip_memoria;
    u_int16_t puerto_memoria;
    u_int16_t entradas_TLB;
    char *reemplazo_TLB;
    char *algoritmo_planificacion;
    u_int16_t retardo_NOOP;
    char *puerto_escucha_dispatch;
    char *puerto_escucha_interrupt;
} t_configuraciones;

typedef struct hilo_struct {
    int socket;
    t_log* logger;
    t_configuraciones* configuraciones;
} t_hilo_struct;


/**
* @NAME: leer_config
* @DESC: lee los datos del config.
*/
void leer_config(t_config* config, t_configuraciones* configuraciones);

/**
* @NAME: leer_file
* @DESC: lee los datos del config y si falta alguno retorna false.
*/
void leer_file(char* path,t_log* logger);


/**
* @NAME: liberar_memoria
* @DESC: libera la memoria usada en este modulo.
*/
void liberar_memoria(t_log* logger, t_config* config , t_configuraciones* configuraciones , int conexion);

/**
* @NAME: nombre_free
* @DESC: libera la memoria usada de nombre structura para almacenar los valores obtenidos del config.
*/
void nombre_free(t_configuraciones* configuraciones);

t_list* obtener_lista_instrucciones(char* buffer, t_pcb* pcb);

t_pcb* recibir_pcb(char* buffer);

int ejecutar_instruccion(t_pcb* pcb,t_configuraciones* configuraciones);

int hay_interrupcion();

void devolver_pcb(t_pcb* pcb,t_log* logger,int socket);

int atender_cliente(void* arg);

int atender_interrupcion(void* arg);

void manejar_conexion(t_log* logger, t_configuraciones* configuraciones, int socket);

#endif