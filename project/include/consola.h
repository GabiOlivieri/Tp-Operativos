#ifndef PROJECT_CONSOLA_H_
#define PROJECT_CONSOLA_H_

#include <stdlib.h>
#include <stdio.h>
#include<commons/log.h>
#include<commons/config.h>

/**
* @NAME: consola
* @DESC: se encarga del modulo de CONSOLA.
*/
int consola(int cantidadArgumentos , char** argumentos);

/**
* @NAME: esta_bien_definido_el_config
* @DESC: chequea que el config este bien definido.
*/
bool esta_bien_definido_el_config(t_log* logger, t_config* config);

/**
* @NAME: liberar_memoria
* @DESC: libera la memoria usada en este modulo.
*/
void liberar_memoria(t_log* logger, t_config* config);

#endif