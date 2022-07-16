#include<kernel.h>

int pid = 0;
int procesos_en_memoria = 0;
int interrupcion_enviada = 0;
int se_ejecuto_primer_proceso=0;
int hay_proceso_bloqueado=0;

pthread_mutex_t levantar_socket_mutex;
pthread_mutex_t se_ejecuto_primer_proceso_mutex;
pthread_mutex_t interrupcion_enviada_mutex;
pthread_mutex_t hay_proceso_bloqueado_mutex;
pthread_mutex_t procesos_en_memoria_mutex;
pthread_mutex_t pid_mutex;
pthread_mutex_t cola_new_mutex;
pthread_mutex_t cola_exec_mutex;
pthread_mutex_t cola_ready_mutex;
pthread_mutex_t cola_blocked_mutex;
pthread_mutex_t cola_ready_suspended_mutex;
pthread_mutex_t cola_ready_blocked_mutex;

//Semaforos de while 1 con condicional
sem_t planificador_largo_mutex_binario;
sem_t planificador_corto_binario;
sem_t planificador_mediano_mutex_binario;
sem_t cliente_servidor;
sem_t procesos_bloqueados_binario;
sem_t blocked_suspended_a_ready_binario;




int main(int argc, char** argv) {
    t_log* logger = log_create("./kernel.log","KERNEL", false , LOG_LEVEL_DEBUG);
	t_config* config = config_create(argv[1]);
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));
    leer_config(config,configuraciones);
//	printf("El Algoritmo Planificacion elegido es %s\n",configuraciones->algoritmo_planificacion);
	t_colas_struct* colas = crear_colas();
	crear_planificadores(logger,configuraciones,colas);

	sem_init(&planificador_largo_mutex_binario, 0, 0);
	sem_init(&planificador_corto_binario, 0, 0);
	sem_init(&planificador_mediano_mutex_binario, 0, 0);
	sem_init(&cliente_servidor, 0, 1);
	sem_init(&procesos_bloqueados_binario, 0, 0);
	sem_init(&blocked_suspended_a_ready_binario, 0, 0);

	int servidor = iniciar_servidor(logger , "Kernel" , "127.0.0.1" , configuraciones->puerto_escucha);
	setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	manejar_conexion(logger,configuraciones,servidor,colas);
	liberar_memoria(logger,config,configuraciones,servidor);
	free(colas);
    return 0;
}

void planificador_largo_plazo(void* arg){
	struct planificador_struct *p;
	p = (struct planificador_struct*) arg;
	while(1){
		sem_wait (&planificador_largo_mutex_binario);
		if(!queue_is_empty(p->colas->cola_new) && procesos_en_memoria<p->configuraciones->grado_multiprogramacion){
			int size = queue_size(p->colas->cola_new);
		//	printf("La cola NEW tiene %d procesos para planificar\n",size);
			log_info(p->logger,"La cola NEW tiene %d procesos para planificar\n",size);
			t_pcb* pcb = queue_pop(p->colas->cola_new);
			pthread_mutex_lock (&cola_ready_mutex);
			queue_push(p->colas->cola_ready,pcb);
			iniciar_estructuras(p->logger,p->configuraciones,pcb);
			pthread_mutex_unlock (&cola_ready_mutex);
			pthread_mutex_lock (&interrupcion_enviada_mutex);
			interrupcion_enviada = 0;
			pthread_mutex_unlock (&interrupcion_enviada_mutex);
			pthread_mutex_lock (&procesos_en_memoria_mutex);
			procesos_en_memoria++;
		//	printf("Se agrego un proceso a la cola READY y la cantidad de procesos en memoria ahora es %d\n",++procesos_en_memoria);
			pthread_mutex_unlock (&procesos_en_memoria_mutex);
			sem_post (&planificador_corto_binario);
		}
	}
}

