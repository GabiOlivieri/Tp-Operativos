#include <comunicacion.h>


extern uint32_t memoria_disponible;
extern pthread_mutex_t MUTEX_MP_BUSY;

#define STR(_) #_

typedef struct {
    int fd;
    char* server_name;
} t_procesar_conexion_args;

static void procesar_conexion(void* void_args) {
	t_log* logger = log_create("./memoria.log","KERNEL", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./memoria.conf");
    t_configuraciones_memoria* cfg = malloc(sizeof(t_configuraciones_memoria));

    char* config_properties[] = {
            "PUERTO_ESCUCHA",
            "TAM_MEMORIA",
            "TAM_PAGINA",
            "PAGINAS_POR_TABLA",
            "RETARDO_MEMORIA",
            "ALGORITMO_REEMPLAZO",
            "MARCOS_POR_PROCESO",
            "RETARDO_SWAP",
            "PATH_SWAP",
            NULL
    };

	if(!config_has_all_properties(config , config_properties)) {
        log_error(logger , "esta mal definido el config");
		liberar_memoria_inicial(logger , config , cfg);
        return EXIT_FAILURE;
    }

	leer_config_memoria(cfg , config);

    t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
    int cliente_socket = args->fd;
    char* server_name = args->server_name;
    free(args);

    op_code cop;
    while (cliente_socket != -1) {

        if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
            log_info(logger, STR(DISCONNECT!));
            return;
        }

        switch (cop) {
            case -1:
                log_error(logger, "Cliente desconectado de FALOPA...");
                liberar_memoria_inicial(logger,config,cfg);
                return;
            default:
                log_error(logger, "Algo anduvo mal en el server de FALOPA");
                liberar_memoria_inicial(logger,config,cfg);
                return;
        }
    }

    log_warning(logger, "El cliente se desconecto de %s server", server_name);
    return;
}


int server_escuchar(char* server_name ,int server_socket , t_log* logger) {
    int cliente_socket = esperar_cliente(logger, server_name, server_socket);

    if (cliente_socket != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->fd = cliente_socket;
        args->server_name = server_name;
        // ver el tema de hilos para continuar , aguante la falopa
        // pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        // pthread_detach(hilo);
        return 1;
    }
    return 0;
}


void leer_config_memoria(t_configuraciones_memoria* configuraciones , t_config* config){
	configuraciones->puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	configuraciones->tam_memoria = config_get_string_value(config,"TAM_MEMORIA");
	configuraciones->tam_pagina = config_get_string_value(config,"TAM_PAGINA");
	configuraciones->paginas_por_tabla = config_get_string_value(config,"PAGINA_POR_TABLA");
	configuraciones->retardo_memoria = config_get_string_value(config,"RETARDO_MEMORIA");
	configuraciones->algoritmo_reemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
	configuraciones->marcos_por_proceso = config_get_string_value(config,"MARCOS_POR_PROCESO");
	configuraciones->retardo_swap = config_get_string_value(config,"RETARDO_SWAP");
	configuraciones->path_swap = config_get_string_value(config,"PATH_SWAP");
}

void liberar_memoria_inicial(t_log* logger , t_config* config , t_configuraciones_memoria* configuraciones){
	log_destroy(logger);
	config_destroy(config);
	free(configuraciones);
}

void liberar_memoria(t_log* logger , t_config* config , t_configuraciones_memoria* configuraciones , int servidor){
	liberar_memoria_inicial(logger , config , configuraciones);
	close(servidor);
}