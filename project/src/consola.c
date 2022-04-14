#include <consola.h>


int consola(int cantidadArgumentos , char** argumentos){
    t_log* logger = log_create("../log/consola.log","CONSOLA", false , LOG_LEVEL_ERROR);
    t_config* config = config_create("../config/consola.conf");
    char* ip_kernel;
    u_int16_t puerto_kernel;

    if(cantidadArgumentos != 3) {
        log_error(logger, "se espera que ingrese 2 parametros por consola");
        return EXIT_FAILURE;
    }

    if(!esta_bien_definido_el_config(logger , config)) return EXIT_FAILURE;

    ip_kernel = config_get_string_value(config , "IP_KERNEL");
    puerto_kernel = config_get_int_value(config , "PUERTO_KERNEL");

    liberar_memoria(logger , config , ip_kernel);
    return EXIT_SUCCESS;
}


bool esta_bien_definido_el_config(t_log* logger, t_config* config){
    if(!(config_has_property(config , "IP_KERNEL"))) {
        log_error(logger , "No esta bien definida la variable IP_KERNEL en el archivo consola.conf");
        return false;
    }
    if(!(config_has_property(config , "PUERTO_KERNEL"))) {
        log_error(logger , "No esta bien definida la variable PUERTO_KERNEL en el archivo consola.conf");
        return false;
    }
    return true;
}


void liberar_memoria(t_log* logger, t_config* config , char* ip_kernel){
    log_destroy(logger);
    config_destroy(config);
    free(ip_kernel);
}