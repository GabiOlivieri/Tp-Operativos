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
#include <math.h>
#include <time.h>

typedef struct configuraciones {
    char *ip_memoria;
    char *puerto_memoria;
    u_int16_t entradas_TLB;
    char *reemplazo_TLB;
    char *algoritmo_planificacion;
    u_int16_t retardo_NOOP;
    char *puerto_escucha_dispatch;
    char *puerto_escucha_interrupt;
    char *ip_local;
} t_configuraciones;

typedef struct hilo_struct {
    int socket;
    t_log* logger;
    t_configuraciones* configuraciones;
    t_list* tlb;
} t_hilo_struct;

typedef struct direccion {
    int entrada_primer_nivel;
    int entrada_segundo_nivel;
    int desplazamiento;
} t_direccion;

typedef struct pagina{
    int entrada_primer_nivel;
    int entrada_segundo_nivel;
} t_pagina;

typedef struct tlb{
    time_t  instante_de_carga;
    time_t  instante_de_ultima_referencia;
    t_pagina* pagina;
    int marco;
} t_fila_tlb;


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

/**
* @NAME: obtener_lista_instrucciones
* @DESC: pasandole el buffer y el pcb, te devuelve una lista con todos los enteros de las intrucciones y sus parametros
*/
t_list* obtener_lista_instrucciones(char* buffer, t_pcb* pcb);

/**
* @NAME: recibir_pcb
* @DESC: recibe el pcb de un buffer
*/
t_pcb* recibir_pcb(char* buffer);

t_direccion mmu_traduccion(int dir_logica);

/**
* @NAME: ciclo_de_instruccion
* @DESC: ejecuta la instrucción que está siendo apuntada por el PC
*/
int ciclo_de_instruccion(t_log* logger,t_pcb* pcb,t_configuraciones* configuraciones,t_list* tlb);

/**
* @NAME: hay_interrupcion
* @DESC: chequea si hay una interrupción enviada por el kernel
*/
int hay_interrupcion();

void devolver_pcb(t_pcb* pcb,t_log* logger,int socket);

int atender_cliente(void* arg);

/**
* @NAME: atender_interrupcion
* @DESC: hilo encargado de iniciar el servidor para atender las interrupciones
*/
int atender_interrupcion(void* arg);

void manejar_conexion_kernel(t_log* logger, t_configuraciones* configuraciones, int socket,t_list* tlb);

t_list* crear_TLB(int cant_entradas);

bool puede_cachear(t_list* tlb);

#endif