#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

// SERVIDOR
/**
* @NAME: iniciar_servidor
* @DESC: Inicia el servidor.
*/
int iniciar_servidor(t_log* logger, const char* name, char* ip, char* puerto);

/**
* @NAME: esperar_cliente
* @DESC: espera a que un cliente se conecte al server.
*/
int esperar_cliente(t_log* logger, const char* name, int socket_servidor);

// CLIENTE
/**
* @NAME: crear_conexion
* @DESC: crea la conexion entre el cliente y el servidor.
*/
int crear_conexion(t_log* logger, const char* server_name, char* ip, char* puerto);

/**
* @NAME: liberar_conexion
* @DESC: libera la conexion creda anteriormente.
*/
void liberar_conexion(int* socket_cliente);

#endif