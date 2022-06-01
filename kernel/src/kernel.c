#include<kernel.h>

int pid = 0;

int main(int argc, char* argv[]) {
	///home/utnso/tp-2022-1c-Champagne-SO/kernel/kernel.log Harcodeada la ruta
    t_log* logger = log_create("./kernel.log","KERNEL", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./kernel.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));

    leer_config(config,configuraciones);

    int servidor = iniciar_servidor(logger , "un nombre" , "127.0.0.1" , configuraciones->puerto_escucha);
	while(1){
		pthread_t hilo_servidor;
		int cliente_fd = esperar_cliente(logger,"un nombre",servidor);
		pthread_create(&hilo_servidor, NULL , (void*) atender_cliente,(void*) cliente_fd);
		pthread_detach(hilo_servidor);
		pthread_join(hilo_servidor,NULL);
	}
	
	
    liberar_memoria(logger,config,configuraciones,servidor);

    return 0;
}

t_pcb* crear_pcb(char* buffer,t_configuraciones* configuraciones){
	printf("Arranca decodificacion\n");
	t_list* lista = list_create();
	int tamanio_proceso = leer_entero(buffer,0);
	printf("El tamaño del proceso es: %d\n",tamanio_proceso);
	int cantidad_enteros = leer_entero(buffer,1);
	printf("La cantidad de enteros es: %d\n",cantidad_enteros);
	cantidad_enteros++;
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
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->pid = pid++; //Variable global que se incrementa
	pcb->size = tamanio_proceso;
	pcb->pc = 0;
	pcb->lista_instrucciones = lista;
	//pcb->estimacion_inicial = configuraciones->estimacion_inicial;
	//pcb->alfa = configuraciones->alfa;
	return pcb; 
}

void atender_cliente(int client_socket){
		printf("Atendiendo cliente\n");
		int cod_op = recibir_operacion(client_socket);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(client_socket , logger);
			break;
		case INICIAR_PROCESO:
			iniciar_proceso(client_socket);
			break;
		case -1:
			//log_error(logger, "el cliente se desconecto. Terminando servidor");
		default:
			//log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
}

void manejar_conexion(int server_fd){
   while(1){
        int cliente_fd = esperar_cliente(logger,"cliente",server_fd);
        pthread_t hilo_servidor;
        pthread_create (&hilo_servidor, NULL , (void*) atender_cliente,(void*) cliente_fd);
        pthread_detach(hilo_servidor);  
    }
}

void iniciar_proceso(int client_socket){
	printf("Me llego un INICIAR_PROCESO\n");	
		int size;
   		char * buffer = recibir_buffer(&size, client_socket);
		t_pcb* pcb  = crear_pcb(buffer,configuraciones);
		printf("El process id es: %d\n",pcb->pid);
		free(pcb);
}

void leer_config(t_config* config, t_configuraciones* configuraciones){
	configuraciones->ip_memoria = config_get_string_value(config , "IP_MEMORIA");
	configuraciones->puerto_memoria = config_get_int_value(config , "PUERTO_MEMORIA");
	configuraciones->ip_cpu = config_get_string_value(config , "IP_CPU");
	configuraciones->puerto_cpu_dispatch = config_get_int_value(config , "PUERTO_CPU_DISPATCH");
	configuraciones->puerto_cpu_interrupt = config_get_int_value(config , "PUERTO_CPU_INTERRUPT");
	configuraciones->puerto_escucha = config_get_string_value(config , "PUERTO_ESCUCHA");
	configuraciones->algoritmo_planificacion = config_get_string_value(config , "ALGORITMO_PLANIFICACION");
	configuraciones->estimacion_inicial = config_get_int_value(config , "ESTIMACION_INICIAL");
	configuraciones->alfa = config_get_double_value(config , "ALFA");
	configuraciones->grado_multiprogramacion = config_get_int_value(config , "GRADO_MULTIPROGRAMACION");
	configuraciones->tiempo_max_bloqueado = config_get_int_value(config , "TIEMPO_MAXIMO_BLOQUEADO");
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
