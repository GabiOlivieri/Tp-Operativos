#include<memoria.h>


char* path_swap = NULL;
char *socket_kernel = NULL;
char *socket_kernel_swap = NULL;
int numeros_tablas_primer_nivel = 0;
int numeros_tablas_segundo_nivel = 0;
void *espacio_Contiguo_En_Memoria;
void* bitmap_memoria;
int pid_en_cpu;

t_list* tablas_primer_nivel;
t_list* tablas_segundo_nivel;
t_list* tabla_swap;

pthread_mutex_t escribir_en_memoria;
pthread_mutex_t tabla_primer_nivel_mutex;
pthread_mutex_t tabla_segundo_nivel_mutex;
pthread_mutex_t tablas_segundo_nivel_mutex;
pthread_mutex_t cola_procesos_a_inicializar_mutex;
pthread_mutex_t cola_suspendidos_mutex;
pthread_mutex_t tablas_primer_nivel_mutex;
pthread_mutex_t tablas_segundo_nivel_mutex;
pthread_mutex_t numeros_tablas_primer_nivel_mutex;
pthread_mutex_t numeros_tablas_segundo_nivel_mutex;
pthread_mutex_t socket_kernel_mutex;
pthread_mutex_t socket_kernel_swap_mutex;
pthread_mutex_t tabla_swap_mutex;
pthread_mutex_t bitmap_memoria_mutex;
pthread_mutex_t actualizar_marco_mutex;
pthread_mutex_t pid_en_cpu_mutex;


//Semaforos de while 1 con condicional
sem_t sem; // swap
sem_t kernel_mutex_binario;
sem_t swap_mutex_binario;

//Semaforos tipo cliente servidor




int main(int argc, char** argv) {
    t_log* logger = log_create("./memoria.log","MEMORIA", false , LOG_LEVEL_TRACE);
    t_config* config = config_create(argv[1]);
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));

    leer_config(config,configuraciones);

	sem_init(&sem, 0, 0);
	sem_init(&kernel_mutex_binario, 0, 0);
	sem_init(&swap_mutex_binario, 0, 1);

	uint32_t espacio_memoria[configuraciones->tam_memoria];

	espacio_Contiguo_En_Memoria = espacio_memoria;

	// iniciar estructuras
	tablas_primer_nivel = list_create();
	tablas_segundo_nivel = list_create();
	tabla_swap = list_create();
	t_queue* cola_suspendidos = queue_create();
	t_queue* cola_procesos_a_crear = queue_create();
	init_memoria(configuraciones,logger);


    int servidor = iniciar_servidor(logger , "Memoria" , configuraciones->ip_local , configuraciones->puerto_escucha);
    setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
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
	t_queue* en_swap = queue_create();
	while(1){

			sem_wait(&sem);
			sem_wait(&swap_mutex_binario);
			pthread_mutex_lock(&cola_suspendidos_mutex);
			if(!queue_is_empty(p->cola)){
				pthread_mutex_unlock(&cola_suspendidos_mutex);
			//	printf("Ingresó un proceso a la cola de suspendidos \n");
				t_pcb* pcb = queue_pop(p->cola);
				//printf("Mando a swap un proceso\n");
				suspender(pcb,p->configuraciones);
				log_info(p->logger,"Se swapeo el proceso: %d",pcb->pid);
				t_paquete* paquete = crear_paquete();
				paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pcb->pid);
				pthread_mutex_lock (&socket_kernel_mutex);
				enviar_paquete(paquete,socket_kernel_swap);
				pthread_mutex_unlock (&socket_kernel_mutex);
				//des_suspender(pcb);
				//printf("termino de mandar a swap un proceso\n");
				sem_post(&swap_mutex_binario);
				//sem_post(&sem);
				eliminar_paquete(paquete);
			}
			sem_post(&swap_mutex_binario);
			pthread_mutex_unlock(&cola_suspendidos_mutex);
	}
}

void suspender(t_pcb* pcb, t_configuraciones* configuraciones){
	t_list_iterator* iterator = list_iterator_create(tabla_swap);
	pthread_mutex_lock(&tabla_swap_mutex);
	while(list_iterator_has_next(iterator)){
		pthread_mutex_unlock(&tabla_swap_mutex);
		pthread_mutex_lock(&tabla_swap_mutex);
    	t_fila_tabla_swap* fila_swap = list_iterator_next(iterator);
		pthread_mutex_unlock(&tabla_swap_mutex);
		if(fila_swap->pid==pcb->pid){
			t_list_iterator* iterator_swap = list_iterator_create(fila_swap->lista_datos);
			char pidchar[5];
			sprintf(pidchar, "%d", pcb->pid);
			FILE* fp = archivo_de_swap(pidchar,1);
			while(list_iterator_has_next(iterator_swap)){
				t_escritura_swap* swap = list_iterator_next(iterator_swap);
				fwrite(swap, sizeof(t_escritura_swap),1,fp);
				usleep(configuraciones->retardo_swap * 1000);
			//	printf("Marco como libre el marco de RAM :%d \n",swap->marco);
				pthread_mutex_lock(&escribir_en_memoria);
				((uint32_t *)espacio_Contiguo_En_Memoria)[swap->marco * configuraciones->tam_pagina +swap->desplazamiento] = NULL;
			//	printf("En la dir %d pongo el NULL",swap->marco * configuraciones->tam_pagina+swap->desplazamiento);
				pthread_mutex_unlock(&escribir_en_memoria);
				pthread_mutex_lock(&bitmap_memoria_mutex);
				bitarray_clean_bit(bitmap_memoria,swap->marco);
				pthread_mutex_unlock(&bitmap_memoria_mutex);
				marco_swapeado(swap->marco,1);
			}
			fclose(fp);
		}
	}
	pthread_mutex_unlock(&tabla_swap_mutex);
	
}

void marco_swapeado(int marco,int modo){
	t_list_iterator* iterator = list_iterator_create(tablas_segundo_nivel);
	pthread_mutex_lock(&tablas_segundo_nivel_mutex);
	while(list_iterator_has_next(iterator)){
		pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
		pthread_mutex_lock(&tablas_segundo_nivel_mutex);
		t_list* tabla_segundo_nivel= list_iterator_next(iterator);
		pthread_mutex_unlock(&tablas_segundo_nivel_mutex);			
		t_list_iterator* iterator_tabla = list_iterator_create(tabla_segundo_nivel);
		pthread_mutex_lock(&tabla_segundo_nivel_mutex);
		while(list_iterator_has_next(iterator_tabla)){
			pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
			pthread_mutex_lock(&tabla_segundo_nivel_mutex);
			t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_iterator_next(iterator_tabla);
			pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
			pthread_mutex_lock(&actualizar_marco_mutex);
			if(fila_tabla_segundo_nivel->marco==marco){
				pthread_mutex_unlock(&actualizar_marco_mutex);
				if(modo){
					//printf("Actualizo marco de proceso a swap\n");
					fila_tabla_segundo_nivel->p = 0;
				}else{
					//printf("Actualizo marco a presente de swap\n");
					fila_tabla_segundo_nivel->p = 1;
				}
				list_replace(tabla_segundo_nivel,iterator_tabla->index,fila_tabla_segundo_nivel);	
			}
			pthread_mutex_unlock(&actualizar_marco_mutex);
		}
		pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
	}
	pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
}

