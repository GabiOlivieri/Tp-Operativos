#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdlib.h>
#include <stdio.h>
#include <shared/socket.h>
#include <shared/structs.h>


/**
* @NAME: enviar_mensaje
* @DESC: envia un mensaje al servidor.
*/
void enviar_mensaje(char* mensaje, int socket_cliente);

/**
* @NAME: eliminar_paquete
* @DESC: elimina el paquete que se mando al servidor.
*/
void eliminar_paquete(t_paquete* paquete);

/**
* @NAME: serializar_paquete
* @DESC: serializa el paquete para que no se pierdan datos.
*/
void * serializar_paquete(t_paquete* paquete, int bytes);

/**
* @NAME: agregar_entero_a_paquete
* @DESC: agrega un entero al buffer en el paquete.
*/
void agregar_entero_a_paquete(t_paquete* paquete, int x);

/**
* @NAME: crear_paquete
* @DESC: Crea un paquete con un buffer vacio
*/
t_paquete* crear_paquete(void);

/**
* @NAME: crear_buffer
* @DESC: Crea un buffer vacio
*/
void crear_buffer(t_paquete* paquete);

void enviar_paquete(t_paquete* paquete, int socket_cliente);
#endif
