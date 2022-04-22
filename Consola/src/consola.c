#include <consola.h>


int main(int argc, char** argv) {
    t_log* logger = log_create("./consola.log","CONSOLA", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./consola.conf");
    t_nombre* nombre = malloc(sizeof(t_nombre));

    char* config_properties[] = {
        "IP_KERNEL",
        "PUERTO_KERNEL",
        NULL
    };

    if(argc != 3) {
        log_error(logger, "se espera que ingrese 2 parametros por consola");
        return EXIT_FAILURE;
    }

    if(!config_has_all_properties(config , config_properties)) {
        log_error(logger , "esta mal definido el config");
        return EXIT_FAILURE;
    }

    leer_file(argv[2],logger);

    leer_config(config , nombre);
    
    int conexion = crear_conexion(logger , "Server Kernel" , nombre->ip_kernel ,"8000");

    enviar_mensaje("Mensaje de prueba 123" , conexion);

    liberar_memoria(logger , config , nombre , conexion);
    return EXIT_SUCCESS;
}

void leer_config(t_config* config, t_nombre* nombre){
    nombre->ip_kernel = config_get_string_value(config , "IP_KERNEL");
    nombre->puerto_kernel = config_get_int_value(config , "PUERTO_KERNEL");
}

void leer_file(char* path,t_log* logger){
    char* file_contents = leer_archivo_completo(path);
    char* token;
    char* param;
    char* argumentos;

    if(!file_contents){
        log_error(logger , "No encontró el archivo de pseudocodigo");
        exit(EXIT_FAILURE);
    }

    char* rest = file_contents;

    while ((token = strtok_r(rest, "\n", &rest))){

            	param = strtok_r(token, " ", &token);
            	if (strcmp(param, "NO_OP") == 0) {
            		argumentos = strtok_r(token, " ", &token);

            		            	for(int i = 0;i <= atoi(argumentos); i++){
            		            		printf("%s \n", "NO_OP");
            		            	}

            	}else{
            		printf("%s ", param);

             while((argumentos = strtok_r(token, " ", &token)))
            	 printf("%d ", atoi(argumentos));
             printf("\n");
            	}

            }


    free(file_contents);
}

void liberar_memoria(t_log* logger, t_config* config , t_nombre* nombre , int conexion){
    log_destroy(logger);
    config_destroy(config);
    nombre_free(nombre);
    close(conexion);
}

void nombre_free(t_nombre* nombre){
    // free(nombre->ip_kernel); // no se porque sin esto me hace todo el free de una igual XD
    free(nombre);
}
