#ifndef PROJECT_KERNEL_H_
#define PROJECT_KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <shared/socket.h>
#include <shared/server.h>
#include <commons/collections/list.h>
#include <commons/config.h>

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
