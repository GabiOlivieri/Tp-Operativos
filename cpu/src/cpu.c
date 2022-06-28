#include<cpu.h>

int interrupcion=0;

pthread_mutex_t interrupcion_mutex;
pthread_mutex_t peticion_memoria_mutex;


int main(int argc, char* argv[]) {
    t_log* logger = log_create("./cpu.log","CPU", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./cpu.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));

    leer_config(config,configuraciones);

    int servidor = iniciar_servidor(logger , "CPU Dispatch" , "127.0.0.1" , configuraciones->puerto_escucha_dispatch);
    manejar_interrupciones(logger,configuraciones);
	manejar_conexion_kernel(logger,configuraciones,servidor);
    liberar_memoria(logger,config,configuraciones,servidor);
    return 0;
}

void manejar_interrupciones(t_log* logger, t_configuraciones* configuraciones){
	int servidor = iniciar_servidor(logger , "CPU Interrupt" , "127.0.0.1" , configuraciones->puerto_escucha_interrupt);
   	int client_socket_interrupt = esperar_cliente(logger , "CPU Interrupt" , servidor);
	t_hilo_struct* hilox = malloc(sizeof(t_hilo_struct));
	hilox->logger = logger;
	hilox->socket = client_socket_interrupt;
	hilox->configuraciones = configuraciones;
	pthread_t hilo_servidor_atender_interrupcion;
    pthread_create (&hilo_servidor_atender_interrupcion, NULL , (void*) atender_interrupcion,(void*) hilox);
    pthread_detach(hilo_servidor_atender_interrupcion); 
}

void manejar_conexion_kernel(t_log* logger, t_configuraciones* configuraciones, int socket){
	while(1){
        int client_socket = esperar_cliente(logger , "CPU Dispatch" , socket);
		t_hilo_struct* hilo = malloc(sizeof(t_hilo_struct));
		hilo->logger = logger;
		hilo->socket = client_socket;
		hilo->configuraciones = configuraciones;
        pthread_t hilo_servidor;
        pthread_create (&hilo_servidor, NULL , (void*) atender_cliente,(void*) hilo);
        pthread_detach(hilo_servidor);

    }	
}

int atender_interrupcion(void* arg){
	printf("Atendedor de interrupciones\n");
	struct hilo_struct *p;
	p = (struct hilo_struct*) arg;
	while (1){
		printf("Espero interrupcion\n");
		int cod_op = recibir_operacion(p->socket);
		switch (cod_op) {		
    		case -1:
    			log_error(p->logger, "el cliente se desconecto. Terminando servidor");
    			return EXIT_FAILURE;
    		default:
				pthread_mutex_lock (&interrupcion_mutex);
				if(interrupcion != 1){
						pthread_mutex_unlock (&interrupcion_mutex);
						actualizar_interrupcion(p->logger);
					}
				pthread_mutex_unlock (&interrupcion_mutex);
    			break;
    		}
	}
	return EXIT_SUCCESS;	
}

void actualizar_interrupcion(t_log* logger){
	    log_info(logger, "Me llego una interrupcion");
       	printf("Me llego una interrupcion\n");
		pthread_mutex_lock (&interrupcion_mutex);
		interrupcion=1;
		pthread_mutex_unlock (&interrupcion_mutex);
}