void swap(int marco,t_configuraciones* configuraciones){
	t_list_iterator* iterator = list_iterator_create(tabla_swap);
	pthread_mutex_lock(&tabla_swap_mutex);
	while(list_iterator_has_next(iterator)){
		pthread_mutex_unlock(&tabla_swap_mutex);
		pthread_mutex_lock(&tabla_swap_mutex);
    	t_fila_tabla_swap* fila_swap = list_iterator_next(iterator);
		pthread_mutex_unlock(&tabla_swap_mutex);
		t_list_iterator* iterator_swap = list_iterator_create(fila_swap->lista_datos);
		char pidchar[5];
		sprintf(pidchar, "%d", fila_swap->pid);
		pthread_mutex_lock(&tabla_swap_mutex);
		while(list_iterator_has_next(iterator_swap) && fila_swap->pid == pid_en_cpu){
			pthread_mutex_unlock(&tabla_swap_mutex);
			t_escritura_swap* swap = list_iterator_next(iterator_swap);
			if(swap->marco==marco){
				FILE* fp = archivo_de_swap(pidchar,1);
				fwrite(swap, sizeof(t_escritura_swap),1,fp);
				usleep(configuraciones->retardo_swap * 1000);
				pthread_mutex_lock(&escribir_en_memoria);
				((uint32_t *)espacio_Contiguo_En_Memoria)[swap->marco * configuraciones->tam_pagina+swap->desplazamiento] = NULL;
				pthread_mutex_unlock(&escribir_en_memoria);
				pthread_mutex_lock(&bitmap_memoria_mutex);
				//printf("Marco como libre el marco de RAM :%d \n",swap->marco);
				bitarray_clean_bit(bitmap_memoria,swap->marco);
				pthread_mutex_unlock(&bitmap_memoria_mutex);
				fclose(fp);
			}
		}
		pthread_mutex_unlock(&tabla_swap_mutex);
	}
	pthread_mutex_unlock(&tabla_swap_mutex);
}

void des_suspender(t_pcb* pcb, t_configuraciones* configuraciones){
	char pidchar[5];
	sprintf(pidchar, "%d", pcb->pid);
	FILE* fp = archivo_de_swap(pidchar,0);
	t_escritura_swap* swap = malloc(sizeof(t_escritura_swap));
	size_t resultado;
	while (!feof(fp)){
   		resultado = fread(swap, sizeof(t_escritura_swap), 1, fp);
		if (resultado != 1){
			break;
		}
		usleep(configuraciones->retardo_swap * 1000);
		int marco = swap->marco;
		pthread_mutex_lock(&escribir_en_memoria);
		if(marco_desocupado(marco)){
			pthread_mutex_unlock(&escribir_en_memoria);
			pthread_mutex_lock(&escribir_en_memoria);
			((uint32_t *)espacio_Contiguo_En_Memoria)[marco+swap->desplazamiento] = swap->valor;
			pthread_mutex_unlock(&escribir_en_memoria);
			pthread_mutex_lock(&bitmap_memoria_mutex);
			bitarray_set_bit(bitmap_memoria,swap->marco);
			pthread_mutex_unlock(&bitmap_memoria_mutex);
			marco_swapeado(swap->marco,0);
		}else{
			int marco_nuevo = asignar_pagina_de_memoria();
			actualizar_tabla(marco,marco_nuevo);
			pthread_mutex_unlock(&escribir_en_memoria);
			pthread_mutex_lock(&escribir_en_memoria);
			((uint32_t *)espacio_Contiguo_En_Memoria)[marco+swap->desplazamiento] = swap->valor;
			pthread_mutex_unlock(&escribir_en_memoria);
			marco_swapeado(swap->marco,0);
		}

		t_list_iterator* iterator = list_iterator_create(tabla_swap);
		pthread_mutex_lock(&tabla_swap_mutex);
		while(list_iterator_has_next(iterator)){
			pthread_mutex_unlock(&tabla_swap_mutex);
			pthread_mutex_lock(&tabla_swap_mutex);
			t_fila_tabla_swap* fila_swap = list_iterator_next(iterator);
			pthread_mutex_unlock(&tabla_swap_mutex);
			if(fila_swap->pid==pcb->pid){
				pthread_mutex_lock(&tabla_swap_mutex);
				list_add(fila_swap->lista_datos,swap);
				pthread_mutex_unlock(&tabla_swap_mutex);
			}
		}
		pthread_mutex_unlock(&tabla_swap_mutex);
	}
	fclose(fp);
}

void des_swap(int marco_anterior, int marco_nuevo,t_configuraciones* configuraciones,int entrada_tabla_segundo_nivel,int tabla_segundo_nivel){
	t_list_iterator* iterator = list_iterator_create(tabla_swap);
	pthread_mutex_lock(&tabla_swap_mutex);
	while(list_iterator_has_next(iterator)){
		pthread_mutex_unlock(&tabla_swap_mutex);
		pthread_mutex_lock(&tabla_swap_mutex);
    	t_fila_tabla_swap* fila_swap = list_iterator_next(iterator);
		pthread_mutex_unlock(&tabla_swap_mutex);
		t_list_iterator* iterator_swap = list_iterator_create(fila_swap->lista_datos);
		char pidchar[5];
		sprintf(pidchar, "%d", fila_swap->pid);
		pthread_mutex_lock(&pid_en_cpu_mutex);
		while(list_iterator_has_next(iterator_swap) && fila_swap->pid == pid_en_cpu){
			pthread_mutex_unlock(&pid_en_cpu_mutex);
			t_escritura_swap* swap = list_iterator_next(iterator_swap);
			if(swap->marco==marco_anterior && !(swap->valor == ((uint32_t *)espacio_Contiguo_En_Memoria)[swap->marco * configuraciones->tam_pagina +swap->desplazamiento])){
				swap->marco=marco_nuevo;
				list_replace(fila_swap->lista_datos,iterator_swap->index,swap);
				actualizar_tabla(entrada_tabla_segundo_nivel,tabla_segundo_nivel,marco_nuevo);
				FILE* fp = archivo_de_swap(pidchar,1);
				fwrite(swap, sizeof(t_escritura_swap),1,fp);
				usleep(configuraciones->retardo_swap * 1000);
				pthread_mutex_lock(&escribir_en_memoria);
				((uint32_t *)espacio_Contiguo_En_Memoria)[marco_nuevo * configuraciones->tam_pagina +swap->desplazamiento] = swap->valor;
				//printf("El nuevo valor de memoria es %d y el valor tendria que ser %d\n",((uint32_t *)espacio_Contiguo_En_Memoria)[marco_nuevo * configuraciones->tam_pagina +swap->desplazamiento],swap->valor);
				pthread_mutex_unlock(&escribir_en_memoria);
				pthread_mutex_lock(&bitmap_memoria_mutex);
				bitarray_set_bit(bitmap_memoria,marco_nuevo);
				pthread_mutex_unlock(&bitmap_memoria_mutex);
				fclose(fp);
			}
		}
		pthread_mutex_unlock(&pid_en_cpu_mutex);
	}
	pthread_mutex_unlock(&tabla_swap_mutex);
}

int marco_desocupado(int marco){
	t_list_iterator* iterator = list_iterator_create(tablas_segundo_nivel);
	pthread_mutex_lock(&tablas_segundo_nivel_mutex);
	while(list_iterator_has_next(iterator)){
		pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
		pthread_mutex_lock(&tablas_segundo_nivel_mutex);
		t_list* tabla_segundo_nivel= list_iterator_next(iterator);
		pthread_mutex_unlock(&tablas_segundo_nivel_mutex);			
		t_list_iterator* iterator_tabla = list_iterator_create(tabla_segundo_nivel);
		pthread_mutex_lock(&tabla_segundo_nivel_mutex);
		while(list_iterator_has_next(iterator_tabla)){
			pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
			pthread_mutex_lock(&tabla_segundo_nivel_mutex);
			t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_iterator_next(iterator_tabla);
			pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
			pthread_mutex_lock(&actualizar_marco_mutex);
			if(fila_tabla_segundo_nivel->marco==marco & fila_tabla_segundo_nivel->p==1){
				pthread_mutex_unlock(&actualizar_marco_mutex);
				return 0;
			}
			pthread_mutex_unlock(&actualizar_marco_mutex);
		}
		pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
	}
	pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
	return 1;
}

