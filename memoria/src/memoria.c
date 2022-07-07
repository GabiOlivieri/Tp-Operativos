#include<memoria.h>


char* path_swap = NULL;
char *socket_kernel = NULL;
char *socket_kernel_swap = NULL;
int numeros_tablas_primer_nivel = 0;
int numeros_tablas_segundo_nivel = 0;
void* espacio_Contiguo_En_Memoria[];
void* bitmap_memoria;

t_list* tablas_primer_nivel;
t_list* tablas_segundo_nivel;

pthread_mutex_t tablas_primer_nivel_mutex;
pthread_mutex_t tablas_segundo_nivel_mutex;
pthread_mutex_t numeros_tablas_primer_nivel_mutex;
pthread_mutex_t numeros_tablas_segundo_nivel_mutex;
pthread_mutex_t socket_kernel_mutex;
pthread_mutex_t socket_kernel_swap_mutex;

//Semaforos de while 1 con condicional
sem_t sem; // swap
sem_t kernel_mutex_binario;

sem_t cliente_servidor_kernel;



int main(int argc, char* argv[]) {
    t_log* logger = log_create("./memoria.log","MEMORIA", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./memoria.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));

    leer_config(config,configuraciones);

	sem_init(&sem, 0, 0);
	sem_init(&kernel_mutex_binario, 0, 0);
	sem_init(&cliente_servidor_kernel, 0, 1);

	espacio_Contiguo_En_Memoria[configuraciones->tam_memoria / configuraciones->tam_pagina];

	// iniciar estructuras
	tablas_primer_nivel = list_create();
	tablas_segundo_nivel = list_create();
	t_queue* cola_suspendidos = queue_create();
	t_queue* cola_procesos_a_crear = queue_create();
	init_memoria(configuraciones,logger);


    int servidor = iniciar_servidor(logger , "Memoria" , "127.0.0.1" , configuraciones->puerto_escucha);
    
	crear_modulo_swap(logger,configuraciones,cola_suspendidos);
	hilo_para_kernel(logger,configuraciones,cola_procesos_a_crear);
	manejar_conexion(logger,configuraciones,servidor,cola_suspendidos,cola_procesos_a_crear);
    liberar_memoria(logger,config,configuraciones,servidor);
    return 0;
}