void planificador_corto_plazo(void* arg){
	struct planificador_struct *p;
	p = (struct planificador_struct*) arg;
	int conexion_interrupt = crear_conexion(p->logger , "CPU Interrup" , p->configuraciones->ip_cpu , p->configuraciones->puerto_cpu_interrupt);
	while(1){
		sem_wait (&planificador_corto_binario);
		pthread_mutex_lock (&cola_exec_mutex);
		pthread_mutex_lock (&cola_ready_mutex);
		if(!queue_is_empty(p->colas->cola_ready) && queue_is_empty(p->colas->cola_exec)){
			pthread_mutex_unlock (&cola_ready_mutex);
			pthread_mutex_unlock (&cola_exec_mutex);
			if( strcmp(p->configuraciones->algoritmo_planificacion,"FIFO") == 0 ){
				pthread_mutex_lock (&cola_ready_mutex);
				int size = queue_size(p->colas->cola_ready);
				pthread_mutex_unlock (&cola_ready_mutex);
				printf("La cola READY tiene %d procesos para ejecutar\n",size);
				pthread_mutex_lock (&cola_ready_mutex);
				t_pcb* pcb = queue_pop(p->colas->cola_ready);
				pthread_mutex_unlock (&cola_ready_mutex);
				pthread_mutex_lock (&cola_exec_mutex);
				queue_push(p->colas->cola_exec,pcb);
				pthread_mutex_unlock (&cola_exec_mutex);
				t_hilo_struct_standard_con_pcb* hilo = malloc(sizeof(t_hilo_struct_standard_con_pcb));
				hilo->logger = p->logger;
				hilo->configuraciones = p->configuraciones;
				hilo->colas = p->colas;
				hilo->pcb = pcb;
				pthread_t hilo_a_cpu;
				pthread_create (&hilo_a_cpu, NULL , (void*) enviar_pcb,(void*) hilo);
				pthread_detach(hilo_a_cpu);
			}else{
				t_queue* aux = queue_create();
				float aux1,aux2;
				pthread_mutex_lock (&cola_ready_mutex);
				int size = queue_size(p->colas->cola_ready);
				pthread_mutex_unlock (&cola_ready_mutex);
				int estimaciones[p->configuraciones->grado_multiprogramacion];
				printf("La cola READY tiene %d procesos para ejecutar (SRT)\n",size);
				if(size > 1){
					memset(estimaciones, 0, sizeof estimaciones);
				for(int j = 0; j < size; j++){
					pthread_mutex_lock (&cola_ready_mutex);
					t_pcb* elegido = queue_pop(p->colas->cola_ready);
					pthread_mutex_unlock (&cola_ready_mutex);
					for(int i = 0; i < (size-1); i++){

						aux1 = elegido->alfa * elegido->rafaga_anterior + (1 - elegido->alfa) * elegido->estimacion_inicial;
						pthread_mutex_lock (&cola_ready_mutex);
						t_pcb* pcb2 = queue_pop(p->colas->cola_ready);
						pthread_mutex_unlock (&cola_ready_mutex);
						aux2 = pcb2->alfa * pcb2->rafaga_anterior + (1 - pcb2->alfa) * pcb2->estimacion_inicial;

						if (aux1 == aux2){
								printf("Por FIFO tiene mas pioridad %d que %d \n",elegido->pid,pcb2->pid);
								estimaciones[i] = aux1;
								queue_push(aux,elegido);
								elegido = pcb2;
						}
						else if (aux2 < aux1){
								printf("El pcb %d tiene peor estimacion %f que %d con estimacion %f\n",elegido->pid,aux1,pcb2->pid,aux2);
								estimaciones[i] = aux2;
								queue_push(aux,pcb2);
								}
							
						else {
								printf("Un pcb %d tiene peor estimacion %f que %d con estimacion %f\n",pcb2->pid,aux2,elegido->pid,aux1);
								estimaciones[i] = aux1;
								queue_push(aux,elegido);
								elegido = pcb2;
							}
						}
						queue_push(aux,elegido);
						pthread_mutex_lock (&cola_ready_mutex);
						p->colas->cola_ready = aux;
						pthread_mutex_unlock (&cola_ready_mutex);
						}
					}
				asignar_estimaciones(estimaciones,p->colas->cola_ready);
				memset(estimaciones, 0, sizeof estimaciones);
				pthread_mutex_lock (&cola_ready_mutex);
				t_pcb* pcb = queue_pop(p->colas->cola_ready);
				pthread_mutex_unlock (&cola_ready_mutex);
				pthread_mutex_lock (&cola_exec_mutex);
				queue_push(p->colas->cola_exec,pcb);
				pthread_mutex_unlock (&cola_exec_mutex);
			//	printf("Se agrego un proceso a la cola EXEC\n");
				t_hilo_struct_standard_con_pcb* hilo = malloc(sizeof(t_hilo_struct_standard_con_pcb));
				hilo->logger = p->logger;
				hilo->configuraciones = p->configuraciones;
				hilo->colas = p->colas;
				hilo->pcb = pcb;
				pthread_mutex_lock (&interrupcion_enviada_mutex);
				interrupcion_enviada = 1;
				pthread_mutex_unlock (&interrupcion_enviada_mutex);
				pthread_t hilo_a_cpu;
				pthread_create (&hilo_a_cpu, NULL , (void*) enviar_pcb,(void*) hilo);
				pthread_detach(hilo_a_cpu);
				}
				
			}
		pthread_mutex_unlock (&cola_ready_mutex);
		pthread_mutex_unlock (&cola_exec_mutex);
		pthread_mutex_lock (&interrupcion_enviada_mutex);
		pthread_mutex_lock (&cola_exec_mutex);
		pthread_mutex_lock (&cola_ready_mutex);
		pthread_mutex_lock (&se_ejecuto_primer_proceso_mutex);
		if (!queue_is_empty(p->colas->cola_ready) && !queue_is_empty(p->colas->cola_exec) && strcmp(p->configuraciones->algoritmo_planificacion,"SRT") == 0 && !interrupcion_enviada && se_ejecuto_primer_proceso){
			pthread_mutex_unlock (&se_ejecuto_primer_proceso_mutex);
			pthread_mutex_unlock (&cola_ready_mutex);
			pthread_mutex_unlock (&cola_exec_mutex);
			pthread_mutex_unlock (&interrupcion_enviada_mutex);
			log_info(p->logger, "Envio interrupción al cpu");
			printf("Envio interrupción a cpu\n");
			pthread_mutex_lock (&interrupcion_enviada_mutex);
			interrupcion_enviada = 1;
			pthread_mutex_unlock (&interrupcion_enviada_mutex);
			t_paquete* paquete = crear_paquete();
			paquete->codigo_operacion = INICIAR_PROCESO;
			enviar_paquete(paquete,conexion_interrupt);
			eliminar_paquete(paquete);
		}
		pthread_mutex_unlock (&se_ejecuto_primer_proceso_mutex);
		pthread_mutex_unlock (&cola_ready_mutex);
		pthread_mutex_unlock (&cola_exec_mutex);
		pthread_mutex_unlock (&interrupcion_enviada_mutex);
	}
}