void actualizar_tabla(int entrada_tabla_segundo_nivel, int tabla_segundo_nivel, int marco_nuevo){
	pthread_mutex_lock(&tablas_segundo_nivel_mutex);
	t_list* tabla_segundo_nivel_del_proceso = list_get(tablas_segundo_nivel,tabla_segundo_nivel);
	pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
	t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_get(tabla_segundo_nivel_del_proceso,entrada_tabla_segundo_nivel);
		fila_tabla_segundo_nivel->marco = marco_nuevo;
		fila_tabla_segundo_nivel->p = 1;
		list_replace(tabla_segundo_nivel_del_proceso,entrada_tabla_segundo_nivel,fila_tabla_segundo_nivel);
}

int buscar_tabla_segundo_nivel(int marco){
	t_list_iterator* iterator = list_iterator_create(tablas_segundo_nivel);
	pthread_mutex_lock(&tablas_segundo_nivel_mutex);
	while(list_iterator_has_next(iterator)){
		pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
		pthread_mutex_lock(&tablas_segundo_nivel_mutex);
		t_list* tabla_segundo_nivel= list_iterator_next(iterator);
		pthread_mutex_unlock(&tablas_segundo_nivel_mutex);			
		t_list_iterator* iterator_tabla = list_iterator_create(tabla_segundo_nivel);
		pthread_mutex_lock(&tabla_segundo_nivel_mutex);
		while(list_iterator_has_next(iterator_tabla)){
			pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
			pthread_mutex_lock(&tabla_segundo_nivel_mutex);
			t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_iterator_next(iterator_tabla);
			pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
			pthread_mutex_lock(&actualizar_marco_mutex);
			if(fila_tabla_segundo_nivel->marco == marco){
				return iterator->index;
			}
			pthread_mutex_unlock(&actualizar_marco_mutex);
		}
		pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
	}
	pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
	return true;
}

void limpiar_swap(int pid){
	t_list_iterator* iterator = list_iterator_create(tabla_swap);
	int index_a_liberar = -1;
	pthread_mutex_lock(&tabla_swap_mutex);
	while(list_iterator_has_next(iterator)){
		pthread_mutex_unlock(&tabla_swap_mutex);
		pthread_mutex_lock(&tabla_swap_mutex);
    	t_fila_tabla_swap* fila_swap = list_iterator_next(iterator);
		pthread_mutex_unlock(&tabla_swap_mutex);
		if(fila_swap->pid==pid){
			index_a_liberar = iterator->index;
		}
	}
	pthread_mutex_unlock(&tabla_swap_mutex);
	pthread_mutex_lock(&tabla_swap_mutex);
	list_remove(tabla_swap,index_a_liberar);
	pthread_mutex_unlock(&tabla_swap_mutex);
	char pidchar[5];
	sprintf(pidchar, "%d", pid);
	const char* barra = "/";
	const char* extension = ".swap";
	char * path = malloc(256); 
	path = strcpy(path, path_swap);	
	strcat(path, barra);
	strcat(path, pidchar);
	strcat(path, extension);
	//printf("Se elimina el archivo %s \n", pidchar);
	remove(path);
}

void liberar_memoria_de_proceso(int pid, t_configuraciones* configuraciones,int numero_tabla_de_paginas_primer_nivel){
	pthread_mutex_lock(&tablas_primer_nivel_mutex);
	t_list* tabla_primer_nivel = list_get(tablas_primer_nivel,numero_tabla_de_paginas_primer_nivel);
	pthread_mutex_unlock(&tablas_primer_nivel_mutex);
	t_list_iterator* iterator = list_iterator_create(tabla_primer_nivel);
	pthread_mutex_lock(&tabla_primer_nivel_mutex);
	while(list_iterator_has_next(iterator)){
		pthread_mutex_unlock(&tabla_primer_nivel_mutex);
		t_fila_tabla_paginacion_1erNivel* fila_primer_nivel = list_iterator_next(iterator);
		pthread_mutex_lock(&tablas_segundo_nivel_mutex);
		t_list* tabla_segundo_nivel = list_get(tablas_segundo_nivel,fila_primer_nivel->nro_tabla);
		pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
		t_list_iterator* iterator_segundo_nivel = list_iterator_create(tabla_segundo_nivel);
		while(list_iterator_has_next(iterator_segundo_nivel)){
			t_fila_tabla_paginacion_2doNivel* fila_segundo_nivel = list_iterator_next(iterator_segundo_nivel);	
			if(fila_segundo_nivel->p == 1){
				for(int i = 0; i < configuraciones->tam_pagina ; i++ ){
				pthread_mutex_lock(&escribir_en_memoria);
				((uint32_t *)espacio_Contiguo_En_Memoria)[fila_segundo_nivel->marco * configuraciones->tam_pagina + i] = NULL;
				pthread_mutex_unlock(&escribir_en_memoria);
				}
				pthread_mutex_lock(&bitmap_memoria_mutex);
				bitarray_clean_bit(bitmap_memoria,fila_segundo_nivel->marco);
				pthread_mutex_unlock(&bitmap_memoria_mutex);
			}
		}
	}
	pthread_mutex_unlock(&tabla_primer_nivel_mutex);
}

