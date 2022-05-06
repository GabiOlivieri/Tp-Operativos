#ifndef PROJECT_CONSOLA_H_
#define PROJECT_CONSOLA_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <shared/utils.h>
#include <shared/socket.h>
#include <shared/client.h>

typedef struct nombre {
    char *ip_kernel;
    u_int16_t puerto_kernel;
} t_nombre;


/**
* @NAME: leer_config
* @DESC: lee los datos del config.
*/
void leer_config(t_config* config, t_nombre* nombre);

/**
* @NAME: leer_file
* @DESC: lee los datos del config y si falta alguno retorna false.
*/
t_list* leer_file(char* path,t_log* logger);

/**
* @NAME: enviar_instrucciones
* @DESC: lee los datos de la lista y las envia
*/
void enviar_instrucciones(t_list* lista, t_log* logger,t_nombre* nombre);

/**
* @NAME: liberar_memoria
* @DESC: libera la memoria usada en este modulo.
*/
void liberar_memoria(t_log* logger, t_nombre* nombre);

/**
* @NAME: nombre_free
* @DESC: libera la memoria usada de nombre structura para almacenar los valores obtenidos del config.
*/
void nombre_free(t_nombre* nombre);

#endif
