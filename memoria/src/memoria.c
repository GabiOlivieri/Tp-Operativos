#include<memoria.h>


char* path_swap = NULL;
char *socket_cpu = NULL;

pthread_mutex_t socket_cpu_mutex;


int main(int argc, char* argv[]) {
    t_log* logger = log_create("./memoria.log","MEMORIA", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./memoria.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));

    leer_config(config,configuraciones);

	void* espacio_Contiguo_En_Memoria[configuraciones->tam_memoria];
	t_fila_tabla_paginacion_2doNivel filas_tabla_tabla_nivel[configuraciones->tam_pagina];
	t_fila_tabla_paginacion_1erNivel filas_tabla_segundo_nivel[configuraciones->tam_pagina];

    int servidor = iniciar_servidor(logger , "Memoria" , "127.0.0.1" , configuraciones->puerto_escucha);
    t_queue* cola_suspendidos = queue_create();
	crear_modulo_swap(logger,configuraciones,cola_suspendidos);
	manejar_conexion(logger,configuraciones,servidor,cola_suspendidos);
    liberar_memoria(logger,config,configuraciones,servidor);
    return 0;
}

void manejar_conexion(t_log* logger, t_configuraciones* configuraciones, int socket,t_queue* cola_suspendidos){
   	while(1){
        int client_socket = esperar_cliente(logger , "Memoria" , socket);
		t_hilo_struct* hilo = malloc(sizeof(t_hilo_struct));
		hilo->logger = logger;
		hilo->socket = client_socket;
		hilo->configuraciones = configuraciones;
		hilo->cola = cola_suspendidos;
        pthread_t hilo_servidor;
        pthread_create (&hilo_servidor, NULL , (void*) atender_cliente,(void*) hilo);
        pthread_detach(hilo_servidor);  
    }	
}

void crear_modulo_swap(t_log* logger, t_configuraciones* configuraciones,t_queue* cola_suspendidos){
	t_hilo_struct_swap* hilo = malloc(sizeof(t_hilo_struct_swap));
	hilo->logger = logger;
	hilo->configuraciones = configuraciones;
	hilo->cola = cola_suspendidos;
	pthread_t hilo_del_modulo_swap;
    pthread_create (&hilo_del_modulo_swap, NULL , (void*) modulo_swap,(void*) hilo);
    pthread_detach(hilo_del_modulo_swap);
}

void modulo_swap(void* arg){
	struct hilo_struct_swap *p;
	p = (struct hilo_struct_swap*) arg;
	printf("Arrancó modulo SWAP \n");
	while(1){
			if(!queue_is_empty(p->cola)){
			printf("Ingresó un proceso a la cola de suspendidos \n");
			t_pcb* pcb = queue_pop(p->cola);
			pcb = bloquear_proceso(pcb,p->configuraciones);
       		t_paquete* paquete = crear_paquete();
			paquete->codigo_operacion = DEVOLVER_PROCESO;
			printf("El proceso se desbloquea y vuelve a kernel\n");
			agregar_entero_a_paquete(paquete,pcb->pid);
			enviar_paquete(paquete,socket_cpu);
			eliminar_paquete(paquete);
		}
	}
}

int atender_cliente(void* arg){
	struct hilo_struct *p;
	p = (struct hilo_struct*) arg;
	int size;
	char * buffer;
	t_paquete* paquete;
	int pid;
	int direccion_logica;
	while (1){
		int cod_op = recibir_operacion(p->socket);
		switch (cod_op) {
    		case MENSAJE:
    			recibir_mensaje(p->socket , p->logger);
    			break;
    		case PAQUETE:
    			log_info(p->logger, "Me llegaron los siguientes valores:\n");
    			break;

			case PRIMER_ACCESO_A_MEMORIA:
				printf("Recibí un PRIMER_ACCESO_A_MEMORIA\n");
				buffer = recibir_buffer(&size, p->socket);
				pid = leer_entero(buffer,0);
				direccion_logica = leer_entero(buffer,1);
				paquete = crear_paquete();
    			paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				agregar_entero_a_paquete(paquete,2);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				break;

			case SEGUNDO_ACCESSO_A_MEMORIA:
				printf("Recibí un SEGUNDO_ACCESSO_A_MEMORIA\n");
				buffer = recibir_buffer(&size, p->socket);
				pid = leer_entero(buffer,0);
				direccion_logica = leer_entero(buffer,1);
				paquete = crear_paquete();
    			paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				agregar_entero_a_paquete(paquete,3);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				break;
			
			case TERCER_ACCESSO_A_MEMORIA:
				printf("Recibí un TERCER_ACCESSO_A_MEMORIA\n");
				buffer = recibir_buffer(&size, p->socket);
				pid = leer_entero(buffer,0);
				direccion_logica = leer_entero(buffer,1);
				paquete = crear_paquete();
    			paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				agregar_entero_a_paquete(paquete,5);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				break;

			case HANDSHAKE:
				printf("Recibí un HANDSHAKE\n");
    			paquete = crear_paquete();
    			paquete->codigo_operacion = HANDSHAKE;
				agregar_entero_a_paquete(paquete,p->configuraciones->entradas_por_tabla);
				agregar_entero_a_paquete(paquete,p->configuraciones->tam_pagina);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
    			break;

			case INICIAR_PROCESO:
				log_info(p->logger, "Me llego un INICIAR_PROCESO\n");
				FILE *fp;
				printf("Me llegó un INICIAR_PROCESO\n");
       			buffer = recibir_buffer(&size, p->socket);
				t_pcb* pcb = recibir_pcb(buffer,p->configuraciones);
				char *pidchar = {'0' + pcb->pid, '\0' };
				fp = archivo_de_swap(pidchar);
				fclose(fp);
				paquete = crear_paquete();
				paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pcb->pid);
				agregar_entero_a_paquete(paquete,pcb->size);
				enviar_paquete(paquete,p->socket);
				eliminar_paquete(paquete);
				break;

    		case ENVIAR_A_SWAP:
				pthread_mutex_lock (&socket_cpu_mutex);
				socket_cpu = p->socket;
				pthread_mutex_unlock (&socket_cpu_mutex);
    			log_info(p->logger, "Me llego un ENVIAR_A_SWAP\n");
				printf("Me llego un ENVIAR_A_SWAP\n");
       			char * buffer_swap = recibir_buffer(&size, p->socket);
				t_pcb* pcb_swap = malloc(sizeof(t_pcb));
				pcb_swap->pid = leer_entero(buffer_swap,0);
				pcb_swap->tiempo_bloqueo = leer_entero(buffer_swap,1);
				queue_push(p->cola,pcb_swap);
    			break;
			
    		case -1:
    			log_error(p->logger, "el cliente se desconecto. Terminando servidor");
    			return EXIT_FAILURE;
    		default:
    			log_warning(p->logger,"Operacion desconocida. No quieras meter la pata");
    			break;
    		}
	}
	return EXIT_SUCCESS;	
}


