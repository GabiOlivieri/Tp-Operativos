#ifndef STRUCTS_H_
#define STRUCTS_H_

<<<<<<< HEAD
#include <commons/collections/list.h>
=======
#include<commons/collections/list.h>

>>>>>>> ce3b8dc84345cb1210380625b1c1951615379207

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
    int estimacion_rafaga;
    float alfa;
}t_pcb;

typedef struct
{
    int identificador;
}ins_sin_parametro;

typedef struct
{
    int identificador;
    int parametro;
}ins_con_parametro;

typedef struct
{
    int identificador;
    int parametro1;
    int parametro2;
}ins_con_dos_parametros;

#endif