void hilo_a_kernel(void* arg){
	struct hilo_struct_kernel *p;
	p = (struct hilo_struct_kernel*) arg;
	FILE *fp;
	t_paquete* paquete;
	while(1){

			sem_wait(&kernel_mutex_binario);
			if(!queue_is_empty(p->cola)){
	//		printf("Ingresó un nuevo proceso \n");
			pthread_mutex_lock(&cola_procesos_a_inicializar_mutex);
			t_pcb* pcb = queue_pop(p->cola);
			pthread_mutex_unlock(&cola_procesos_a_inicializar_mutex);
			if(pcb->size > p->configuraciones->tam_pagina * p->configuraciones->entradas_por_tabla * p->configuraciones->entradas_por_tabla ){
					//printf("El pcb recibido tiene tamaño más grande que el direccionamiento lógico del sistema\n");
					log_error(p->logger,"El pcb: %d tiene tamaño más grande que el direccionamiento lógico del sistema\n",pcb->pid);
					free(pcb);
					paquete = crear_paquete();
					agregar_entero_a_paquete(paquete,-1);
					pthread_mutex_lock (&socket_kernel_mutex);
					enviar_paquete(paquete, socket_kernel);
					pthread_mutex_unlock (&socket_kernel_mutex);
			}
			else{
				char pidchar[5];
				sprintf(pidchar, "%d", pcb->pid);
				fp = archivo_de_swap(pidchar,1);
				fclose(fp);
				
				t_fila_tabla_swap* fila_swap = malloc(sizeof(t_fila_tabla_swap));
				fila_swap->pid=pcb->pid;
				fila_swap->lista_datos=list_create();
				pthread_mutex_lock(&tabla_swap_mutex);
				list_add(tabla_swap,fila_swap);
				pthread_mutex_unlock(&tabla_swap_mutex);

				paquete = crear_paquete();
				paquete->codigo_operacion = ESTRUCTURAS_CREADAS;
				agregar_entero_a_paquete(paquete,pcb->pid);
				log_info(p->logger,"Iniciando estructuras del proceso: %d", pcb->pid);
				iniciar_tablas(p->configuraciones,pcb->size);
				log_info(p->logger,"Se creo el archivo swap correspondiente al proceso: %d",pcb->pid);
			//	pthread_mutex_lock(&numeros_tablas_primer_nivel_mutex);
				agregar_entero_a_paquete(paquete,(numeros_tablas_primer_nivel - 1));
			//	pthread_mutex_unlock(&numeros_tablas_primer_nivel_mutex);
				pthread_mutex_lock (&socket_kernel_mutex);
				enviar_paquete(paquete,socket_kernel);
				pthread_mutex_unlock (&socket_kernel_mutex);
			//	printf("Devuelvo el proceso %d a kernel", pcb->pid);
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

			case PRIMER_ACCESO_A_MEMORIA:
			//	printf("Recibí un PRIMER_ACCESO_A_MEMORIA\n");
				buffer = recibir_buffer(&size, p->socket);
				pid = leer_entero(buffer,0);
				pthread_mutex_lock(&pid_en_cpu_mutex);
				pid_en_cpu = pid;
				pthread_mutex_unlock(&pid_en_cpu_mutex);
				int nro_tabla_primer_nivel = leer_entero(buffer,1);
				int entrada_tabla_primer_nivel = leer_entero(buffer,2);
				pthread_mutex_lock(&tabla_primer_nivel_mutex);
				t_list* tabla_primer_nivel = list_get(tablas_primer_nivel,nro_tabla_primer_nivel);
				pthread_mutex_unlock(&tabla_primer_nivel_mutex);
				pthread_mutex_lock(&tabla_primer_nivel_mutex);
				t_fila_tabla_paginacion_1erNivel* fila_tabla_primer_nivel = list_get(tabla_primer_nivel,entrada_tabla_primer_nivel);
				pthread_mutex_unlock(&tabla_primer_nivel_mutex);
				paquete = crear_paquete();
    			paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				pthread_mutex_lock(&tabla_primer_nivel_mutex);
				agregar_entero_a_paquete(paquete,fila_tabla_primer_nivel->nro_tabla);
				pthread_mutex_unlock(&tabla_primer_nivel_mutex);
				usleep(p->configuraciones->retardo_memoria * 1000);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				close(p->socket);
				pthread_exit(NULL);
				break;

			case SEGUNDO_ACCESSO_A_MEMORIA:
			//	printf("Recibí un SEGUNDO_ACCESSO_A_MEMORIA\n");
				buffer = recibir_buffer(&size, p->socket);
				pid = leer_entero(buffer,0);
				pthread_mutex_lock(&pid_en_cpu_mutex);
				pid_en_cpu = pid;
				pthread_mutex_unlock(&pid_en_cpu_mutex);
				int nro_tabla_segundo_nivel = leer_entero(buffer,1);
				int entrada_tabla_segundo_nivel = leer_entero(buffer,2);
				operacion = leer_entero(buffer,3);
				pthread_mutex_lock(&tablas_segundo_nivel_mutex);
				t_list* tabla_segundo_nivel = list_get(tablas_segundo_nivel,nro_tabla_segundo_nivel);
				pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
				t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel;
				pthread_mutex_lock(&tabla_segundo_nivel_mutex);
				fila_tabla_segundo_nivel = list_get(tabla_segundo_nivel,entrada_tabla_segundo_nivel);
				pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
				switch(operacion){
					case READ:
						if(fila_tabla_segundo_nivel->p == 1){
							//printf("El frame está presente en memoria y va a contestarle a cpu \n");
							fila_tabla_segundo_nivel->u = 1;
							pthread_mutex_lock(&tabla_segundo_nivel_mutex);
							list_replace(tabla_segundo_nivel,entrada_tabla_segundo_nivel,fila_tabla_segundo_nivel);
							pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
						}else if (fila_tabla_segundo_nivel->p == 0 & fila_tabla_segundo_nivel->marco >= 0){
							//printf("El frame no está presente en memoria y necesito traelo del swap\n");
							int marco_anterior = fila_tabla_segundo_nivel->marco;
							//printf("Marco anterior %d \n", marco_anterior);
							int nro_tabla_primer_nivel = buscar_tabla_primer_nivel(nro_tabla_segundo_nivel);
							t_list* marcos_de_los_proceso = marcos_del_proceso(nro_tabla_primer_nivel);
							int marco;
							if(strcmp(p->configuraciones->algoritmo_reemplazo,"CLOCK") == 0){
								if (!puede_agregar_marco(nro_tabla_segundo_nivel,p->configuraciones->marcos_por_proceso)){
								  marco = realizar_reemplazo_CLOCK(marcos_de_los_proceso,nro_tabla_primer_nivel,p->configuraciones,p->logger);
								}
								else { marco = buscar_marco_libre_en_memoria();}
								//printf("Marco nuevo %d \n",marco);
								sem_wait(&swap_mutex_binario);
								des_swap(marco_anterior,marco,p->configuraciones,entrada_tabla_segundo_nivel,nro_tabla_segundo_nivel);
								sem_post(&swap_mutex_binario);
								fila_tabla_segundo_nivel = ingresar_frame_de_reemplazo(tabla_segundo_nivel,p->configuraciones,entrada_tabla_segundo_nivel,marco,0);
								}
							else{
								if (!puede_agregar_marco(nro_tabla_segundo_nivel,p->configuraciones->marcos_por_proceso)){
								  marco = realizar_reemplazo_CLOCK_MODIFICADO(marcos_de_los_proceso,nro_tabla_primer_nivel,p->configuraciones,p->logger);
								}
								else { marco = buscar_marco_libre_en_memoria();}
								sem_wait(&swap_mutex_binario);
								des_swap(marco_anterior,marco,p->configuraciones,entrada_tabla_segundo_nivel,nro_tabla_segundo_nivel);
								sem_post(&swap_mutex_binario);
								fila_tabla_segundo_nivel = ingresar_frame_de_reemplazo(tabla_segundo_nivel,p->configuraciones,entrada_tabla_segundo_nivel,marco,0);
							}
						}else{
							//printf("El frame es invalido y va a contestarle a cpu con un -1 \n");
						}
						break;
					case WRITE:
						if(fila_tabla_segundo_nivel->p == 1){
							//printf("El frame está presente en memoria y voy a usarlo \n");
							fila_tabla_segundo_nivel->u = 1;
							fila_tabla_segundo_nivel->m = 1;
							pthread_mutex_lock(&tabla_segundo_nivel_mutex);
							list_replace(tabla_segundo_nivel,entrada_tabla_segundo_nivel,fila_tabla_segundo_nivel);
							pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
						}else if (fila_tabla_segundo_nivel->p == 0 & fila_tabla_segundo_nivel->marco >= 0 & fila_tabla_segundo_nivel->p == 0){
							//printf("El frame no está presente en memoria y necesito traelo del swap\n");
							int marco_anterior = fila_tabla_segundo_nivel->marco;
							int nro_tabla_primer_nivel = buscar_tabla_primer_nivel(nro_tabla_segundo_nivel);
							t_list* marcos_de_los_proceso = marcos_del_proceso(nro_tabla_primer_nivel);
							int marco;
							if(strcmp(p->configuraciones->algoritmo_reemplazo,"CLOCK") == 0){
								if (!puede_agregar_marco(nro_tabla_segundo_nivel,p->configuraciones->marcos_por_proceso)){
								  marco = realizar_reemplazo_CLOCK(marcos_de_los_proceso,nro_tabla_primer_nivel,p->configuraciones,p->logger);
								}
								else { marco = buscar_marco_libre_en_memoria();}
								sem_wait(&swap_mutex_binario);
								des_swap(marco_anterior,marco,p->configuraciones,entrada_tabla_segundo_nivel,nro_tabla_segundo_nivel);
								sem_post(&swap_mutex_binario);
								fila_tabla_segundo_nivel = ingresar_frame_de_reemplazo(tabla_segundo_nivel,p->configuraciones,entrada_tabla_segundo_nivel,marco,1);
								}
							else{
								if (!puede_agregar_marco(nro_tabla_segundo_nivel,p->configuraciones->marcos_por_proceso)){
								  marco = realizar_reemplazo_CLOCK_MODIFICADO(marcos_de_los_proceso,nro_tabla_primer_nivel,p->configuraciones,p->logger);
								}
								else { marco = buscar_marco_libre_en_memoria();}
								sem_wait(&swap_mutex_binario);
								des_swap(marco_anterior,marco,p->configuraciones,entrada_tabla_segundo_nivel,nro_tabla_segundo_nivel);
								sem_post(&swap_mutex_binario);
								fila_tabla_segundo_nivel = ingresar_frame_de_reemplazo(tabla_segundo_nivel,p->configuraciones,entrada_tabla_segundo_nivel,marco,1);
							}
						
						}else{
							//printf("El frame no esta asignado\n");
							if (!puede_agregar_marco(nro_tabla_segundo_nivel,p->configuraciones->marcos_por_proceso)){
								//printf("Hay que reemplazar algun frame \n");
								int nro_tabla_primer_nivel = buscar_tabla_primer_nivel(nro_tabla_segundo_nivel);
								t_list* marcos_de_los_proceso = marcos_del_proceso(nro_tabla_primer_nivel);
								if(strcmp(p->configuraciones->algoritmo_reemplazo,"CLOCK") == 0){
									int marco = realizar_reemplazo_CLOCK(marcos_de_los_proceso,nro_tabla_primer_nivel,p->configuraciones,p->logger);
									fila_tabla_segundo_nivel = ingresar_frame_de_reemplazo(tabla_segundo_nivel,p->configuraciones,entrada_tabla_segundo_nivel,marco,1);
								}
								else{
									int marco = realizar_reemplazo_CLOCK_MODIFICADO(marcos_de_los_proceso,nro_tabla_primer_nivel,p->configuraciones,p->logger);
									fila_tabla_segundo_nivel = ingresar_frame_de_reemplazo(tabla_segundo_nivel,p->configuraciones,entrada_tabla_segundo_nivel,marco,1);
								}
							}else{
								//printf("Escribo en un frame nuevo \n");
								fila_tabla_segundo_nivel = buscar_frame_libre(tabla_segundo_nivel,p->configuraciones);
								pthread_mutex_lock(&tablas_segundo_nivel_mutex);
								list_replace(tablas_segundo_nivel,nro_tabla_segundo_nivel,tabla_segundo_nivel);
								pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
							}
						}
						break;
				}
				paquete = crear_paquete();
    			paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				pthread_mutex_lock(&tablas_segundo_nivel_mutex);
				agregar_entero_a_paquete(paquete,fila_tabla_segundo_nivel->marco);
				pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
				usleep(p->configuraciones->retardo_memoria * 1000);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				close(p->socket);
				pthread_exit(NULL);
				break;
			
			case TERCER_ACCESSO_A_MEMORIA:
			//	printf("Recibí un TERCER_ACCESSO_A_MEMORIA\n");
				buffer = recibir_buffer(&size, p->socket);
				pid = leer_entero(buffer,0);
				pthread_mutex_lock(&pid_en_cpu_mutex);
				pid_en_cpu = pid;
				pthread_mutex_unlock(&pid_en_cpu_mutex);
				int marco = leer_entero(buffer,1);
				int desplazamiento = leer_entero(buffer,2);
				operacion = leer_entero(buffer,3);
				uint32_t valor;
				if ( desplazamiento > p->configuraciones->tam_pagina)
					printf("Seg Fault\n");
				switch(operacion){
					case READ:
				//	printf("leo el valor %d de la dir %d \n",((uint32_t *)espacio_Contiguo_En_Memoria)[marco * p->configuraciones->tam_pagina + desplazamiento],marco * p->configuraciones->tam_pagina + desplazamiento);
					pthread_mutex_lock(&escribir_en_memoria);
					log_info(p->logger,"leo el valor %d de la dir %d \n",((uint32_t *)espacio_Contiguo_En_Memoria)[marco * p->configuraciones->tam_pagina + desplazamiento],marco * p->configuraciones->tam_pagina + desplazamiento);
					pthread_mutex_unlock(&escribir_en_memoria);
						break;
					case WRITE:
						valor = leer_entero(buffer,4);
				//		printf("Escribo en la dir : %d \n",marco * p->configuraciones->tam_pagina + desplazamiento);
						log_info(p->logger,"Escribo en la dir : %d \n",marco * p->configuraciones->tam_pagina + desplazamiento);
						pthread_mutex_lock(&escribir_en_memoria);
						((uint32_t *)espacio_Contiguo_En_Memoria)[marco * p->configuraciones->tam_pagina + desplazamiento] = valor;
						pthread_mutex_unlock(&escribir_en_memoria);
						t_list_iterator* iterator = list_iterator_create(tabla_swap);
						pthread_mutex_lock(&tabla_swap_mutex);
						while(list_iterator_has_next(iterator)){
							pthread_mutex_unlock(&tabla_swap_mutex);
        					pthread_mutex_lock(&tabla_swap_mutex);
							t_fila_tabla_swap* fila_swap = list_iterator_next(iterator);
							pthread_mutex_unlock(&tabla_swap_mutex);
							if(fila_swap->pid==pid){
								t_escritura_swap* swap = malloc(sizeof(t_escritura_swap));
								swap->marco=marco;
								swap->desplazamiento=desplazamiento;
								swap->valor=valor;
								pthread_mutex_lock(&tabla_swap_mutex);
								list_add(fila_swap->lista_datos,swap);
								pthread_mutex_unlock(&tabla_swap_mutex);
							}
						}
						pthread_mutex_unlock(&tabla_swap_mutex);
				}
				paquete = crear_paquete();
				paquete->codigo_operacion = DEVOLVER_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				//printf("marco %d \n",marco);
				//printf("desplazamiento %d \n",desplazamiento);
				pthread_mutex_lock(&escribir_en_memoria);
				agregar_entero_a_paquete(paquete,((uint32_t *)espacio_Contiguo_En_Memoria)[marco * p->configuraciones->tam_pagina + desplazamiento]);
				pthread_mutex_unlock(&escribir_en_memoria);
				usleep(p->configuraciones->retardo_memoria * 1000);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				close(p->socket);
				pthread_exit(NULL);
				
				break;

			case HANDSHAKE:
			//	printf("Recibí un HANDSHAKE\n");
    			paquete = crear_paquete();
    			paquete->codigo_operacion = HANDSHAKE;
				agregar_entero_a_paquete(paquete,p->configuraciones->entradas_por_tabla);
				agregar_entero_a_paquete(paquete,p->configuraciones->tam_pagina);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				close(p->socket);
				pthread_exit(NULL);
				
    			break;

			case INICIAR_PROCESO:
				log_info(p->logger, "Me llego un INICIAR_PROCESO\n");
				pthread_mutex_lock (&socket_kernel_mutex);
				socket_kernel = p->socket;
				pthread_mutex_unlock (&socket_kernel_mutex);
       			buffer = recibir_buffer(&size, p->socket);
				t_pcb* pcb = recibir_pcb(buffer,p->configuraciones);
				
				pthread_mutex_lock(&cola_procesos_a_inicializar_mutex);
				queue_push(p->cola_procesos_a_inicializar,pcb);
				pthread_mutex_unlock(&cola_procesos_a_inicializar_mutex);

				sem_post(&kernel_mutex_binario);
				close(p->socket);
				pthread_exit(NULL);
				
				break;

		    case SACAR_DE_SWAP:
				log_info(p->logger, "Desuspendo el proceso\n");
				char * buffer_swap_sacar = recibir_buffer(&size, p->socket);
				t_pcb* pcb_swap_sacar = malloc(sizeof(t_pcb));
				pcb_swap_sacar->pid = leer_entero(buffer_swap_sacar,0);
				//printf("Voy a sacar de swap el pid %d \n",pcb_swap_sacar->pid);
				sem_wait(&swap_mutex_binario);
				//des_suspender(pcb_swap_sacar,p->configuraciones);
				sem_post(&swap_mutex_binario);
				//printf("Lo saco de swap el pid %d \n",pcb_swap_sacar->pid);
				paquete = crear_paquete();
				paquete->codigo_operacion = FINALIZAR_PROCESO;
				agregar_entero_a_paquete(paquete,pcb_swap_sacar->pid);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				close(p->socket);
				pthread_exit(NULL);
				break;
				

    		case ENVIAR_A_SWAP:
				pthread_mutex_lock (&socket_kernel_swap_mutex);
				socket_kernel_swap = p->socket;
				pthread_mutex_unlock (&socket_kernel_swap_mutex);
    			
       			char * buffer_swap = recibir_buffer(&size, p->socket);
				t_pcb* pcb_swap = malloc(sizeof(t_pcb));
				pcb_swap->pid = leer_entero(buffer_swap,0);
				pcb_swap->tiempo_bloqueo = leer_entero(buffer_swap,1);
				log_info(p->logger, "Suspendo el proceso: %d\n",pcb_swap->pid);
				pthread_mutex_lock(&cola_suspendidos_mutex);
				queue_push(p->cola_suspendidos,pcb_swap);
				pthread_mutex_unlock(&cola_suspendidos_mutex);
				sem_post(&sem);
				close(p->socket);
				pthread_exit(NULL);
				
    			break;

			case FINALIZAR_PROCESO:
				buffer = recibir_buffer(&size, p->socket);
				pid = leer_entero(buffer,0);
				log_info(p->logger,"Finalizando el proceso: %d\n",pid);
				int entrada = leer_entero(buffer,0);
				liberar_memoria_de_proceso(pid,p->configuraciones,entrada);
				log_info(p->logger,"Se libero memoria del proceso: %d\n",pid);
				limpiar_swap(pid);
				log_info(p->logger,"Se elimino el archivo swap del proceso: %d\n",pid);
				log_info(p->logger,"Se finalizo el proceso: %d\n",pid);
				paquete = crear_paquete();
    			paquete->codigo_operacion = FINALIZAR_PROCESO;
				agregar_entero_a_paquete(paquete,pid);
				enviar_paquete(paquete, p->socket);
				eliminar_paquete(paquete);
				close(p->socket);
				pthread_exit(NULL);
				
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
	pthread_mutex_lock(&tabla_segundo_nivel_mutex);
	t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_get(tabla_segundo_nivel,marco_solicitado);
	pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
	return fila_tabla_segundo_nivel->p != 0;
}

bool puede_agregar_marco(int nro_tabla_segundo_nivel,int cant_frames_posibles){
	int nro_tabla_primer_nivel = buscar_tabla_primer_nivel(nro_tabla_segundo_nivel);
	t_list* marcos_de_los_proceso = marcos_del_proceso(nro_tabla_primer_nivel);
	//printf("El proceso tiene %d marcos\n",list_size(marcos_de_los_proceso));
	return list_size(marcos_de_los_proceso) < cant_frames_posibles;
}

bool sorteanding(t_info_marcos_por_proceso* marco1, t_info_marcos_por_proceso* marco2){
	if(marco1->entrada_segundo_nivel->marco > marco2->entrada_segundo_nivel->marco)
	return false;
	else if (marco1->entrada_segundo_nivel->marco < marco2->entrada_segundo_nivel->marco)
	return true;
}

t_list* marcos_del_proceso(int nro_tabla_primer_nivel){
	t_list* marcos_de_proceso = list_create();
	pthread_mutex_lock(&tablas_primer_nivel_mutex);
	t_list* tabla_primer_nivel = list_get(tablas_primer_nivel,nro_tabla_primer_nivel);
	pthread_mutex_unlock(&tablas_primer_nivel_mutex);
	t_list_iterator* iterator = list_iterator_create(tabla_primer_nivel);
	while(list_iterator_has_next(iterator)){
		t_fila_tabla_paginacion_1erNivel* fila_tabla_primer_nivel = list_iterator_next(iterator);
		t_list* tabla_segundo_nivel = list_get(tablas_segundo_nivel,fila_tabla_primer_nivel->nro_tabla);
		t_list_iterator* iterator_tabla_segundo_nivel = list_iterator_create(tabla_segundo_nivel);
		while(list_iterator_has_next(iterator_tabla_segundo_nivel)){
			t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_iterator_next(iterator_tabla_segundo_nivel);
			if (fila_tabla_segundo_nivel->p == 1){
	//			printf("Encontre un marco del proceso en las entradas %d y %d \n",iterator->index,iterator_tabla_segundo_nivel->index );
				t_info_marcos_por_proceso* marco_del_proceso = malloc(sizeof(t_info_marcos_por_proceso));
				marco_del_proceso->entrada_segundo_nivel = fila_tabla_segundo_nivel;
				t_indice* indice = malloc(sizeof(t_indice));
				indice->entrada_primer_nivel = iterator->index;
				indice->entrada_segundo_nivel = iterator_tabla_segundo_nivel->index;
				marco_del_proceso->index = indice;
				list_add(marcos_de_proceso,marco_del_proceso);
			}
		}
	}
	list_sort(marcos_de_proceso,sorteanding);
	return marcos_de_proceso;
}

int buscar_tabla_primer_nivel(int nro_tabla_segundo_nivel){
	t_list_iterator* iterator = list_iterator_create(tablas_primer_nivel);
	pthread_mutex_lock(&tablas_primer_nivel_mutex);
	while(list_iterator_has_next(iterator)){
		pthread_mutex_unlock(&tablas_primer_nivel_mutex);
		pthread_mutex_lock(&tablas_primer_nivel_mutex);
        t_list* tabla_primer_nivel = list_iterator_next(iterator);
		pthread_mutex_unlock(&tablas_primer_nivel_mutex);
		t_list_iterator* iterator_tabla_primer_nivel = list_iterator_create(tabla_primer_nivel);
		while(list_iterator_has_next(iterator_tabla_primer_nivel)){
			t_fila_tabla_paginacion_1erNivel* fila_tabla_primer_nivel = list_iterator_next(iterator_tabla_primer_nivel);
		//	printf("tiene la tabla de segundo nivel %d \n",fila_tabla_primer_nivel->nro_tabla);
			pthread_mutex_lock(&tablas_primer_nivel_mutex);
			if(fila_tabla_primer_nivel->nro_tabla == nro_tabla_segundo_nivel ){
				pthread_mutex_unlock(&tablas_primer_nivel_mutex);
		//		printf("El numero de tabla de segundo nivel es %d \n",nro_tabla_segundo_nivel);
		//		printf("El numero de tabla de primer nivel es %d \n",iterator->index);
				return iterator->index;
			}
			pthread_mutex_unlock(&tablas_primer_nivel_mutex);
		}
		
	}
	pthread_mutex_unlock(&tablas_primer_nivel_mutex);
}


// TODO : Cambiar para que acá haga el algoritmo de reemplazo
t_fila_tabla_paginacion_2doNivel* buscar_frame_libre(t_list* tabla_segundo_nivel,t_configuraciones* configuraciones){
	t_list_iterator* iterator = list_iterator_create(tabla_segundo_nivel);
	while(list_iterator_has_next(iterator)){
        t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_iterator_next(iterator);
        //printf("Analizando la entrada de segundo nivel: %d\n",iterator->index);
		if(fila_tabla_segundo_nivel-> p == 0){
			fila_tabla_segundo_nivel->p = 1;
			fila_tabla_segundo_nivel->m = 1;
			fila_tabla_segundo_nivel->u = 1;
			fila_tabla_segundo_nivel->marco = asignar_pagina_de_memoria();
			pthread_mutex_lock(&tabla_segundo_nivel_mutex);
			list_replace(tabla_segundo_nivel,iterator->index,fila_tabla_segundo_nivel);
			pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
			return fila_tabla_segundo_nivel;
		}
	}
}

t_fila_tabla_paginacion_2doNivel* ingresar_frame_de_reemplazo(t_list* tabla_segundo_nivel,t_configuraciones* configuraciones,int entrada, int marco,int m){
        t_fila_tabla_paginacion_2doNivel* fila_tabla_segundo_nivel = list_get(tabla_segundo_nivel,entrada);
		fila_tabla_segundo_nivel->p = 1;
		fila_tabla_segundo_nivel->m = m;
		fila_tabla_segundo_nivel->u = 1;
		pthread_mutex_lock(&actualizar_marco_mutex);
		fila_tabla_segundo_nivel->marco = marco;
		pthread_mutex_unlock(&actualizar_marco_mutex);
		pthread_mutex_lock(&tabla_segundo_nivel_mutex);
		list_replace(tabla_segundo_nivel,entrada,fila_tabla_segundo_nivel);
		pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
		return fila_tabla_segundo_nivel;
}



int asignar_pagina_de_memoria(){
	for(int i = 0; i < bitarray_get_max_bit(bitmap_memoria); i++){
		pthread_mutex_lock(&bitmap_memoria_mutex);
		if (!bitarray_test_bit(bitmap_memoria,i)){
			pthread_mutex_unlock(&bitmap_memoria_mutex);
			//printf("Se carga la posición de memoria %d\n",i);
			pthread_mutex_lock(&bitmap_memoria_mutex);
			bitarray_set_bit(bitmap_memoria,i);
			pthread_mutex_unlock(&bitmap_memoria_mutex);
			return i;
		}
		pthread_mutex_unlock(&bitmap_memoria_mutex);
	}
	//printf("No hay memoria xd\n");
	return -1;
}

int buscar_marco_libre_en_memoria(){
	for(int i = 0; i < bitarray_get_max_bit(bitmap_memoria); i++){
		pthread_mutex_lock(&bitmap_memoria_mutex);
		if (!bitarray_test_bit(bitmap_memoria,i)){
			pthread_mutex_unlock(&bitmap_memoria_mutex);
			//printf("%d se encuentra libre\n",i);
			return i;
		}
		pthread_mutex_unlock(&bitmap_memoria_mutex);
	}
	//printf("No hay memoria xd\n");
	return -1;
}


FILE* archivo_de_swap(char pid[],int modo){
		const char* barra = "/";
		const char* extension = ".swap";
		char * path = malloc(256); 
		path = strcpy(path, path_swap);	
		strcat(path, barra);
		strcat(path, pid);
		strcat(path, extension);
	//	printf("%s\n", path);
		if(modo){
			return fopen(path, "w+");
		}else{
			return fopen(path, "r");
		}
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
					filas_tabla_segundo_nivel->marco = -1; // está libre
					filas_tabla_segundo_nivel->p = 0;
					filas_tabla_segundo_nivel->u = 0;
					filas_tabla_segundo_nivel->m = 0;
					pthread_mutex_lock(&tabla_segundo_nivel_mutex);
					list_add(tabla_paginas_segundo_nivel,filas_tabla_segundo_nivel);
					pthread_mutex_unlock(&tabla_segundo_nivel_mutex);
				}
			//printf("Creo pagina de segundo nivel \n");
			tablas_necesarias--;		
		}
		pthread_mutex_lock(&tablas_segundo_nivel_mutex);
		list_add(tablas_segundo_nivel,tabla_paginas_segundo_nivel);
		pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
		pthread_mutex_lock(&tabla_primer_nivel_mutex);
		list_add(tabla_paginas_primer_nivel,filas_tabla_primer_nivel);
		pthread_mutex_unlock(&tabla_primer_nivel_mutex);
	}
	numeros_tablas_primer_nivel++; // Se que acá habría que poner un mutex, pero lo pones y tira un error que hace parecer al payaso de it como algo lindo
	
	pthread_mutex_lock(&tablas_primer_nivel_mutex);
	list_add(tablas_primer_nivel,tabla_paginas_primer_nivel);
	pthread_mutex_unlock(&tablas_primer_nivel_mutex);
}