void asignar_estimaciones(int estimaciones[],t_queue* aux){
	t_queue* aux2 = queue_create();
	int aux_size=queue_size(aux);
	for(int i = aux_size; i < aux_size; i++){
		t_pcb* pcb = queue_pop(aux);
		pcb->estimacion_inicial = estimaciones[i];
	//	printf("Estimacion de pid %d: %d \n",pcb->pid,pcb->estimacion_inicial);
		queue_push(aux2,pcb);
	}
	for(int i = aux_size; i < aux_size; i++){
		t_pcb* pcb = queue_pop(aux2);
		queue_push(aux,pcb);
	}
}

void enviar_pcb(void* arg){
	struct hilo_struct_standard_con_pcb *p;
	p = (struct hilo_struct_standard_con_pcb*) arg;
	t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = INICIAR_PROCESO;
    int cantidad_enteros = list_size(p->pcb->lista_instrucciones);
    printf("El proceso %d es enviado a cpu\n",p->pcb->pid);
    agregar_entero_a_paquete(paquete,p->pcb->pid);
    agregar_entero_a_paquete(paquete,p->pcb->pc);
    agregar_entero_a_paquete(paquete,cantidad_enteros);
	agregar_entero_a_paquete(paquete,p->pcb->tabla_paginas);
    t_list_iterator* iterator = list_iterator_create(p->pcb->lista_instrucciones);
    while(list_iterator_has_next(iterator)){
        int ins = list_iterator_next(iterator);
        agregar_entero_a_paquete(paquete,ins);
    }
    list_iterator_destroy(iterator);
    int conexion = crear_conexion(p->logger , "CPU" , p->configuraciones->ip_cpu ,p->configuraciones->puerto_cpu_dispatch);
	enviar_paquete(paquete,conexion);
    eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(conexion);
	int size;
    char * buffer = recibir_buffer(&size, conexion);
	close(conexion);
	actualizar_pcb(buffer,p->configuraciones,p->logger,p->colas);
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
	pcb->rafaga_anterior= rafaga_anterior;
	log_info(logger,"El proceso %d tiene estado %d",pcb->pid,estado);
	pthread_mutex_lock (&se_ejecuto_primer_proceso_mutex);
	se_ejecuto_primer_proceso = 1;
	pthread_mutex_unlock (&se_ejecuto_primer_proceso_mutex);
	if(estado == BLOCKED ){
	//		printf("El proceso %d pasa a la cola BLOCK\n",pcb->pid);
			pthread_mutex_lock (&cola_blocked_mutex);
			queue_push(colas->cola_blocked,pcb);
			pthread_mutex_unlock (&cola_blocked_mutex);
			sem_post (&planificador_mediano_mutex_binario);
	}
	else if (estado == RUNNING){ // Volvió por una interrupción, entonces hay que meterlo de nuevo a Ready
		pcb->estado= READY;
		pthread_mutex_lock (&cola_ready_mutex);
		queue_push(colas->cola_ready,pcb);
		pthread_mutex_unlock (&cola_ready_mutex);
		sem_post (&planificador_corto_binario);
	}
	else if (estado == TERMINATED){
		log_info(logger, "El proceso %d terminó\n", pcb->pid);
		pthread_mutex_lock (&procesos_en_memoria_mutex);
		printf("El proceso %d terminó y la cantidad de procesos en memoria ahora es %d\n", pcb->pid,--procesos_en_memoria);
		pthread_mutex_unlock (&procesos_en_memoria_mutex);
		sem_post (&planificador_mediano_mutex_binario);
		sem_post (&planificador_corto_binario);
	}
}