FILE* archivo_de_swap(char *pid){
		const char* barra = "/";
		const char* extension = ".swap";
		char * path = malloc(256); 
		path = strcpy(path, path_swap);	
		strcat(path, barra);
		strcat(path, &pid);
		strcat(path, extension);
		printf("%s\n", path);
		return fopen(path, "w+");
}


void devolver_pcb(t_pcb* pcb,t_log* logger,int socket){
	t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = DEVOLVER_PROCESO;
    printf("El proceso se desbloquea y vuelve a kernel\n");
    agregar_entero_a_paquete(paquete,pcb->pid);
    enviar_paquete(paquete,socket);
    eliminar_paquete(paquete);
}

t_pcb* recibir_pcb(char* buffer,t_configuraciones* configuraciones){
	t_pcb* pcb = malloc(sizeof(t_pcb));
	t_list* lista = list_create();

	pcb->pid = leer_entero(buffer,0);
	int cantidad_enteros = leer_entero(buffer,1);
	int tamanio_proceso = leer_entero(buffer,2);
	pcb->size = tamanio_proceso;
	pcb->pc = leer_entero(buffer,3);
	pcb->estimacion_inicial = leer_entero(buffer,4);
	pcb->alfa = leer_entero(buffer,5);
	pcb->estado = leer_entero(buffer,6);
	pcb->tiempo_bloqueo= leer_entero(buffer,7);
	pcb->rafaga_anterior = leer_entero(buffer,8);
	cantidad_enteros = cantidad_enteros + 8;
	printf("la cantidad de instrucciones es: %d\n",cantidad_enteros);
	for(int i = 9; i <= cantidad_enteros;i++){
		int x = leer_entero(buffer,i);
		if(x==NO_OP){
			printf("NO_OP\n");
			list_add(lista,x);
		}else if(x==IO){
			printf("IO\n");
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			list_add(lista,y);
		}else if(x==READ){
			printf("READ\n");
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			list_add(lista,y);
		}else if(x==WRITE){
			printf("WRITE\n");
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			list_add(lista,y);
			i++;
			int z = leer_entero(buffer,i);
			list_add(lista,z);
		}else if(x==COPY){
			printf("COPY\n");
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			list_add(lista,y);
			i++;
			int z = leer_entero(buffer,i);
			list_add(lista,z);
		}else if(x==EXIT){
			printf("EXIT\n");
			list_add(lista,x);
		}
	}
	pcb->lista_instrucciones = lista;
	printf("El Process Id del pcb recibido es: %d y tamaño %d\n",pcb->pid,pcb->size);
	return pcb;
}

t_pcb* bloquear_proceso(t_pcb* pcb,t_configuraciones* configuraciones){
	printf("El Process Id del pcb recibido es: %d y se va a quedar: %d \n",pcb->pid,pcb->tiempo_bloqueo);
    usleep(pcb->tiempo_bloqueo + configuraciones->retardo_swap);
	return pcb;
}


void leer_config(t_config* config, t_configuraciones* configuraciones){
	configuraciones->puerto_escucha = config_get_string_value(config , "PUERTO_ESCUCHA");
	configuraciones->tam_memoria = config_get_int_value(config , "TAM_MEMORIA");
	configuraciones->tam_pagina = config_get_int_value(config , "TAM_PAGINA");
	configuraciones->entradas_por_tabla = config_get_int_value(config , "ENTRADAS_POR_TABLA");
	configuraciones->retardo_memoria = config_get_int_value(config , "RETARDO_MEMORIA");
	configuraciones->algoritmo_reemplazo = config_get_string_value(config , "ALGORITMO_REEMPLAZO");
	configuraciones->marcos_por_proceso = config_get_int_value(config , "MARCOS_POR_PROCESO");
    configuraciones->retardo_swap = config_get_int_value(config , "RETARDO_SWAP");
	path_swap = config_get_string_value(config , "PATH_SWAP");
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
