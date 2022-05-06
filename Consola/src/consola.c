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

    leer_config(config , nombre);

    t_list* lista = leer_file(argv[2],logger);
    enviar_instrucciones(lista,logger,nombre);
    list_destroy(lista);

    liberar_memoria(logger , nombre, config);
    return EXIT_SUCCESS;
}

void leer_config(t_config* config, t_nombre* nombre){
    nombre->ip_kernel = config_get_string_value(config , "IP_KERNEL");
    nombre->puerto_kernel = config_get_int_value(config , "PUERTO_KERNEL");
}

t_list* leer_file(char* path,t_log* logger){
    char* file_contents = leer_archivo_completo(path);
    char* token;
    char* param;
    char* argumentos;

    if(!file_contents){
        log_error(logger , "No encontró el archivo de pseudocodigo");
        exit(EXIT_FAILURE);
    }

    char* rest = file_contents;
    t_list* lista = list_create();
    
    while ((token = strtok_r(rest, "\n", &rest))){
        param = strtok_r(token," ",&token);
            if (strcmp(param, "NO_OP") == 0) {
            	argumentos = strtok_r(token, " ", &token);	
                for(int i = 0;i < atoi(argumentos); i++){
            	    printf("%s \n", "NO_OP");
                    list_add(lista,0);
            	}
            }else{
                //TO DO -> SWITCHEAR POR IDENTIFICADOR Y AGREGAR EL CORRECTO
                list_add(lista,0);
                printf("%s ", param);

             while((argumentos = strtok_r(token, " ", &token))){
                //TO DO -> CASTEAR PARAMETROS A ENTEROS Y AGREGARLOS
                //list_add(lista,arg_enteros);
            	 printf("%d ", atoi(argumentos));
             }
            printf("\n");
            }
    }
    free(file_contents);
    return lista;
}
void enviar_instrucciones(t_list* lista, t_log* logger,t_nombre* nombre){
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = INICIAR_PROCESO;
    t_list_iterator* iterator = list_iterator_create(lista);
    while(list_iterator_has_next(iterator)){
        int ins = list_iterator_next(iterator);
        printf("El entero es: %d\n",ins);
        agregar_entero_a_paquete(paquete,ins);
    }
    list_iterator_destroy(iterator);
    int conexion = crear_conexion(logger , "SERVER PLATA Y MIEDO NUNCA TUVE" , nombre->ip_kernel ,"8000");
    enviar_paquete(paquete,conexion);
    eliminar_paquete(paquete);
    close(conexion);   
}


void liberar_memoria(t_log* logger, t_nombre* nombre, t_config* config){
    log_destroy(logger);
    config_destroy(config);
    nombre_free(nombre);
}

void nombre_free(t_nombre* nombre){
    // free(nombre->ip_kernel); // no se porque sin esto me hace todo el free de una igual XD
    free(nombre);
}