void bloquear_proceso(void* arg){
	struct planificador_struct *p;
	p = (struct planificador_struct*) arg;
	while(1){
		sem_wait(&procesos_bloqueados_binario);
		if(!queue_is_empty(p->colas->cola_io)){
			t_info_bloqueado* info_nuevo_bloqueado = queue_pop(p->colas->cola_io);
			printf("Lo bloqueo/suspendo \n");
			sem_post (&planificador_corto_binario);
			usleep(info_nuevo_bloqueado->pcb->tiempo_bloqueo * 1000);
			printf("lo desbloqueo/dessuspendo \n");
			pthread_mutex_lock (&interrupcion_enviada_mutex);
			interrupcion_enviada = 0;
			pthread_mutex_unlock (&interrupcion_enviada_mutex);
			if(strcmp (info_nuevo_bloqueado->provieneDe,"BLOCK") == 0){
			//	printf("El proceso %d sale de la cola BLOCK\n",info_nuevo_bloqueado->pcb->pid);
				pthread_mutex_lock (&cola_ready_mutex);
				queue_push(p->colas->cola_ready,info_nuevo_bloqueado->pcb);
				pthread_mutex_unlock (&cola_ready_mutex);
				sem_post (&planificador_largo_mutex_binario);
				sem_post (&planificador_corto_binario);
			}
			else{
			//	printf("El proceso %d sale a la cola SUSPENDED-BLOCK\n",info_nuevo_bloqueado->pcb->pid);
				queue_push(p->colas->cola_ready_suspended,info_nuevo_bloqueado->pcb);
				sem_post (&planificador_mediano_mutex_binario);
			}
		}
	}
}

