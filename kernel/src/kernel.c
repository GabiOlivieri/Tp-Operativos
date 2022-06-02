#include<kernel.h>

int pid = 0;
int procesos_en_memoria = 0;

int main(int argc, char* argv[]) {
    t_log* logger = log_create("./kernel.log","KERNEL", false , LOG_LEVEL_DEBUG);
	t_config* config = config_create("./kernel.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));
    leer_config(config,configuraciones);
	t_colas_struct* colas = crear_colas();
	crear_planificadores(logger,configuraciones,colas);
	int servidor = iniciar_servidor(logger , "un nombre" , "127.0.0.1" , configuraciones->puerto_escucha);
	manejar_conexion(logger,configuraciones,servidor,colas->cola_new);
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
			queue_push(p->colas->cola_ready,pcb);
			printf("Se agrego un proceso a la cola READY y la cantidad de procesos en memoria ahora es %d\n",++procesos_en_memoria);
		}
	}
}

void planificador_corto_plazo(void* arg){
	struct planificador_struct *p;
	p = (struct planificador_struct*) arg;
	while(1){
		sleep(3);
		if(!queue_is_empty(p->colas->cola_ready)&&queue_is_empty(p->colas->cola_exec)){
			int size = queue_size(p->colas->cola_ready);
			printf("La cola READY tiene %d procesos para ejecutar\n",size);
			t_pcb* pcb = queue_pop(p->colas->cola_ready);
			queue_push(p->colas->cola_exec,pcb);
			printf("Se agrego un proceso a la cola EXEC\n");
		}
	}
}

t_colas_struct* crear_colas(){
	t_colas_struct* colas = malloc(sizeof(t_colas_struct));
	t_queue* cola_new = queue_create();
	t_queue* cola_ready = queue_create();
	t_queue* cola_exec = queue_create();
	colas->cola_new = cola_new;
	colas->cola_ready = cola_ready;
	colas->cola_exec = cola_exec;
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
			iniciar_proceso(p->logger,p->socket,p->configuraciones,p->cola_new);
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

void manejar_conexion(t_log* logger, t_configuraciones* configuraciones, int socket,t_queue* cola_new){
   	while(1){
        int client_socket = esperar_cliente(logger,"a escucha",socket);
		t_hilo_struct* hilo = malloc(sizeof(t_hilo_struct));
		hilo->logger = logger;
		hilo->socket = client_socket;
		hilo->configuraciones = configuraciones;
		hilo->cola_new = cola_new;
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
	log_info(logger,"Se creo el PCB con pid=%d y tamaño=%d\n",pcb->pid,pcb->size);
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
	return pcb; 
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
