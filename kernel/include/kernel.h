#ifndef PROJECT_KERNEL_H_
#define PROJECT_KERNEL_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <commons/log.h>
#include <shared/socket.h>
#include <shared/server.h>
#include <shared/client.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <shared/utils.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>


typedef struct configuraciones {
    char *ip_propia;
    char *ip_memoria;
    char * puerto_memoria;
    char *ip_cpu;
    char * puerto_cpu_dispatch;
    char * puerto_cpu_interrupt;
    char *puerto_escucha;
    char *algoritmo_planificacion;
    u_int16_t estimacion_inicial;
    double alfa;
    u_int16_t grado_multiprogramacion;
    u_int16_t tiempo_max_bloqueado;
} t_configuraciones;

typedef struct colas_struct {
    t_queue* cola_new;
    t_queue* cola_ready;
    t_queue* cola_exec;
    t_queue* cola_blocked;
    t_queue* cola_ready_suspended;
    t_queue* cola_blocked_suspended;
    t_queue* cola_io;
} t_colas_struct;

typedef struct info_bloqueado{
    t_pcb* pcb;
    char *provieneDe;
} t_info_bloqueado;

typedef struct hilo_struct {
    int socket;
    t_log* logger;
    t_configuraciones* configuraciones;
    t_colas_struct* colas;
} t_hilo_struct;

typedef struct binary_semaphore {
    pthread_mutex_t mutex;
    pthread_cond_t cvar;
    bool v;
} t_binary_semaphore;

void mysem_post(struct binary_semaphore *p);
void mysem_wait(struct binary_semaphore *p);

typedef struct hilo_struct_standard_con_pcb {
    t_pcb* pcb;
    t_log* logger;
    t_configuraciones* configuraciones;
    t_colas_struct* colas;
} t_hilo_struct_standard_con_pcb;

typedef struct planificador_struct {
    t_log* logger;
    t_configuraciones* configuraciones;
    t_colas_struct* colas;
} t_planificador_struct;

/**
* @NAME: crear_pcb
* @DESC: lee los datos del paquete e interpreta las instrucciones para crear el pcb correspondiente
*/
t_pcb* crear_pcb(char* buffer,t_configuraciones* configuraciones,t_log* logger);

/**
* @NAME: enviar_pcb
* @DESC: Envia el pcb recibido a cpu
*/
void enviar_pcb(void* arg);

/**
* @NAME: manejar_conexion
* @DESC: espera clientes y deriva la tarea de atenderlos en un nuevo hilo
*/
void manejar_conexion(t_log* logger, t_configuraciones* configuraciones, int socket, t_colas_struct* colas);

/**
* @NAME: atender_cliente
* @DESC: recibe la operacion del cliente y la ejecuta
*/
int atender_cliente(void* hilo_struct);


int atender_cpu(void* arg);

/**
* @NAME: atender_cliente
* @DESC: recibe la informacion del proceso y crea su pcb correspondiente
*/
void iniciar_proceso(t_log* logger,int client_socket, t_configuraciones* configuraciones, t_queue* cola_new);

/**
* @NAME: planificador_largo_plazo
* @DESC: planifica los procesos que entran y salen del sistema
*/
void planificador_largo_plazo(void* arg);

/**
* @NAME: planificador_corto_plazo
* @DESC: planifica los procesos que deben ejecutarse
*/
void planificador_corto_plazo(void* arg);

/**
* @NAME: crear_colas()
* @DESC: crea las colas para los procesos y las almacena en un struct
*/
t_colas_struct* crear_colas();

/**
* @NAME: crear_planificadores()
* @DESC: instancia hilos para los planificadores pasando la informacion que necesita para planificar
*/
void crear_planificadores(t_log* logger, t_configuraciones* configuraciones,t_colas_struct* colas);

/**
* @NAME: iniciar_estructuras()
* @DESC: solicita a memoria iniciar las estructuras y asigna la tabla de paginas al pcb
*/
void iniciar_estructuras(t_log* logger, t_configuraciones* configuraciones, t_pcb* pcb);



/**
* @NAME: leer_config
* @DESC: lee los datos del config.
*/
void leer_config(t_config* config, t_configuraciones* configuraciones);

/**
* @NAME: liberar_memoria
* @DESC: libera la memoria usada en este modulo.
*/
void liberar_memoria(t_log* logger, t_config* config , t_configuraciones* configuraciones , int servidor);

/**
* @NAME: nombre_free
* @DESC: libera la memoria usada de nombre structura para almacenar los valores obtenidos del config.
*/
void configuraciones_free(t_configuraciones* configuraciones);

/**
* @NAME: mandar_y_recibir_confirmacion
* @DESC: Es la función de un hilo que extrae el pcb de el struct del hilo y lo manda a memoria esperando a que memoria lo devuelva.
*/
int mandar_y_recibir_confirmacion(void* arg);

void hilo_enviar_pcb_cpu(void* arg);

#endif