void planificador_mediano_plazo(void* arg){
	struct planificador_struct *p;
	p = (struct planificador_struct*) arg;
	t_hilo_struct_standard_con_pcb* hilo = malloc(sizeof(t_hilo_struct_standard_con_pcb));
	hilo->logger = p->logger;
	hilo->configuraciones = p->configuraciones;
	hilo->colas = p->colas;
	while(1){
		sem_wait (&planificador_mediano_mutex_binario);
		pthread_mutex_lock (&cola_blocked_mutex);
		pthread_mutex_lock (&hay_proceso_bloqueado_mutex);
		if(!queue_is_empty(p->colas->cola_blocked)){
			pthread_mutex_unlock (&hay_proceso_bloqueado_mutex);
			pthread_mutex_unlock (&cola_blocked_mutex);
		//	printf("Ingresó un proceso a la cola de bloqueados \n");
			pthread_mutex_lock (&cola_blocked_mutex);
			t_pcb* pcb = queue_pop(p->colas->cola_blocked);
			pthread_mutex_unlock (&cola_blocked_mutex);
			t_info_bloqueado* info_nuevo_bloqueado = malloc(sizeof(t_info_bloqueado));
			info_nuevo_bloqueado->pcb = pcb;
			if (pcb->tiempo_bloqueo >= p->configuraciones->tiempo_max_bloqueado){
			//	printf("El proceso %d pasa a la cola SUSPENDED-BLOCK\n",pcb->pid);
				info_nuevo_bloqueado->provieneDe = "SUSPENDED-BLOCK";
				t_paquete* paquete = crear_paquete();
				paquete->codigo_operacion = ENVIAR_A_SWAP;
				int conexion = crear_conexion(p->logger , "Memoria" , p->configuraciones->ip_memoria ,p->configuraciones->puerto_memoria);
				agregar_entero_a_paquete(paquete,pcb->pid);
				agregar_entero_a_paquete(paquete,pcb->tiempo_bloqueo);
				enviar_paquete(paquete,conexion);
				pthread_mutex_lock (&procesos_en_memoria_mutex);
				procesos_en_memoria--;
				pthread_mutex_unlock (&procesos_en_memoria_mutex);
				int codigoOperacion = recibir_operacion(conexion);
				eliminar_paquete(paquete);
				int size;
				char * buffer = recibir_buffer(&size, conexion);
				close(conexion);
				queue_push(p->colas->cola_io,info_nuevo_bloqueado);
				sem_post (&planificador_largo_mutex_binario);
				//sem_post (&planificador_corto_binario);
				
			}
			else{
				info_nuevo_bloqueado->provieneDe = "BLOCK";
				queue_push(p->colas->cola_io,info_nuevo_bloqueado);
				sem_post (&planificador_corto_binario);
			}
			sem_post(&procesos_bloqueados_binario);
		}
		pthread_mutex_unlock (&hay_proceso_bloqueado_mutex);
		pthread_mutex_unlock (&cola_blocked_mutex);
		if(!queue_is_empty(p->colas->cola_ready_suspended)){
			pthread_mutex_lock(&procesos_en_memoria_mutex);
			if (procesos_en_memoria<p->configuraciones->grado_multiprogramacion){
				t_pcb* pcb = queue_pop(p->colas->cola_ready_suspended);
				pthread_mutex_unlock (&procesos_en_memoria_mutex);
				sacar_de_swap(p->logger,p->configuraciones,pcb);
			//	printf("Sale de la cola READY SUSPENDED a READY\n");
				pthread_mutex_lock (&cola_ready_mutex);
				queue_push(p->colas->cola_ready,pcb);
				pthread_mutex_unlock (&cola_ready_mutex);
				pthread_mutex_lock(&procesos_en_memoria_mutex);
				procesos_en_memoria++;
				pthread_mutex_unlock(&procesos_en_memoria_mutex);
				pthread_mutex_lock (&interrupcion_enviada_mutex);
				interrupcion_enviada = 0;
				pthread_mutex_unlock (&interrupcion_enviada_mutex);
				sem_post (&planificador_corto_binario);	
			}
			pthread_mutex_unlock (&procesos_en_memoria_mutex);
		}
		sem_post (&planificador_largo_mutex_binario);
	}
}




t_colas_struct* crear_colas(){
	t_colas_struct* colas = malloc(sizeof(t_colas_struct));
	t_queue* cola_new = queue_create();
	t_queue* cola_ready = queue_create();
	t_queue* cola_exec = queue_create();
	t_queue* cola_blocked = queue_create();
	t_queue* cola_ready_suspended = queue_create();
	t_queue* cola_blocked_suspended = queue_create();
	t_queue* cola_io = queue_create();
	colas->cola_new = cola_new;
	colas->cola_ready = cola_ready;
	colas->cola_exec = cola_exec;
	colas->cola_blocked = cola_blocked;
	colas->cola_ready_suspended = cola_ready_suspended;
	colas->cola_blocked_suspended = cola_blocked_suspended;
	colas->cola_io = cola_io;
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
	pthread_t hilo_bloquear_proceso;
    pthread_create (&hilo_bloquear_proceso, NULL , (void*) bloquear_proceso,(void*) planificador);
    pthread_detach(hilo_bloquear_proceso);
}

