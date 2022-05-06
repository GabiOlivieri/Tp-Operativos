#include<cpu.h>

int main(int argc, char* argv[]) {
    t_log* logger = log_create("./cpu.log","CPU", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./cpu.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));


    char* config_properties[] = {
            "IP_KERNEL",
            "PUERTO_KERNEL",
        };

    leer_config(config,configuraciones);

    int servidor = iniciar_servidor(logger , "CPU Dispatch" , "127.0.0.1" , configuraciones->puerto_escucha_dispatch);
    int client_socket = esperar_cliente(logger , "CPU Dispatch" , servidor);


    t_list* lista;
        while (client_socket != -1 ) {
    		int cod_op = recibir_operacion(client_socket);
    		switch (cod_op) {
    		case MENSAJE:
    			recibir_mensaje(client_socket , logger);
    			break;
    		case PAQUETE:
    			log_info(logger, "Me llegaron los siguientes valores:\n");
    			// list_iterate(lista, (void*) iterator);
    			break;

    		case INICIAR_PROCESO:
    			log_info(logger, "Me llego un INICIAR_PROCESO\n");
    			int size;
       			char * buffer = recibir_buffer(&size, client_socket);
    			ejecutar_instruccion(buffer);
    			break;

    		case -1:
    			log_error(logger, "el cliente se desconecto. Terminando servidor");
    			return EXIT_FAILURE;
    		default:
    			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
    			break;
    		}
    	}
        liberar_memoria(logger,config,configuraciones,servidor);

    return 0;
}

void ejecutar_instruccion(char* buffer){
		printf("Ejecuta instrucción\n");
		t_list* lista = list_create();
		int pcbid = leer_entero(buffer,0);
		printf("El id de pcb recibido es: %d\n",pcbid);
		int cantidad_enteros = leer_entero(buffer,1);
		printf("La cantidad de enteros es: %d\n",cantidad_enteros);
		for(int i = 2; i <= cantidad_enteros;i++){
			int x = leer_entero(buffer,i);
			void* id = malloc(sizeof(int));
			id = (void*)(&x);
			if(x==NO_OP){
				printf("Me llego un %d por lo que es un NO_OP\n",x);
				list_add(lista,id);
			}else if(x==IO){
				printf("Me llego un %d por lo que es un IO\n",x);
				list_add(lista,id);
				i++;
				int y = leer_entero(buffer,i);
				void* parametro = malloc(sizeof(int));
				parametro = (void*)(&y);
				printf("Me llego un parametro: %d \n",y);
				list_add(lista,parametro);
			}else if(x==READ){
				printf("Me llego un %d por lo que es un READ\n",x);
				list_add(lista,id);
				i++;
				int y = leer_entero(buffer,i);
				void* parametro = malloc(sizeof(int));
				parametro = (void*)(&y);
				printf("Me llego un parametro: %d \n",y);
				list_add(lista,parametro);
			}else if(x==WRITE){
				printf("Me llego un %d por lo que es un WRITE\n",x);
				list_add(lista,id);
				i++;
				int y = leer_entero(buffer,i);
				void* parametro = malloc(sizeof(int));
				parametro = (void*)(&y);
				printf("Me llego un parametro: %d \n",y);
				list_add(lista,parametro);
				i++;
				int z = leer_entero(buffer,i);
				void* parametro1 = malloc(sizeof(int));
				parametro1 = (void*)(&z);
				printf("Me llego un parametro: %d \n",z);
				list_add(lista,parametro1);
			}else if(x==COPY){
				printf("Me llego un %d por lo que es un COPY\n",x);
				list_add(lista,id);
				i++;
				int y = leer_entero(buffer,i);
				void* parametro = malloc(sizeof(int));
				parametro = (void*)(&y);
				printf("Me llego un parametro: %d \n",y);
				list_add(lista,parametro);
				i++;
				int z = leer_entero(buffer,i);
				void* parametro1 = malloc(sizeof(int));
				parametro1 = (void*)(&z);
				printf("Me llego un parametro: %d \n",z);
				list_add(lista,parametro1);
			}else if(x==EXIT){
				printf("Me llego un %d por lo que es un EXIT\n",x);
				list_add(lista,id);
			}
		}

}


void leer_config(t_config* config, t_configuraciones* configuraciones){
	configuraciones->entradas_TLB = config_get_int_value(config , "ENTRADAS_TLB");
	configuraciones->reemplazo_TLB = config_get_string_value(config , "REEMPLAZO_TLB");
	configuraciones->retardo_NOOP = config_get_int_value(config , "RETARDO_NOOP");
	configuraciones->ip_memoria = config_get_string_value(config , "IP_MEMORIA");
	configuraciones->puerto_memoria = config_get_int_value(config , "PUERTO_MEMORIA");
	configuraciones->puerto_escucha_dispatch = config_get_string_value(config , "PUERTO_ESCUCHA_DISPATCH");
	configuraciones->puerto_escucha_interrupt = config_get_string_value(config , "PUERTO_ESCUCHA_INTERRUPT");
}

void liberar_memoria(t_log* logger, t_config* config , t_configuraciones* configuraciones , int servidor){
    log_destroy(logger);
    config_destroy(config);
    configuraciones_free(configuraciones);
    close(servidor);
}

void configuraciones_free(t_configuraciones* configuraciones){
    free(configuraciones);
}