int realizar_reemplazo_CLOCK(t_list* marcosProceso, int nro_tabla_primer_nivel,t_configuraciones* configuraciones, t_log* logger)
//paginas_ppal lista de paginas del proceso
{
	int punteroClock = 0;
	t_info_marcos_por_proceso* recorredorPaginas;
	int cantidadFrames = list_size(marcosProceso);
	//printf("Cantidad de frames del proceso: %d\n",cantidadFrames);
//	printf("Voy a analizar los %d frames del proceso\n", cantidadFrames);
	int marco;

	//esta es la primera vuelta para encontrar 0 modificando el bit de uso si esta en 1
	for(int i = 0; i < cantidadFrames ; i++){
		if(punteroClock == cantidadFrames) // Vuelve a poner arriba al puntero
		{
			punteroClock = 0;
		}

		recorredorPaginas = list_get(marcosProceso, punteroClock);
		punteroClock++;

		//printf("El marco sospechoso tiene u: %d y m: %d y marco: %d \n",recorredorPaginas->entrada_segundo_nivel->u,recorredorPaginas->entrada_segundo_nivel->m,recorredorPaginas->entrada_segundo_nivel->marco);

	//	printf("Bit de uso == %d \n",recorredorPaginas->entrada_segundo_nivel->u);

		if(recorredorPaginas->entrada_segundo_nivel->u == 0 ){
			marco = reemplazar_marco(recorredorPaginas,nro_tabla_primer_nivel,configuraciones);
			log_info(logger,"Victima CLOCK: pagina:%d - marco:%d \n", marco,
				recorredorPaginas->entrada_segundo_nivel->marco);
			return marco; // Para que lo retorna??
		} 
		//printf("El el marco tenia bit de u en 1 \n");
		actualizar_bit_u(recorredorPaginas,nro_tabla_primer_nivel);
	}

	//esta segunda vuelta es para encontrar 0 
	for(int i = 0; i < cantidadFrames ; i++){
		if(punteroClock == cantidadFrames)
		{
			punteroClock = 0;
		}

		recorredorPaginas = list_get(marcosProceso, punteroClock);
		punteroClock++;

		//printf("El marco sospechoso tiene u: %d y m: %d \n",recorredorPaginas->entrada_segundo_nivel->u,recorredorPaginas->entrada_segundo_nivel->m);

		if(recorredorPaginas->entrada_segundo_nivel->u == 0 ){
			marco = reemplazar_marco(recorredorPaginas,nro_tabla_primer_nivel,configuraciones);
			log_info(logger,"Victima CLOCK: pagina:%d - marco:%d \n", marco,
				recorredorPaginas->entrada_segundo_nivel->marco);
			return marco; // Para que lo retorna??
		} 
	}

}