int atender_cliente(void* arg){
	printf("Atendedor de clientes\n");
	struct hilo_struct *p;
	p = (struct hilo_struct*) arg;
	while (1){
		int cod_op = recibir_operacion(p->socket);
		switch (cod_op) {
    		case MENSAJE:
    			recibir_mensaje(p->socket , p->logger);
    			break;
    		case PAQUETE:
    			log_info(p->logger, "Me llegaron los siguientes valores:\n");
    			break;

    		case INICIAR_PROCESO:
    			log_info(p->logger, "Me llego un INICIAR_PROCESO\n");
    			int instruccion=0;
    			int size;
				time_t begin = time(NULL);
       			char * buffer = recibir_buffer(&size, p->socket);
       			t_pcb* pcb = recibir_pcb(buffer);
       			while (!interrupcion && instruccion != EXIT && instruccion != IO){
    			instruccion = ejecutar_instruccion(p->logger,pcb,p->configuraciones);
				}
				pthread_mutex_lock (&interrupcion_mutex);
				interrupcion=0;
				pthread_mutex_unlock (&interrupcion_mutex);
				time_t end = time(NULL);
				printf("Tardó %d segundos \n", (end - begin) );
				pcb->rafaga_anterior= (end - begin);
				//printf("Voy a devolver pcb\n");
       			devolver_pcb(pcb,p->logger,p->socket);
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

int ejecutar_instruccion(t_log* logger,t_pcb* pcb,t_configuraciones* configuraciones){
		int x = list_get(pcb->lista_instrucciones,pcb->pc);
		if (x==NO_OP) {
			printf("Eecuto un NO_OP\n");
			sleep(configuraciones->retardo_NOOP);
		}
		else if (x==IO){
			pcb->pc++;
			int y = list_get(pcb->lista_instrucciones,pcb->pc);
			printf("Eecuto un IO de argumento %d\n",y);
			pcb->tiempo_bloqueo= y;
			pcb->estado = BLOCKED;
		}
		else if (x==READ){
			t_paquete* paquete = crear_paquete();
			paquete->codigo_operacion = PRIMER_ACCESO_A_MEMORIA;
			int socket = crear_conexion(logger , "Memoria" ,configuraciones->ip_memoria , "8002");
			agregar_entero_a_paquete(paquete,pcb->pid);
			pcb->pc++;
			agregar_entero_a_paquete(paquete,5);
			enviar_paquete(paquete,socket);
			eliminar_paquete(paquete);
			pthread_mutex_lock (&peticion_memoria_mutex);
			int codigoOperacion = recibir_operacion(socket);
			pthread_mutex_unlock (&peticion_memoria_mutex);
			int size;
			char * buffer = recibir_buffer(&size, socket);
			int pid = leer_entero(buffer,0);
			printf("La conexión por READ fue exitosa y el pid %d leyó\n",pid);
		}
		else if (x==EXIT) {
			printf("Eecuto un EXIT\n");
			pcb->estado = TERMINATED;
			return x;}
		pcb->pc++;
		return x;

}


void devolver_pcb(t_pcb* pcb,t_log* logger,int socket){
	t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = DEVOLVER_PROCESO;
    int cantidad_enteros = list_size(pcb->lista_instrucciones);
    printf("Devuelvo proceso a Kernel\n");
    agregar_entero_a_paquete(paquete,pcb->pc);
    agregar_entero_a_paquete(paquete,pcb->estado);
	agregar_entero_a_paquete(paquete,pcb->tiempo_bloqueo);
	agregar_entero_a_paquete(paquete,pcb->rafaga_anterior);
    t_list_iterator* iterator = list_iterator_create(pcb->lista_instrucciones);
    while(list_iterator_has_next(iterator)){
        int ins = list_iterator_next(iterator);
        agregar_entero_a_paquete(paquete,ins);
    }
    list_iterator_destroy(iterator);
    enviar_paquete(paquete,socket);
    eliminar_paquete(paquete);
}


int hay_interrupcion(){
	// Aća va toda la logica cuando llega una interrupción para que devuelva un true
	return interrupcion;
}

t_pcb* recibir_pcb(char* buffer){
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->pid = leer_entero(buffer,0); //Variable global que se incrementa
	pcb->pc = leer_entero(buffer,1);
	printf("El Process Id del pcb recibido es: %d y su Program Counter es: %d\n",pcb->pid,pcb->pc);
	pcb->estado = RUNNING;
	pcb->tiempo_bloqueo = 0;
	pcb->lista_instrucciones = obtener_lista_instrucciones(buffer,pcb);
	return pcb;
}



t_list* obtener_lista_instrucciones(char* buffer, t_pcb* pcb){
	t_list* lista = list_create();
			int aux = 3;
			int cantidad_enteros = 3 + leer_entero(buffer,2);
			printf("La lista de instrucciones contiene: \n");
			for(; aux <= cantidad_enteros;aux++){
				int x = leer_entero(buffer,aux);
				void* id = malloc(sizeof(int));
				id = (void*)(&x);
				if(x==NO_OP){
					printf("NO_OP\n");
					list_add(lista,0);
				}else if(x==IO){
					list_add(lista,1);
					aux++;
					int y = leer_entero(buffer,aux);
					printf("IO %d \n",y);
					list_add(lista,y);
				}else if(x==READ){
					printf("Me llego un %d por lo que es un READ\n",x);
					list_add(lista,2);
					aux++;
					int y = leer_entero(buffer,aux);
					printf("READ %d \n",y);
					list_add(lista,y);
				}else if(x==WRITE){
					printf("Me llego un %d por lo que es un WRITE\n",x);
					list_add(lista,3);
					aux++;
					int y = leer_entero(buffer,aux);
					printf("Me llego un parametro: %d \n",y);
					list_add(lista,y);
					aux++;
					int z = leer_entero(buffer,aux);
					printf("WRITE %d %d \n",y,z);
					list_add(lista,z);
				}else if(x==COPY){
					printf("Me llego un %d por lo que es un COPY\n",x);
					list_add(lista,4);
					aux++;
					int y = leer_entero(buffer,aux);
					list_add(lista,y);
					aux++;
					int z = leer_entero(buffer,aux);
					printf("COPY %d %d \n",y,z);
					list_add(lista,z);
				}else if(x==EXIT){
					printf("EXIT\n",x);
					list_add(lista,5);
				}
			}
			return lista;
}


void leer_config(t_config* config, t_configuraciones* configuraciones){
	configuraciones->entradas_TLB = config_get_int_value(config , "ENTRADAS_TLB");
	configuraciones->reemplazo_TLB = config_get_string_value(config , "REEMPLAZO_TLB");
	configuraciones->retardo_NOOP = config_get_int_value(config , "RETARDO_NOOP");
	configuraciones->ip_memoria = config_get_string_value(config , "IP_MEMORIA");
	configuraciones->puerto_memoria = config_get_string_value(config , "PUERTO_MEMORIA");
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