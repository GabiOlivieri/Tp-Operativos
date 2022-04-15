#include <consola.h>


int main(int argc, char** argv) {
    t_log* logger = log_create("./log/consola.log","CONSOLA", false , LOG_LEVEL_ERROR);
    t_config* config = config_create("./config/consola.conf");
    t_nombre* nombre = malloc(sizeof(t_nombre));

    if(argc != 3) {
        log_error(logger, "se espera que ingrese 2 parametros por consola");
        return EXIT_FAILURE;
    }

    if(!esta_bien_definido_el_config(config)) {
        log_error(logger , "esta mal definido el config");
        return EXIT_FAILURE;
    }

    // if(!pseudocodigo){
    //     log_error(logger , "no se pudo abrir el archivo de pseudocodigo");
    //     return EXIT_FAILURE;
    // }


    leer_config(config , nombre);

    char* pseudocodigo = leer_file(argv[2]);
    printf("%s",pseudocodigo);

    liberar_memoria(logger , config , nombre , pseudocodigo);
    return EXIT_SUCCESS;
}


bool esta_bien_definido_el_config(t_config* config){
    return config_has_property(config , "IP_KERNEL") && config_has_property(config , "PUERTO_KERNEL");
}

void leer_config(t_config* config, t_nombre* nombre){
    nombre->ip_kernel = config_get_string_value(config , "IP_KERNEL");
    nombre->puerto_kernel = config_get_int_value(config , "PUERTO_KERNEL");
}

char* leer_file(char* path){
    FILE* pseudocodigo = fopen(path , "r");

    struct stat sb;
    if (stat(path, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    char* file_contents = malloc(sb.st_size + 1);
    fread(file_contents, sb.st_size, 1, pseudocodigo);

    // printf("%s\n", file_contents);

    fclose(pseudocodigo);
    return file_contents;
}

void liberar_memoria(t_log* logger, t_config* config , t_nombre* nombre , char* pseudocodigo){
    log_destroy(logger);
    config_destroy(config);
    nombre_free(nombre);
    free(pseudocodigo);
}

void nombre_free(t_nombre* nombre){
    // free(nombre->ip_kernel); // no se porque sin esto me hace todo el free de una igual XD
    free(nombre);
}