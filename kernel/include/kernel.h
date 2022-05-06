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
    char *puerto_cpu_dispatch;
    char *puerto_cpu_interrupt;
    char *puerto_escucha;
    char *algoritmo_planificacion;
    u_int16_t estimacion_inicial;
    double alfa;
    u_int16_t grado_multiprogramacion;
    u_int16_t tiempo_max_bloqueado;
} t_configuraciones;

/**
* @NAME: decodificar_instrucciones
* @DESC: lee los datos del paquete e interpreta las instrucciones para almacenarlas.
*/
t_list* decodificar_instrucciones(char* buffer);

/**
* @NAME: crear_pcb
* @DESC: crea los pcb usando la lista de instrucciones y las configuraciones
*/
t_pcb* crear_pcb(t_list* lista,t_configuraciones* configuraciones);
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
