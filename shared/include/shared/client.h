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

#endif
