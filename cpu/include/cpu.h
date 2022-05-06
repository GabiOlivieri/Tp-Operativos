#ifndef PROJECT_CONSOLA_H_
#define PROJECT_CONSOLA_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <shared/utils.h>
#include <shared/socket.h>
#include <shared/client.h>
#include <commons/collections/list.h>

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
* @NAME: decodificar_instrucciones
* @DESC: lee los datos del paquete e interpreta las instrucciones para almacenarlas.
*/
t_list* decodificar_instrucciones(char* buffer);

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

#endif
