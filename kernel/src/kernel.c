#include<kernel.h>

int pid = 0;
int procesos_en_memoria = 0;
int interrupcion_enviada = 0;


int main(int argc, char* argv[]) {
    t_log* logger = log_create("./kernel.log","KERNEL", false , LOG_LEVEL_DEBUG);
	t_config* config = config_create("./kernel.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));
    leer_config(config,configuraciones);
	printf("El Algoritmo Planificacion elegido es %s\n",configuraciones->algoritmo_planificacion);
	t_colas_struct* colas = crear_colas();
	crear_planificadores(logger,configuraciones,colas);
	int servidor = iniciar_servidor(logger , "Kernel" , "127.0.0.1" , configuraciones->puerto_escucha);
	manejar_conexion(logger,configuraciones,servidor,colas);
	liberar_memoria(logger,config,configuraciones,servidor);
	free(colas);
    return 0;
}

void planificador_largo_plazo(void* arg){
	struct planificador_struct *p;
	p = (struct planificador_struct*) arg;
	while(1){
		sleep(3);
		if(!queue_is_empty(p->colas->cola_new)&&procesos_en_memoria<p->configuraciones->grado_multiprogramacion){
			int size = queue_size(p->colas->cola_new);
			printf("La cola NEW tiene %d procesos para planificar\n",size);
			t_pcb* pcb = queue_pop(p->colas->cola_new);
			//iniciar_estructuras(p->logger,p->configuraciones,pcb);
			queue_push(p->colas->cola_ready,pcb);
			printf("Se agrego un proceso a la cola READY y la cantidad de procesos en memoria ahora es %d\n",++procesos_en_memoria);
		}
	}
}

void planificador_corto_plazo(void* arg){
	struct planificador_struct *p;
	p = (struct planificador_struct*) arg;
	int conexion_interrupt = crear_conexion(p->logger , "CPU Interrup" , p->configuraciones->ip_cpu , p->configuraciones->puerto_cpu_interrupt);
	while(1){
		sleep(3);
		if(!queue_is_empty(p->colas->cola_ready) && queue_is_empty(p->colas->cola_exec)){
			interrupcion_enviada = 0;
			if( strcmp(p->configuraciones->algoritmo_planificacion,"FIFO") == 0 ){
				int size = queue_size(p->colas->cola_ready);
				printf("La cola READY tiene %d procesos para ejecutar\n",size);
			}else{
				t_queue* aux = queue_create();
				float aux1,aux2;
				int size = queue_size(p->colas->cola_ready);
				printf("La cola READY tiene %d procesos para ejecutar\n",size);
				if(size > 1){
					t_pcb* elegido = queue_pop(p->colas->cola_ready);
				for(int i = 0; i<(size-1); i++){
					printf("Entro al for por %d vez \n", i);

					aux1 = elegido->alfa * elegido->rafaga_anterior + (1 - elegido->alfa) * elegido->estimacion_inicial;
					t_pcb* pcb2 = queue_pop(p->colas->cola_ready);
					aux2 = pcb2->alfa * pcb2->rafaga_anterior + (1 - pcb2->alfa) * pcb2->estimacion_inicial;

					if (aux2 > aux1){
							printf("El pcb %d tiene mas pioridad %f que %d con pioridad %f\n",pcb2->pid,aux2,elegido->pid,aux1);
							queue_push(aux,pcb2);}
						
					else {
							printf("Un pcb %d tiene mas pioridad %f que %d con pioridad %f\n",elegido->pid,aux1,pcb2->pid,aux2);
							queue_push(aux,elegido);
							elegido = pcb2;
							printf("El elegido pasa a ser %d", elegido->pid);
						}
					}
					queue_push(aux,elegido);
					p->colas->cola_ready = aux;
					}
				t_pcb* pcb = queue_pop(p->colas->cola_ready);
				t_hilo_struct_standard_con_pcb* hilo = malloc(sizeof(t_hilo_struct_standard_con_pcb));
				hilo->logger = p->logger;
				hilo->configuraciones = p->configuraciones;
				hilo->colas = p->colas;
				hilo->pcb = pcb;
				pthread_t hilo_a_cpu;
				pthread_create (&hilo_a_cpu, NULL , (void*) hilo_enviar_pcb_cpu,(void*) hilo);
				pthread_detach(hilo_a_cpu);
				}
				
			}
		if (!queue_is_empty(p->colas->cola_ready) && !queue_is_empty(p->colas->cola_exec) && strcmp(p->configuraciones->algoritmo_planificacion,"SRT") == 0 && !interrupcion_enviada){
			log_info(p->logger, "Envio interrupción al cpu");
//			conexion_interrupt = crear_conexion(p->logger , "CPU Interrup" , p->configuraciones->ip_cpu , p->configuraciones->puerto_cpu_interrupt);
			printf("Envio interrupción a cpu\n");
			interrupcion_enviada = 1;
			t_paquete* paquete = crear_paquete();
			paquete->codigo_operacion = INICIAR_PROCESO;
			enviar_paquete(paquete,conexion_interrupt);
			eliminar_paquete(paquete);
		}
	}
}

void hilo_enviar_pcb_cpu(void* arg){
	struct hilo_struct_standard_con_pcb *p;
	p = (struct hilo_struct_standard_con_pcb*) arg;
	t_pcb* pcb = p->pcb;
	queue_push(p->colas->cola_exec,pcb);
	printf("Se agrego un proceso a la cola EXEC\n");
	enviar_pcb(p->logger,p->configuraciones,pcb,p->colas);
}

void planificador_mediano_plazo(void* arg){
	struct planificador_struct *p;
	p = (struct planificador_struct*) arg;
	printf("Arrancó planificador de mediano plazo \n");
	while(1){
		if(!queue_is_empty(p->colas->cola_suspended)){
			printf("Ingresó un proceso a la cola de suspendidos \n");
			t_pcb* pcb = queue_pop(p->colas->cola_suspended);
			t_hilo_struct_standard_con_pcb* hilo = malloc(sizeof(t_hilo_struct_standard_con_pcb));
			hilo->logger = p->logger;
			hilo->configuraciones = p->configuraciones;
			hilo->colas = p->colas;
			hilo->pcb = pcb;
			pthread_t hilo_a_memoria;
			pthread_create (&hilo_a_memoria, NULL , (void*) mandar_y_recibir_confirmacion,(void*) hilo);
			pthread_detach(hilo_a_memoria);  
		}
	}
}

int mandar_y_recibir_confirmacion(void* arg){
	struct hilo_struct_standard_con_pcb *p;
	p = (struct t_hilo_struct_standard_con_pcb*) arg;
	t_pcb* pcb = p->pcb;
	printf("El proceso %d es enviado a memoria \n",pcb->pid);
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = INICIAR_PROCESO;
	int conexion = crear_conexion(p->logger , "Memoria" , p->configuraciones->ip_memoria ,p->configuraciones->puerto_memoria);
	agregar_entero_a_paquete(paquete,pcb->pid);
	agregar_entero_a_paquete(paquete,pcb->tiempo_bloqueo);
	enviar_paquete(paquete,conexion);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(conexion);
	int size;
	char * buffer = recibir_buffer(&size, conexion);
	printf("El proceso %d sale de la cola SUSPENDED-BLOCK\n",pcb->pid);
	if (procesos_en_memoria<p->configuraciones->grado_multiprogramacion)
	queue_push(p->colas->cola_ready,pcb);
}



t_colas_struct* crear_colas(){
	t_colas_struct* colas = malloc(sizeof(t_colas_struct));
	t_queue* cola_new = queue_create();
	t_queue* cola_ready = queue_create();
	t_queue* cola_exec = queue_create();
	t_queue* cola_suspended = queue_create();
	colas->cola_new = cola_new;
	colas->cola_ready = cola_ready;
	colas->cola_exec = cola_exec;
	colas->cola_suspended = cola_suspended;
	return colas;
}

void crear_planificadores(t_log* logger, t_configuraciones* configuraciones,t_colas_struct* colas){
	t_planificador_struct* planificador = malloc(sizeof(t_planificador_struct));
	planificador->logger = logger;
	planificador->configuraciones = configuraciones;
	planificador->colas = colas;
	pthread_t hilo_planificador_largo_plazo;
    pthread_create (&hilo_planificador_largo_plazo, NULL , (void*) planificador_largo_plazo,(void*) planificador);
    pthread_detach(hilo_planificador_largo_plazo);
	pthread_t hilo_planificador_corto_plazo;
    pthread_create (&hilo_planificador_corto_plazo, NULL , (void*) planificador_corto_plazo,(void*) planificador);
    pthread_detach(hilo_planificador_corto_plazo);
	pthread_t hilo_planificador_mediano_plazo;
    pthread_create (&hilo_planificador_mediano_plazo, NULL , (void*) planificador_mediano_plazo,(void*) planificador);
    pthread_detach(hilo_planificador_mediano_plazo);
}

void iniciar_estructuras(t_log* logger, t_configuraciones* configuraciones, t_pcb* pcb){
	t_paquete* paquete = crear_paquete();
   	paquete->codigo_operacion = INICIAR_ESTRUCTURAS;
	agregar_entero_a_paquete(paquete,pcb->pid);
	int conexion = crear_conexion(logger , "Conexion con memoria" , configuraciones->ip_memoria ,configuraciones->puerto_memoria);
	enviar_paquete(paquete,conexion);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(conexion);
	int size;
    char * buffer = recibir_buffer(&size, conexion);
	close(conexion);
	int tabla_paginas = leer_entero(buffer,0);
	pcb->tabla_paginas = tabla_paginas;
}
void enviar_pcb(t_log* logger, t_configuraciones* configuraciones,t_pcb* pcb,t_colas_struct* colas){
	t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = INICIAR_PROCESO;
    int cantidad_enteros = list_size(pcb->lista_instrucciones);
    printf("El proceso %d es enviado a cpu\n",pcb->pid);
    agregar_entero_a_paquete(paquete,pcb->pid);
    agregar_entero_a_paquete(paquete,pcb->pc);
    agregar_entero_a_paquete(paquete,cantidad_enteros);
    t_list_iterator* iterator = list_iterator_create(pcb->lista_instrucciones);
    while(list_iterator_has_next(iterator)){
        int ins = list_iterator_next(iterator);
        agregar_entero_a_paquete(paquete,ins);
    }
    list_iterator_destroy(iterator);
    int conexion = crear_conexion(logger , "CPU" , configuraciones->ip_cpu ,configuraciones->puerto_cpu_dispatch);
	enviar_paquete(paquete,conexion);
    eliminar_paquete(paquete);
	int valor_semaforo;
	int codigoOperacion = recibir_operacion(conexion);
	int size;
    char * buffer = recibir_buffer(&size, conexion);
	actualizar_pcb(buffer,configuraciones,logger,colas);
}

void actualizar_pcb(char* buffer,t_configuraciones* configuraciones,t_log* logger,t_colas_struct* colas){
	log_info(logger,"Decodificando paquete\n");
	int pc = leer_entero(buffer,0);
	int estado = leer_entero(buffer,1);
	int tiempo_bloqueo = leer_entero(buffer,2);
	int rafaga_anterior = leer_entero(buffer,3);
	log_info(logger,"Decodificacion finalizada\n");
	t_pcb* pcb = queue_pop(colas->cola_exec);
	pcb->pc=pc;
	pcb->estado=estado;
	pcb->tiempo_bloqueo= tiempo_bloqueo;
	pcb->rafaga_anterior=rafaga_anterior;
	log_info(logger,"El proceso %d tiene estado %d",pcb->pid,estado);
	if(estado == BLOCKED ){
			printf("El proceso %d pasa a la cola BLOCK\n",pcb->pid);
			if (pcb->tiempo_bloqueo >= configuraciones->tiempo_max_bloqueado){
				printf("El proceso %d pasa a la cola SUSPENDED-BLOCK\n",pcb->pid);
				queue_push(colas->cola_suspended,pcb);
			}
			else{
				usleep(20);
				printf("El proceso %d sale de la cola BLOCK\n",pcb->pid);
				queue_push(colas->cola_ready,pcb);
			}
		}
	else if (estado == RUNNING){ // Volvió por una interrupción, entonces hay que meterlo de nuevo a Ready
		pcb->estado= READY;
		queue_push(colas->cola_ready,pcb);
	}
	else if (estado == TERMINATED){
		log_info(logger, "El proceso %d terminó", pcb->pid);
		printf("El proceso %d terminó y la cantidad de procesos en memoria ahora es %d\n", pcb->pid,--procesos_en_memoria);
	}
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
			iniciar_proceso(p->logger,p->socket,p->configuraciones,p->colas->cola_new);
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

void manejar_conexion(t_log* logger, t_configuraciones* configuraciones, int socket,t_colas_struct* colas){
   	while(1){
        int client_socket = esperar_cliente(logger,"CPU",socket);
		t_hilo_struct* hilo = malloc(sizeof(t_hilo_struct));
		hilo->logger = logger;
		hilo->socket = client_socket;
		hilo->configuraciones = configuraciones;
		hilo->colas = colas;
        pthread_t hilo_servidor;
        pthread_create (&hilo_servidor, NULL , (void*) atender_cliente,(void*) hilo);
        pthread_detach(hilo_servidor);  
    }	
}

int atender_cpu(void* arg){
	struct hilo_struct *p;
	p = (struct hilo_struct*) arg;
	while (1){
		int cod_op = recibir_operacion(p->socket);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(p->socket , p->logger);
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

void iniciar_proceso(t_log* logger,int client_socket, t_configuraciones* configuraciones,t_queue* cola_new){
	log_info(logger,"Recibi un INICIAR_PROCESO desde consola\n");	
	int size;
   	char * buffer = recibir_buffer(&size, client_socket);
	t_pcb* pcb  = crear_pcb(buffer,configuraciones,logger);
	log_info(logger,"Se creo el PCB con Process Id: %d y Tamaño: %d\n",pcb->pid,pcb->size);
	printf("Se agrego un proceso a la cola NEW\n");
	queue_push(cola_new,pcb);
}

t_pcb* crear_pcb(char* buffer,t_configuraciones* configuraciones,t_log* logger){
	log_info(logger,"Decodificando paquete\n");
	t_list* lista = list_create();
	int tamanio_proceso = leer_entero(buffer,0);
	int cantidad_enteros = leer_entero(buffer,1);
	cantidad_enteros++;
	for(int i = 2; i <= cantidad_enteros;i++){
		int x = leer_entero(buffer,i);
		if(x==NO_OP){
			list_add(lista,x);
		}else if(x==IO){
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			list_add(lista,y);
		}else if(x==READ){
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			list_add(lista,y);
		}else if(x==WRITE){
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			list_add(lista,y);
			i++;
			int z = leer_entero(buffer,i);
			list_add(lista,z);
		}else if(x==COPY){
			list_add(lista,x);
			i++;
			int y = leer_entero(buffer,i);
			list_add(lista,y);
			i++;
			int z = leer_entero(buffer,i);
			list_add(lista,z);
		}else if(x==EXIT){
			list_add(lista,x);
		}
	}
	log_info(logger,"Decodificacion finalizada\n");
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->pid = pid++; 
	pcb->size = tamanio_proceso;
	pcb->pc = 0;
	pcb->lista_instrucciones = lista;
	pcb->estimacion_inicial = configuraciones->estimacion_inicial;
	pcb->alfa = configuraciones->alfa;
	pcb->estado = NEW;
	pcb->tiempo_bloqueo= 0;
	pcb->rafaga_anterior = 0;
	return pcb; 
}

void leer_config(t_config* config, t_configuraciones* configuraciones){
    configuraciones->ip_memoria = config_get_string_value(config , "IP_MEMORIA");
    configuraciones->puerto_memoria = config_get_string_value(config , "PUERTO_MEMORIA");
    configuraciones->ip_cpu = config_get_string_value(config , "IP_CPU");
    configuraciones->puerto_cpu_dispatch = config_get_string_value(config , "PUERTO_CPU_DISPATCH");
    configuraciones->puerto_cpu_interrupt = config_get_string_value(config , "PUERTO_CPU_INTERRUPT");
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
