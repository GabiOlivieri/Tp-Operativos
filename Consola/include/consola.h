#ifndef PROJECT_CONSOLA_H_
#define PROJECT_CONSOLA_H_

#include <stdlib.h>
#include <stdio.h>
#include<commons/log.h>
#include<commons/config.h>
#include <sys/stat.h>

typedef struct nombre {
    char *ip_kernel;
    u_int16_t puerto_kernel;
} t_nombre;

/**
* @NAME: esta_bien_definido_el_config
* @DESC: chequea que el config este bien definido.
*/
bool esta_bien_definido_el_config(t_config* config);

/**
* @NAME: leer_config
* @DESC: lee los datos del config y si falta alguno retorna false.
*/
void leer_config(t_config* config, t_nombre* nombre);

/**
* @NAME: leer_file
* @DESC: lee los datos del config y si falta alguno retorna false.
*/
char* leer_file(char* path);

/**
* @NAME: liberar_memoria
* @DESC: libera la memoria usada en este modulo.
*/
void liberar_memoria(t_log* logger, t_config* config , t_nombre* nombre, char* pseudocodigo);

/**
* @NAME: nombre_free
* @DESC: libera la memoria usada de nombre structura para almacenar los valores obtenidos del config.
*/
void nombre_free(t_nombre* nombre);

#endif