void manejar_conexion(t_log* logger, t_configuraciones* configuraciones, int socket,t_queue* cola_suspendidos,t_queue* cola_procesos_a_crear){
   	while(1){
        int client_socket = esperar_cliente(logger , "Memoria" , socket);
		t_hilo_struct* hilo = malloc(sizeof(t_hilo_struct));
		hilo->logger = logger;
		hilo->socket = client_socket;
		hilo->configuraciones = configuraciones;
		hilo->cola_suspendidos = cola_suspendidos;
		hilo->cola_procesos_a_inicializar = cola_procesos_a_crear;
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

void hilo_para_kernel(t_log* logger, t_configuraciones* configuraciones,t_queue* cola_procesos_a_crear){
	t_hilo_struct_kernel* hilo = malloc(sizeof(t_hilo_struct_kernel));
	hilo->logger = logger;
	hilo->configuraciones = configuraciones;
	hilo->cola = cola_procesos_a_crear;
	pthread_t hilo_de_kernel;
    pthread_create (&hilo_de_kernel, NULL , (void*) hilo_a_kernel,(void*) hilo);
    pthread_detach(hilo_de_kernel);
}

void modulo_swap(void* arg){
	struct hilo_struct_swap *p;
	p = (struct hilo_struct_swap*) arg;
	printf("Arrancó modulo SWAP \n");
	while(1){
			printf("swap_mutex_binario lock\n");

			sem_wait(&sem);
			if(!queue_is_empty(p->cola)){
				printf("Ingresó un proceso a la cola de suspendidos \n");
				t_pcb* pcb = queue_pop(p->cola);
				pcb = bloquear_proceso(pcb,p->configuraciones);
				t_paquete* paquete = crear_paquete();
				paquete->codigo_operacion = DEVOLVER_PROCESO;
				printf("El proceso se desbloquea y vuelve a kernel\n");
				agregar_entero_a_paquete(paquete,pcb->pid);
				enviar_paquete(paquete,socket_kernel_swap);
				eliminar_paquete(paquete);
				printf("Terminó de devolver el paquete\n");
			}

	}
	printf("Termina hilo swap\n");
}

void hilo_a_kernel(void* arg){
	struct hilo_struct_kernel *p;
	p = (struct hilo_struct_kernel*) arg;
	printf("Arrancó hilo para kernel \n");
	FILE *fp;
	t_paquete* paquete;
	while(1){
			printf("kernel_mutex_binario lock\n");

			sem_wait(&kernel_mutex_binario);
			if(!queue_is_empty(p->cola)){
			printf("Ingresó un nuevo proceso \n");
			t_pcb* pcb = queue_pop(p->cola);
			if(pcb->size > p->configuraciones->tam_pagina * p->configuraciones->entradas_por_tabla * p->configuraciones->entradas_por_tabla ){
					printf("El pcb recibido tiene tamaño más grande que el direccionamiento lógico del sistema\n");
					free(pcb);
					paquete = crear_paquete();
					agregar_entero_a_paquete(paquete,-1);
					enviar_paquete(paquete, socket_kernel);
			}
			else{
				char *pidchar = {'0' + pcb->pid, '\0' };
				fp = archivo_de_swap(pidchar);
				fclose(fp);
				paquete = crear_paquete();
				paquete->codigo_operacion = ESTRUCTURAS_CREADAS;
				agregar_entero_a_paquete(paquete,pcb->pid);
				iniciar_tablas(p->configuraciones,pcb->size);
				agregar_entero_a_paquete(paquete,(numeros_tablas_primer_nivel - 1));
				usleep(p->configuraciones->retardo_memoria * 1000);
				enviar_paquete(paquete,socket_kernel);
				printf("Devuelvo el proceso %d a kernel", pcb->pid);
			}
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
	int operacion;
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
				int nro_tabla_primer_nivel = leer_entero(buffer,1);
				int entrada_tabla_primer_nivel = leer_entero(buffer,2);
				t_list* tabla_primer_nivel = list_get(tablas_primer_nivel,nro_tabla_primer_nivel);
				t_fila_tabla_paginacion_1erNivel* fila_tabla_primer_nivel = list_get(tabla_primer_nivel,entrada_tabla_primer_nivel);
				paquete = crear_paquete();
    			paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				agregar_entero_a_paquete(paquete,fila_tabla_primer_nivel->nro_tabla);
				usleep(p->configuraciones->retardo_memoria * 1000);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				break;

			case SEGUNDO_ACCESSO_A_MEMORIA:
				printf("Recibí un SEGUNDO_ACCESSO_A_MEMORIA\n");
				buffer = recibir_buffer(&size, p->socket);
				pid = leer_entero(buffer,0);
				int nro_tabla_segundo_nivel = leer_entero(buffer,1);
				int entrada_tabla_segundo_nivel = leer_entero(buffer,2);
				operacion = leer_entero(buffer,3);
				t_list* tabla_segundo_nivel = list_get(tablas_segundo_nivel,nro_tabla_segundo_nivel);
				t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel;
				switch(operacion){
					case READ:
						if(frame_valido(tabla_segundo_nivel,entrada_tabla_segundo_nivel)){
							printf("El frame está presente en memoria y va a contestarle a cpu \n");
							fila_tabla_segundo_nivel = list_get(tabla_segundo_nivel,entrada_tabla_segundo_nivel);
							fila_tabla_segundo_nivel->u = 1;
							list_replace(tabla_segundo_nivel,entrada_tabla_segundo_nivel,fila_tabla_segundo_nivel);

						}
						else{
							printf("El frame no está presente en memoria y va a contestarle a cpu con un 0 \n");
							fila_tabla_segundo_nivel = list_get(tabla_segundo_nivel,entrada_tabla_segundo_nivel);
						}
						break;
					case WRITE:
						if (!frame_valido(tabla_segundo_nivel,entrada_tabla_segundo_nivel)){
							printf("El frame no está presente en memoria y voy a escribirlo \n");
							fila_tabla_segundo_nivel = buscar_frame_libre(tabla_segundo_nivel,p->configuraciones);
						}else{
							printf("El frame está presente en memoria y voy a usarlo \n");
							fila_tabla_segundo_nivel = list_get(tabla_segundo_nivel,entrada_tabla_segundo_nivel);
							fila_tabla_segundo_nivel->u = 1;
							list_replace(tabla_segundo_nivel,entrada_tabla_segundo_nivel,fila_tabla_segundo_nivel);
						}
						break;

				}
				paquete = crear_paquete();
    			paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				agregar_entero_a_paquete(paquete,fila_tabla_segundo_nivel->marco);
				usleep(p->configuraciones->retardo_memoria * 1000);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				break;
			
			case TERCER_ACCESSO_A_MEMORIA:
				printf("Recibí un TERCER_ACCESSO_A_MEMORIA\n");
				buffer = recibir_buffer(&size, p->socket);
				pid = leer_entero(buffer,0);
				int marco = leer_entero(buffer,1);
				int desplazamiento = leer_entero(buffer,2);
				operacion = leer_entero(buffer,3);
				int valor;
				if (marco + desplazamiento > p->configuraciones->tam_memoria / p->configuraciones->tam_pagina)
					printf("La posición a la que busca acceder no existe\n");
				paquete = crear_paquete();
				paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				switch(operacion){
					case READ:
						break;
					case WRITE:
						valor = leer_entero(buffer,4);
						espacio_Contiguo_En_Memoria[marco+desplazamiento] = valor;

				}
				agregar_entero_a_paquete(paquete,espacio_Contiguo_En_Memoria[marco+desplazamiento]);
				usleep(p->configuraciones->retardo_memoria * 1000);
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
				pthread_mutex_lock (&socket_kernel_mutex);
				socket_kernel = p->socket;
				pthread_mutex_unlock (&socket_kernel_mutex);
				printf("Me llegó un INICIAR_PROCESO\n");
       			buffer = recibir_buffer(&size, p->socket);
				t_pcb* pcb = recibir_pcb(buffer,p->configuraciones);
				queue_push(p->cola_procesos_a_inicializar,pcb);
				printf("kernel_mutex_binario unlock\n");
				sem_post(&kernel_mutex_binario);

				break;

    		case ENVIAR_A_SWAP:
				pthread_mutex_lock (&socket_kernel_swap_mutex);
				socket_kernel_swap = p->socket;
				pthread_mutex_unlock (&socket_kernel_swap_mutex);
    			log_info(p->logger, "Me llego un ENVIAR_A_SWAP\n");
				printf("Me llego un ENVIAR_A_SWAP\n");
       			char * buffer_swap = recibir_buffer(&size, p->socket);
				t_pcb* pcb_swap = malloc(sizeof(t_pcb));
				pcb_swap->pid = leer_entero(buffer_swap,0);
				pcb_swap->tiempo_bloqueo = leer_entero(buffer_swap,1);
				queue_push(p->cola_suspendidos,pcb_swap);
				printf("swap_mutex_binario unlock\n");
				printf("cantidad de procesos en swap %d \n",queue_size(p->cola_suspendidos));
				sem_post(&sem);
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

bool frame_valido(t_list* tabla_segundo_nivel,int marco_solicitado){
	t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_get(tabla_segundo_nivel,marco_solicitado);
	return fila_tabla_segundo_nivel->p != 0;
}

t_fila_tabla_paginacion_2doNivel* buscar_frame_libre(t_list* tabla_segundo_nivel,t_configuraciones* configuraciones){
	t_list_iterator* iterator = list_iterator_create(tabla_segundo_nivel);
	while(list_iterator_has_next(iterator)){
        t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_iterator_next(iterator);
        printf("Analizando la entrada de segundo nivel: %d\n",iterator->index);
		if(fila_tabla_segundo_nivel-> p == 0){
			fila_tabla_segundo_nivel->p = 1;
			fila_tabla_segundo_nivel->m = 1;
			fila_tabla_segundo_nivel->u = 1;
			fila_tabla_segundo_nivel->marco = asignar_pagina_de_memoria();
			list_replace(tabla_segundo_nivel,iterator->index,fila_tabla_segundo_nivel);
			return fila_tabla_segundo_nivel;
		}
	}
}

int asignar_pagina_de_memoria(){
	for(int i = 0; i < bitarray_get_max_bit(bitmap_memoria); i++){
		if (!bitarray_test_bit(bitmap_memoria,i)){
			printf("Se carga la posición de memoria %d\n",i);
			 bitarray_set_bit(bitmap_memoria,i);
			 return i;
		}
	}
	printf("No hay memoria xd\n");
	return -1;
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

int iniciar_tablas(t_configuraciones* configuraciones,int tamano_necesario){
	t_list* tabla_paginas_primer_nivel = list_create();
	t_list* tabla_paginas_segundo_nivel = list_create();
	int filas_de_tabla_primer_nivel = 0;

	int tablas_necesarias = floor((tamano_necesario / configuraciones->tam_pagina) / configuraciones->entradas_por_tabla);
	if (tablas_necesarias == 0) tablas_necesarias = 1; // la ecuación de arriba equivale a decir tablas a llenar, pero si no llena la 0 no importa, hay que generarle una igual

		for(int i = 1; i <= configuraciones->entradas_por_tabla; i++){
			t_fila_tabla_paginacion_1erNivel* filas_tabla_primer_nivel = malloc(sizeof(t_fila_tabla_paginacion_1erNivel));
			filas_tabla_primer_nivel->nro_tabla = numeros_tablas_segundo_nivel;

			numeros_tablas_segundo_nivel++;

			t_list* tabla_paginas_segundo_nivel = list_create();

		if (tablas_necesarias){	
				for(int j = 1; j <= configuraciones->entradas_por_tabla; j++){
					
					t_fila_tabla_paginacion_2doNivel* filas_tabla_segundo_nivel = malloc(sizeof(t_fila_tabla_paginacion_2doNivel));
					filas_tabla_segundo_nivel->marco = NULL;
					filas_tabla_segundo_nivel->p = 0;
					filas_tabla_segundo_nivel->u = 0;
					filas_tabla_segundo_nivel->m = 0;
					list_add(tabla_paginas_segundo_nivel,filas_tabla_segundo_nivel);
				}
			printf("Creo pagina de segundo nivel \n");
			tablas_necesarias--;		
		}
		list_add(tablas_segundo_nivel,tabla_paginas_segundo_nivel);
		list_add(tabla_paginas_primer_nivel,filas_tabla_primer_nivel);
	}
	numeros_tablas_primer_nivel++; // Se que acá habría que poner un mutex, pero lo pones y tira un error que hace parecer al payaso de it como algo lindo
	
	list_add(tablas_primer_nivel,tabla_paginas_primer_nivel);
	
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
	for(int i = 9; i <= cantidad_enteros;i++){
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
	pcb->lista_instrucciones = lista;
	printf("El Process Id del pcb recibido es: %d y tamaño %d\n",pcb->pid,pcb->size);
	return pcb;
}

t_pcb* bloquear_proceso(t_pcb* pcb,t_configuraciones* configuraciones){
	printf("El Process Id del pcb recibido es: %d y se va a quedar: %d \n",pcb->pid,pcb->tiempo_bloqueo);
    usleep((pcb->tiempo_bloqueo * 1000) + (configuraciones->retardo_swap * 1000));
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

char* asignar_bytes(uint32_t cant_frames) {
    char* buf;
    int bytes;
    if(cant_frames < 8)
        bytes = 1;
    else
    {
        double c = (double) cant_frames;
        bytes = (int) ceil(c/8.0);
    }
    buf = malloc(bytes);
    memset(buf,0,bytes);
    return buf;
}

void init_memoria(t_configuraciones* configuraciones,t_log* logger){


    int memoriaPrincipal = malloc(sizeof(configuraciones->tam_memoria));
    if (memoriaPrincipal == NULL) {
        log_error(logger, "Error al crear espacio contiguo de la memoria principal");
    }

    t_list* listaTablasPrimerNivel = list_create();
    uint32_t cantFrames = configuraciones->tam_memoria / configuraciones->tam_pagina;
    printf("RAM FRAMES: %d \n", cantFrames);
    char* data = asignar_bytes(cantFrames);
    bitmap_memoria = bitarray_create_with_mode(data, cantFrames/8, MSB_FIRST);

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