int realizar_reemplazo_CLOCK_MODIFICADO(t_list* marcosProceso, int nro_tabla_primer_nivel,t_configuraciones* configuraciones,t_log* logger)
//paginas_ppal lista de paginas del proceso
{
    
	int punteroClock = 0;
	t_info_marcos_por_proceso* recorredorPaginas;
	int cantidadFrames = list_size(marcosProceso);
//	printf("Voy a analizar los %d frames del proceso\n", cantidadFrames);
	int marco;

	//esta es la primera vuelta para encontrar 0|0
	for(int i = 0; i < cantidadFrames ; i++){
		if(punteroClock == cantidadFrames) // Vuelve a poner arriba al puntero
		{
			punteroClock = 0;
		}

		recorredorPaginas = list_get(marcosProceso, punteroClock);
		punteroClock++;

		if(recorredorPaginas->entrada_segundo_nivel->u == 0 && recorredorPaginas->entrada_segundo_nivel->m == 0 ){
			marco = reemplazar_marco(recorredorPaginas,nro_tabla_primer_nivel,configuraciones);
			log_info(logger,"Victima CLOCK-M: pagina:%d - marco:%d \n", marco,
				recorredorPaginas->entrada_segundo_nivel->marco);
			return marco; // Para que lo retorna??
		}

	}

	//esta segunda vuelta es para encontrar 0|1 modificando el bit de uso
	for(int i = 0; i < cantidadFrames ; i++){
		if(punteroClock == cantidadFrames)
		{
			punteroClock = 0;
		}

		recorredorPaginas = list_get(marcosProceso, punteroClock);
		punteroClock++;

		if(recorredorPaginas->entrada_segundo_nivel->u == 0 && recorredorPaginas->entrada_segundo_nivel->m == 1 ){
			marco = reemplazar_marco(recorredorPaginas,nro_tabla_primer_nivel,configuraciones);
			log_info(logger,"Victima CLOCK-M: pagina:%d - marco:%d \n", marco,
				recorredorPaginas->entrada_segundo_nivel->marco);
			return marco; // Para que lo retorna??
		}

		actualizar_bit_u(recorredorPaginas,nro_tabla_primer_nivel);

	}

	//esta tercera vuelta es para encontrar 0|0
	for(int i = 0; i < cantidadFrames ; i++){
		if(punteroClock == cantidadFrames)
		{
			punteroClock = 0;
		}

		recorredorPaginas = list_get(marcosProceso, punteroClock);
		punteroClock++;

		if(recorredorPaginas->entrada_segundo_nivel->u == 0 && recorredorPaginas->entrada_segundo_nivel->m == 0 ){
			marco = reemplazar_marco(recorredorPaginas,nro_tabla_primer_nivel,configuraciones);
			log_info(logger,"Victima CLOCK-M: pagina:%d - marco:%d \n", marco,
				recorredorPaginas->entrada_segundo_nivel->marco);
			return marco; // Para que lo retorna??
		}

	}

	//esta segunda vuelta es para encontrar 0|1
	for(int i = 0; i < cantidadFrames ; i++){
		if(punteroClock == cantidadFrames)
		{
			punteroClock = 0;
		}

		recorredorPaginas = list_get(marcosProceso, punteroClock);
		punteroClock++;

		if(recorredorPaginas->entrada_segundo_nivel->u == 0 && recorredorPaginas->entrada_segundo_nivel->m == 1 ){
			marco = reemplazar_marco(recorredorPaginas,nro_tabla_primer_nivel,configuraciones);
			log_info(logger,"Victima CLOCK-M: pagina:%d - marco:%d \n", marco,
				recorredorPaginas->entrada_segundo_nivel->marco);
			return marco; // Para que lo retorna??
		}
	}
}

