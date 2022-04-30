#include <comunicacion.h>

int main(int argc, char* argv[]) {
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


	int server_socket = iniciar_servidor(logger , "un nombre" , "127.0.0.1" , cfg->puerto_escucha);
    server_escuchar("asdf", server_socket , logger);


    liberar_memoria(logger,config,cfg,server_socket);
    return 0;
}