void sacar_de_swap(t_log* logger,t_configuraciones* configuraciones,t_pcb* pcb){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = SACAR_DE_SWAP;
	printf("Mando SACAR_DE_SWAP a memoria\n");
	int conexion = crear_conexion(logger , "Memoria" , configuraciones->ip_memoria ,configuraciones->puerto_memoria);
	agregar_entero_a_paquete(paquete,pcb->pid);
	sem_wait(&cliente_servidor);
	enviar_paquete(paquete,conexion);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(conexion);
	sem_post(&cliente_servidor);
	int size;
	char* buffer = recibir_buffer(&size, conexion);
	close(conexion);
	int estado_deswap = leer_entero(buffer,0);
	printf("Saco de swap el proceso %d\n", pcb->pid);
}

void iniciar_estructuras(t_log* logger,t_configuraciones* configuraciones,t_pcb* pcb){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = INICIAR_PROCESO;
//	printf("Mando estructuras a memoria\n");
	int conexion = crear_conexion(logger , "Memoria" , configuraciones->ip_memoria ,configuraciones->puerto_memoria);
	agregar_entero_a_paquete(paquete,pcb->pid);
	int cantidad_enteros = list_size(pcb->lista_instrucciones);
	agregar_entero_a_paquete(paquete,cantidad_enteros);
	agregar_entero_a_paquete(paquete,pcb->size);
	agregar_entero_a_paquete(paquete,pcb->pc);
	agregar_entero_a_paquete(paquete,pcb->estimacion_inicial);
	agregar_entero_a_paquete(paquete,pcb->alfa);
	agregar_entero_a_paquete(paquete,pcb->estado);
	agregar_entero_a_paquete(paquete,pcb->tiempo_bloqueo);
	agregar_entero_a_paquete(paquete,pcb->rafaga_anterior);
	t_list_iterator* iterator = list_iterator_create(pcb->lista_instrucciones);
    while(list_iterator_has_next(iterator)){
        int ins = list_iterator_next(iterator);
        agregar_entero_a_paquete(paquete,ins);
    }
    list_iterator_destroy(iterator);
	sem_wait(&cliente_servidor);
	enviar_paquete(paquete,conexion);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(conexion);
	sem_post(&cliente_servidor);
	int size;
	char* buffer = recibir_buffer(&size, conexion);
	close(conexion);
	if(leer_entero(buffer,0) < 0){
		printf("Falló al iniciar pcb\n");
		free(pcb);
	}
	else{
		int entrada_tabla_primer_nivel = leer_entero(buffer,1);
	//	printf("Termine de recibirlo\n");
	//	printf("El proceso %d tiene la entrada de tabla de primer nivel: %d\n",pcb->pid,entrada_tabla_primer_nivel);
	//	printf("Se agrego un proceso a la cola NEW\n");
		pcb->tabla_paginas = entrada_tabla_primer_nivel;
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
			close(p->socket);
			pthread_exit(NULL);
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
		//pthread_mutex_lock(&levantar_socket_mutex);
        int client_socket = esperar_cliente(logger,"Kernel",socket);
		//pthread_mutex_unlock(&levantar_socket_mutex);
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
void iniciar_proceso(t_log* logger,int client_socket, t_configuraciones* configuraciones,t_queue* cola_new){
	log_info(logger,"Recibi un INICIAR_PROCESO desde consola\n");	
	int size;
   	char * buffer = recibir_buffer(&size, client_socket);
	t_pcb* pcb  = crear_pcb(buffer,configuraciones,logger);
	log_info(logger,"Se creo el PCB con Process Id: %d y Tamaño: %d\n",pcb->pid,pcb->size);
	pthread_mutex_lock(&cola_new_mutex);
	queue_push(cola_new,pcb);
	pthread_mutex_unlock(&cola_new_mutex);
	sem_post (&planificador_largo_mutex_binario);
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
	pthread_mutex_lock (&pid_mutex);
	pcb->pid = pid++;
	pthread_mutex_unlock (&pid_mutex); 
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