int reemplazar_marco(t_info_marcos_por_proceso* recorredorPaginas,int nro_tabla_primer_nivel,t_configuraciones* configuraciones){
	pthread_mutex_lock(&tablas_primer_nivel_mutex);
	t_list* tabla_primer_nivel = list_get(tablas_primer_nivel,nro_tabla_primer_nivel);
	pthread_mutex_unlock(&tablas_primer_nivel_mutex);
	t_fila_tabla_paginacion_1erNivel* fila_tabla_paginacion_1erNivel = list_get(tabla_primer_nivel,recorredorPaginas->index->entrada_primer_nivel);
	pthread_mutex_lock(&tablas_segundo_nivel_mutex);
	t_list* tabla_segundo_nivel = list_get(tablas_segundo_nivel,fila_tabla_paginacion_1erNivel->nro_tabla);
	pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
	t_fila_tabla_paginacion_2doNivel* fila_tabla_paginacion_2doNivel = list_get(tabla_segundo_nivel,recorredorPaginas->index->entrada_segundo_nivel); 

	pthread_mutex_lock(&actualizar_marco_mutex);
	int marco_a_asignar = fila_tabla_paginacion_2doNivel->marco;
	pthread_mutex_unlock(&actualizar_marco_mutex);
	sem_wait(&swap_mutex_binario);
	swap(marco_a_asignar,configuraciones);
	sem_post(&swap_mutex_binario);
	fila_tabla_paginacion_2doNivel->p = 0;

	//printf("La fila tenia u: %d y m: %d \n",fila_tabla_paginacion_2doNivel->u,fila_tabla_paginacion_2doNivel->m);

	//printf("Asigno pagina %d a marco\n", marco_a_asignar);

	list_replace(tabla_segundo_nivel,recorredorPaginas->index->entrada_segundo_nivel,fila_tabla_paginacion_2doNivel);
	pthread_mutex_lock(&tablas_segundo_nivel_mutex);
	list_replace(tablas_segundo_nivel,fila_tabla_paginacion_1erNivel->nro_tabla,tabla_segundo_nivel);
	pthread_mutex_unlock(&tablas_segundo_nivel_mutex);

	return marco_a_asignar;
}

