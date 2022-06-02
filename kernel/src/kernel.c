#include<kernel.h>

int pid = 0;

int main(int argc, char* argv[]) {
    t_log* logger = log_create("./kernel.log","KERNEL", false , LOG_LEVEL_DEBUG);
	t_config* config = config_create("./kernel.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));
    leer_config(config,configuraciones);
	t_queue* cola_new = queue_create();
    int servidor = iniciar_servidor(logger , "un nombre" , "127.0.0.1" , configuraciones->puerto_escucha);
	
	pthread_t hilo_planificador_largo_plazo;
    pthread_create (&hilo_planificador_largo_plazo, NULL , (void*) planificador_largo_plazo,(void*) cola_new);
    pthread_detach(hilo_planificador_largo_plazo);

	while(1){
		sleep(1);
        int cliente_fd = esperar_cliente(logger,"cliente",servidor);
		t_hilo_struct* hilo = malloc(sizeof(t_hilo_struct));
		hilo->logger = logger;
		hilo->socket = cliente_fd;
		hilo->configuraciones = configuraciones;
		hilo->cola_new = cola_new;
        pthread_t hilo_servidor;
        pthread_create (&hilo_servidor, NULL , (void*) atender_cliente,(void*) hilo);
        pthread_detach(hilo_servidor);  
    }

    liberar_memoria(logger,config,configuraciones,servidor);
	queue_destroy(cola_new);
    return 0;
}

void planificador_largo_plazo(t_queue* cola_new){
	while(1){
		sleep(3);
		if(!queue_is_empty(cola_new)){
			int size = queue_size(cola_new);
			printf("La cola NEW tiene procesos %d para planificar\n",size);	
		}
	}
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
		if(x==NO_OP){
			printf("Me llego un %d por lo que es un NO_OP\n",x);
			list_add(lista,x);
		}else if(x==IO){
			printf("Me llego un %d por lo que es un IO\n",x);
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			printf("Me llego un parametro: %d \n",y);
			list_add(lista,y);
		}else if(x==READ){
			printf("Me llego un %d por lo que es un READ\n",x);
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			printf("Me llego un parametro: %d \n",y);
			list_add(lista,y);
		}else if(x==WRITE){
			printf("Me llego un %d por lo que es un WRITE\n",x);
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			printf("Me llego un parametro: %d \n",y);
			list_add(lista,y);
			i++;
			int z = leer_entero(buffer,i);
			printf("Me llego un parametro: %d \n",z);
			list_add(lista,z);
		}else if(x==COPY){
			printf("Me llego un %d por lo que es un COPY\n",x);
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			printf("Me llego un parametro: %d \n",y);
			list_add(lista,y);
			i++;
			int z = leer_entero(buffer,i);
			printf("Me llego un parametro: %d \n",z);
			list_add(lista,z);
		}else if(x==EXIT){
			printf("Me llego un %d por lo que es un EXIT\n",x);
			list_add(lista,x);
		}
	}
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->pid = pid++; 
	pcb->size = tamanio_proceso;
	pcb->pc = 0;
	pcb->lista_instrucciones = lista;
	pcb->estimacion_inicial = configuraciones->estimacion_inicial;
	pcb->alfa = configuraciones->alfa;
	return pcb; 
}

int atender_cliente(void* arg){
	struct hilo_struct *p;
	p = (struct hilo_struct*) arg;
	while (1){
		int cod_op = recibir_operacion(p->socket);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(p->socket , p->logger);
			break;
		case INICIAR_PROCESO:
			iniciar_proceso(p->socket,p->configuraciones,p->cola_new);
			break;
		case -1:
			log_info(p->logger, "El cliente se desconecto. Terminando el hilo");
			return EXIT_FAILURE;
		default:
			log_warning(p->logger,"Operacion desconocida");
			break;
		}
	}
	return EXIT_SUCCESS;	
}

void manejar_conexion(void* arg){
	struct hilo_struct *p;
	p = (struct hilo_struct*) arg;
   	while(1){
        int client_socket = esperar_cliente(p->logger,"cliente",p->socket);
		t_hilo_struct* hilo = malloc(sizeof(t_hilo_struct));
		hilo->logger = p->logger;
		hilo->socket = client_socket;
		hilo->configuraciones = p->configuraciones;
        pthread_t hilo_servidor;
        pthread_create (&hilo_servidor, NULL , (void*) atender_cliente,(void*) hilo);
        pthread_detach(hilo_servidor);  
    }
}

void iniciar_proceso(int client_socket, t_configuraciones* configuraciones,t_queue* cola_new){
	printf("Me llego un INICIAR_PROCESO\n");	
	int size;
   	char * buffer = recibir_buffer(&size, client_socket);
	t_pcb* pcb  = crear_pcb(buffer,configuraciones);
	printf("El process id es: %d\n",pcb->pid);
	queue_push(cola_new,pcb);
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
