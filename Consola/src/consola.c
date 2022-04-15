#include <consola.h>


int main(int argc, char** argv) {
    t_log* logger = log_create("./consola.log","CONSOLA", false , LOG_LEVEL_ERROR);
    t_config* config = config_create("./consola.conf");
    t_nombre* nombre = malloc(sizeof(t_nombre));

    if(argc != 3) {
        log_error(logger, "se espera que ingrese 2 parametros por consola");
        return EXIT_FAILURE;
    }

    if(!esta_bien_definido_el_config(config)) {
        log_error(logger , "esta mal definido el config");
        return EXIT_FAILURE;
    }

    leer_config(config , nombre);

    leer_file(argv[2],logger);


    liberar_memoria(logger , config , nombre);
    return EXIT_SUCCESS;
}


bool esta_bien_definido_el_config(t_config* config){
    return config_has_property(config , "IP_KERNEL") && config_has_property(config , "PUERTO_KERNEL");
}

void leer_config(t_config* config, t_nombre* nombre){
    nombre->ip_kernel = config_get_string_value(config , "IP_KERNEL");
    nombre->puerto_kernel = config_get_int_value(config , "PUERTO_KERNEL");
}

void leer_file(char* path,t_log* logger){
    FILE* pseudocodigo = fopen(path , "r");
    char* token;
    char* param;
    char* argumentos;


    struct stat sb;
    if (stat(path, &sb) == -1) {
        perror("stat");
        log_error(logger , "No encontró el archivo de pseudocodigo");
        exit(EXIT_FAILURE);
    }

    char* file_contents = malloc(sb.st_size + 1);
    fread(file_contents, sb.st_size, 1, pseudocodigo);

    char* rest = file_contents;

    while ((token = strtok_r(rest, "\n", &rest))){

            if (strcmp(token, "NO_OP") == 0){
            	for(int i=0;i<=5;i++){
            		printf("NO_OP\n");
            	}
            }else{
            	param = strtok_r(token, " ", &token);
            	printf("%s ", param);

             while((argumentos = strtok_r(token, " ", &token)))
            	 printf("%d ", atoi(argumentos));
            }
            printf("\n");
    }

    free(file_contents);
    fclose(pseudocodigo);
}

void liberar_memoria(t_log* logger, t_config* config , t_nombre* nombre){
    log_destroy(logger);
    config_destroy(config);
    nombre_free(nombre);
}


void nombre_free(t_nombre* nombre){
    // free(nombre->ip_kernel); // no se porque sin esto me hace todo el free de una igual XD
    free(nombre);
}
