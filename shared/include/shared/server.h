#ifndef SERVER_H_
#define SERVER_H_

#include <stdlib.h>
#include <stdio.h>
#include <shared/socket.h>
#include <shared/structs.h>
#include<commons/collections/list.h>

/**
* @NAME: recibir_operacion
* @DESC: recibe la operacion que debe hacer desde el cliente.
*/
int recibir_operacion(int socket_cliente);

/**
* @NAME: recibir_mensaje
* @DESC: recibe un mensaje enviado desde el cliente.
*/
void recibir_mensaje(int socket_cliente , t_log* logger);

/**
* @NAME: iterator
* @DESC: falopa.
*/
// void iterator(char* value);

/**
* @NAME: recibir_buffer
* @DESC: recibe el buffer desde el cliente.
*/
void* recibir_buffer(int* size, int socket_cliente);

/**
* @NAME: recibir_paquete
* @DESC: recibe un paquete de datos enviado desde el cliente.
*/
t_list* recibir_paquete(int socket_cliente);

/**
* @NAME: leer_entero
* @DESC: lee un entero en la posicion indicada existente en el buffer.
*/
int leer_entero(void * buffer, int desplazamiento);
#endif
