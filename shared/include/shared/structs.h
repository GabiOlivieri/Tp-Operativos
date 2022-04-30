#ifndef STRUCTS_H_
#define STRUCTS_H_

typedef enum
{
	MENSAJE,
	PAQUETE,
	INICIAR_PROCESO,
	DESCONEXION=-1
}op_code;

typedef enum
{
	NO_OP,
	IO,
	READ,
	WRITE,
	COPY,
	EXIT
}op_ins;

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


#endif