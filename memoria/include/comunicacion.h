#ifndef PROJECT_COMUNICACION_H_
#define PROJECT_COMUNICACION_H_

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/log.h>
#include <commons/config.h>
#include <shared/utils.h>
#include <shared/socket.h>
#include <shared/structs.h>
#include <structs.h>

/**
* @NAME: escuchar_server
* @DESC: falopa.
*/
int server_escuchar(char* server_name ,int server_socket , t_log* logger);

/**
* @NAME: leer_config_memoria server
* @DESC: falopa.
*/
void leer_config_memoria(t_configuraciones_memoria* configuraciones , t_config* config);

/**
* @NAME: liberar_memoria_inicial
* @DESC: falopa.
*/
void liberar_memoria_inicial(t_log* logger , t_config* config , t_configuraciones_memoria* configuraciones);

/**
* @NAME: liberar_memoria
* @DESC: falopa.
*/
void liberar_memoria(t_log* logger , t_config* config , t_configuraciones_memoria* configuraciones , int servidor);

#endif