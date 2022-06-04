#ifndef STRUCTS_H_
#define STRUCTS_H_

#include <commons/collections/list.h>

typedef enum
{
	MENSAJE,
	PAQUETE,
	INICIAR_PROCESO,
	INICIAR_ESTRUCTURAS,
    DEVOLVER_PROCESO
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

typedef enum
{
	NEW,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} estado;

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
    float estimacion_inicial;
    int rafaga_anterior;
    float alfa;
    estado estado;
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
