#ifndef STRUCTS_H_
#define STRUCTS_H_

#include <commons/collections/list.h>

typedef enum
{
	MENSAJE,
	PAQUETE,
	INICIAR_PROCESO
} op_code;

typedef enum
{
	NO_OP,
	IO,
	READ,
	WRITE,
	COPY,
	EXIT
} op_ins;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct
{
    int pid;
    int size;
    int pc;
    t_list* lista_instrucciones;
    int tabla_paginas; //Inchequeable
    int estimacion_inicial;
    float alfa;
}t_pcb;

#endif