void actualizar_bit_u(t_info_marcos_por_proceso* recorredorPaginas,int nro_tabla_primer_nivel){
	pthread_mutex_lock(&tablas_primer_nivel_mutex);
	t_list* tabla_primer_nivel = list_get(tablas_primer_nivel,nro_tabla_primer_nivel);
	pthread_mutex_unlock(&tablas_primer_nivel_mutex);
	t_fila_tabla_paginacion_1erNivel* fila_tabla_paginacion_1erNivel = list_get(tabla_primer_nivel,recorredorPaginas->index->entrada_primer_nivel);
	pthread_mutex_lock(&tablas_segundo_nivel_mutex);
	t_list* tabla_segundo_nivel = list_get(tablas_segundo_nivel,fila_tabla_paginacion_1erNivel->nro_tabla);
	pthread_mutex_unlock(&tablas_segundo_nivel_mutex);
	t_fila_tabla_paginacion_2doNivel* fila_tabla_paginacion_2doNivel = list_get(tabla_segundo_nivel,recorredorPaginas->index->entrada_segundo_nivel); 

	fila_tabla_paginacion_2doNivel->u = 0;

	//printf("Asigno pagina %d a marco\n", marco_a_asignar);

	list_replace(tabla_segundo_nivel,recorredorPaginas->index->entrada_segundo_nivel,fila_tabla_paginacion_2doNivel);

}


void devolver_pcb(t_pcb* pcb,t_log* logger,int socket){
	t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = DEVOLVER_PROCESO;
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
	//printf("El Process Id del pcb recibido es: %d y tamaño %d\n",pcb->pid,pcb->size);
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
	configuraciones->ip_local = config_get_string_value(config , "IP_LOCAL");
	path_swap = config_get_string_value(config , "PATH_SWAP");
	mkdir(path_swap,S_IRWXU);
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

	log_info(logger,"Iniciando Memoria\n");
    int memoriaPrincipal = malloc(sizeof(configuraciones->tam_memoria));
    if (memoriaPrincipal == NULL) {
        log_error(logger, "Error al crear espacio contiguo de la memoria principal");
    }

    t_list* listaTablasPrimerNivel = list_create();
    uint32_t cantFrames = configuraciones->tam_memoria / configuraciones->tam_pagina;
    //printf("RAM FRAMES: %d \n", cantFrames);
	log_info(logger,"RAM FRAMES: %d \n",cantFrames);
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
