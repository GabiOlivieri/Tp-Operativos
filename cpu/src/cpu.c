#include<cpu.h>

int interrupcion=0;
int cantidad_de_entradas_por_tabla_de_paginas=0;
int tamano_de_pagina=0;
t_log* logger_debug;

pthread_mutex_t interrupcion_mutex;
pthread_mutex_t peticion_memoria_mutex;
pthread_mutex_t tlb_mutex;
pthread_mutex_t tlb_marco_mutex;
pthread_mutex_t tlb_ultima_referencia_mutex;
pthread_mutex_t tlb_entrada_primer_nivel_mutex;
pthread_mutex_t tlb_entrada_segundo_nivel_mutex;



int main(int argc, char* argv[]) {
    t_log* logger = log_create("./cpu.log","CPU", false , LOG_LEVEL_TRACE);
	logger_debug = log_create("./cpu.log","CPU", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./cpu.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));

    leer_config(config,configuraciones);
	handshake_memoria(logger,configuraciones);

	t_list* tlb = crear_TLB(configuraciones->entradas_TLB);



    int servidor = iniciar_servidor(logger , "CPU Dispatch" , "127.0.0.1" , configuraciones->puerto_escucha_dispatch);
    setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	manejar_interrupciones(logger,configuraciones);
	manejar_conexion_kernel(logger,configuraciones,servidor,tlb);
    liberar_memoria(logger,config,configuraciones,servidor);
    return 0;
}

void handshake_memoria(t_log* logger,t_configuraciones* configuraciones){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = HANDSHAKE;
	int socket = crear_conexion(logger , "Memoria" ,configuraciones->ip_memoria , configuraciones->puerto_memoria);
	enviar_paquete(paquete,socket);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(socket);
	int size;
	char * buffer = recibir_buffer(&size, socket);
	cantidad_de_entradas_por_tabla_de_paginas = leer_entero(buffer,0);
	tamano_de_pagina = leer_entero(buffer,1);
	close(socket);
}

t_list* crear_TLB(int cant_entradas){
	t_list* tlb = list_create();
	for(int i = 0; i < cant_entradas; i++){
		t_fila_tlb* fila_tlb = malloc(sizeof(t_fila_tlb));
		fila_tlb->instante_de_ultima_referencia = NULL;
		fila_tlb->instante_de_carga = NULL;
		t_pagina* pagina = malloc(sizeof(t_pagina));
		pagina->entrada_primer_nivel=-1;
		pagina->entrada_segundo_nivel=-1;
		fila_tlb->pagina = pagina;
		fila_tlb->marco = -1;
		list_add(tlb,fila_tlb);
	}
	return tlb;
}

void limpiar_TLB(t_list* tlb){
	int limite = list_size(tlb);
	for(int i = 0; i < limite ; i++){
		pthread_mutex_lock(&tlb_mutex);
		t_fila_tlb* fila_tlb = list_get(tlb,i);
		pthread_mutex_unlock(&tlb_mutex);
		pthread_mutex_lock(&tlb_ultima_referencia_mutex);
		fila_tlb->instante_de_ultima_referencia = NULL;
		pthread_mutex_unlock(&tlb_ultima_referencia_mutex);
		fila_tlb->instante_de_carga = NULL;
		pthread_mutex_lock(&tlb_entrada_primer_nivel_mutex);
		fila_tlb->pagina->entrada_primer_nivel=-1;
		pthread_mutex_unlock(&tlb_entrada_primer_nivel_mutex);
		fila_tlb->pagina->entrada_segundo_nivel=-1;
		pthread_mutex_lock(&tlb_marco_mutex);
		fila_tlb->marco = -1;
		pthread_mutex_unlock(&tlb_marco_mutex);
		pthread_mutex_lock(&tlb_mutex);
		list_replace(tlb,i,fila_tlb);
		pthread_mutex_unlock(&tlb_mutex);
	}
}

void cargar_en_TLB(t_list* tlb,int pagina1,int pagina2,int marco,t_configuraciones* configuraciones){
	t_fila_tlb* fila_tlb = malloc(sizeof(t_fila_tlb));
	fila_tlb->instante_de_carga = time(NULL);
	fila_tlb->instante_de_ultima_referencia = time(NULL);
	t_pagina* pagina = malloc(sizeof(t_pagina));
	pagina->entrada_primer_nivel=pagina1;
	pagina->entrada_segundo_nivel=pagina2;
	fila_tlb->pagina = pagina;
	fila_tlb->marco = marco;
	if(!puede_cachear(tlb)){
		reemplazar_entrada_tlb(tlb,fila_tlb,configuraciones);
	}
	else{
		t_list_iterator* iterator = list_iterator_create(tlb);
		while(list_iterator_has_next(iterator)){
			t_fila_tlb* fila_tlb_iterator = list_iterator_next(iterator);
			pthread_mutex_lock(&tlb_marco_mutex);
			if (fila_tlb_iterator->marco == -1){ // -1 significa que no está cargado en memoria
				pthread_mutex_unlock(&tlb_marco_mutex);
				printf("Cargo a la TLB \n");
				pthread_mutex_lock(&tlb_mutex);
				list_replace(tlb,iterator->index,fila_tlb);
				pthread_mutex_unlock(&tlb_mutex);
				list_iterator_destroy(iterator);
				return;
			}
			pthread_mutex_unlock(&tlb_marco_mutex);
		}
		list_iterator_destroy(iterator);
	}

}

int buscar_en_TLB(t_list* tlb, int entrada1, int entrada2){
	t_list_iterator* iterator = list_iterator_create(tlb);
		while(list_iterator_has_next(iterator)){
			pthread_mutex_lock(&tlb_mutex);
			t_fila_tlb* fila_tlb_iterator = list_iterator_next(iterator);
			pthread_mutex_unlock(&tlb_mutex);
			pthread_mutex_lock(&tlb_entrada_primer_nivel_mutex);
			if (fila_tlb_iterator->pagina->entrada_primer_nivel == entrada1 && fila_tlb_iterator->pagina->entrada_segundo_nivel == entrada2){
			pthread_mutex_unlock(&tlb_entrada_primer_nivel_mutex);
				printf("Encontre la dirección en la TLB \n");
				pthread_mutex_lock(&tlb_ultima_referencia_mutex);
				fila_tlb_iterator->instante_de_ultima_referencia = time(NULL);
				pthread_mutex_unlock(&tlb_ultima_referencia_mutex);
				pthread_mutex_lock(&tlb_mutex);
				list_replace(tlb,iterator->index,fila_tlb_iterator);
				pthread_mutex_unlock(&tlb_mutex);
				list_iterator_destroy(iterator);
				return fila_tlb_iterator->marco;
			}
			pthread_mutex_unlock(&tlb_entrada_primer_nivel_mutex);
		}
	list_iterator_destroy(iterator);	
	return -1;
}

void reemplazar_entrada_tlb(t_list* tlb,t_fila_tlb* fila_tlb_a_reemplazar,t_configuraciones* configuraciones){
	int index_elegido = 0;
	t_fila_tlb* elegido =  list_get(tlb,0);
	if (strcmp(configuraciones->reemplazo_TLB,"LRU") == 0){
		t_list_iterator* iterator = list_iterator_create(tlb);
    	while(list_iterator_has_next(iterator)){
			t_fila_tlb* fila_tlb = list_iterator_next(iterator);
			if (difftime(fila_tlb->instante_de_ultima_referencia, elegido->instante_de_ultima_referencia) < 0 ){
				index_elegido = iterator->index;
				elegido = fila_tlb;
			}
		}
		printf("Reemplazo la pagina %d con LRU \n",index_elegido);
		list_iterator_destroy(iterator);
	}
	else
	{
		t_list_iterator* iterator = list_iterator_create(tlb);
    	while(list_iterator_has_next(iterator)){
			t_fila_tlb* fila_tlb = list_iterator_next(iterator);
			if (difftime(fila_tlb->instante_de_carga,elegido->instante_de_carga) < 0){
				index_elegido = iterator->index;
				elegido = fila_tlb;
			}
		}
		printf("Reemplazo la pagina %d con FIFO \n",index_elegido);
		list_iterator_destroy(iterator);
	}
	
	pthread_mutex_lock(&tlb_mutex);
	list_replace(tlb,index_elegido,fila_tlb_a_reemplazar);
	pthread_mutex_unlock(&tlb_mutex);
}


bool puede_cachear(t_list* tlb){
	int limite = list_size(tlb);
	t_list_iterator* iterator = list_iterator_create(tlb);
    while(list_iterator_has_next(iterator)){
		pthread_mutex_lock(&tlb_mutex);
        t_fila_tlb* fila_tlb = list_iterator_next(iterator);
		pthread_mutex_unlock(&tlb_mutex);
		pthread_mutex_lock(&tlb_marco_mutex);
        if (fila_tlb->marco == -1){
			pthread_mutex_unlock(&tlb_marco_mutex);
			list_iterator_destroy(iterator);
			return true;
		}
		pthread_mutex_unlock(&tlb_marco_mutex);
    }
    list_iterator_destroy(iterator);
	return false;
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

void manejar_conexion_kernel(t_log* logger, t_configuraciones* configuraciones, int socket,t_list* tlb){
	while(1){
        int client_socket = esperar_cliente(logger , "CPU Dispatch" , socket);
		t_hilo_struct* hilo = malloc(sizeof(t_hilo_struct));
		hilo->logger = logger;
		hilo->socket = client_socket;
		hilo->configuraciones = configuraciones;
		hilo->tlb = tlb;
        pthread_t hilo_servidor;
        pthread_create (&hilo_servidor, NULL , (void*) atender_cliente,(void*) hilo);
        pthread_detach(hilo_servidor);

    }	
}

int atender_interrupcion(void* arg){
	struct hilo_struct *p;
	p = (struct hilo_struct*) arg;
	while (1){
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
    //   	printf("Me llego una interrupcion\n");
		pthread_mutex_lock (&interrupcion_mutex);
		interrupcion=1;
		pthread_mutex_unlock (&interrupcion_mutex);
}

int atender_cliente(void* arg){
	struct hilo_struct *p;
	p = (struct hilo_struct*) arg;
	while (1){
		int cod_op = recibir_operacion(p->socket);
		switch (cod_op) {

    		case INICIAR_PROCESO:
    			log_info(p->logger, "Me llego un INICIAR_PROCESO\n");
    			int instruccion=0;
    			int size;
       			char * buffer = recibir_buffer(&size, p->socket);
       			t_pcb* pcb = recibir_pcb(buffer);
				clock_t begin = clock();
				pthread_mutex_lock (&interrupcion_mutex);
       			while (!interrupcion && instruccion != EXIT && instruccion != IO){
				pthread_mutex_unlock (&interrupcion_mutex);
    			instruccion = ciclo_de_instruccion(p->logger,pcb,p->configuraciones,p->tlb);
				}
				pthread_mutex_unlock (&interrupcion_mutex);
				pthread_mutex_lock (&interrupcion_mutex);
				if(instruccion == EXIT) notificar_fin(p->logger,pcb,p->configuraciones,p->tlb);
				pthread_mutex_unlock (&interrupcion_mutex);
				pthread_mutex_lock (&interrupcion_mutex);
				interrupcion=0;
				pthread_mutex_unlock (&interrupcion_mutex);
				clock_t end = clock();
				double rafaga = (double)(end - begin) / CLOCKS_PER_SEC;
				printf("Hizo %f rafagas\n", rafaga * 1000000 );
				pcb->rafaga_anterior= rafaga * 1000000;
				limpiar_TLB(p->tlb);
       			devolver_pcb(pcb,p->logger,p->socket);
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

void notificar_fin(t_log* logger,t_pcb* pcb,t_configuraciones* configuraciones,t_list* tlb){
	int socket = crear_conexion(logger , "Memoria" ,configuraciones->ip_memoria , configuraciones->puerto_memoria);
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = FINALIZAR_PROCESO;
	printf("Notifico fin de proceso \n" );
	log_info(logger, "Fin de proceso\n");
	agregar_entero_a_paquete(paquete,pcb->pid);
	enviar_paquete(paquete,socket);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(socket);
	int size;
	char * buffer = recibir_buffer(&size, socket);
	int pid = leer_entero(buffer,0);
}

int ciclo_de_instruccion(t_log* logger,t_pcb* pcb,t_configuraciones* configuraciones,t_list* tlb){
		t_direccion direccion;
		// fetch
		int x = list_get(pcb->lista_instrucciones,pcb->pc); 
		//decode
		if (x==NO_OP) {
			usleep(configuraciones->retardo_NOOP * 1000);
		}
		else if (x==IO){
			pcb->pc++;
			int y = list_get(pcb->lista_instrucciones,pcb->pc);
			pcb->tiempo_bloqueo= y;
			pcb->estado = BLOCKED;
		}
		else if (x==READ){
			pcb->pc++;
			int direccion_logica = list_get(pcb->lista_instrucciones,pcb->pc);
			direccion = mmu_traduccion(direccion_logica);
			int marco = buscar_en_TLB(tlb,direccion.entrada_primer_nivel,direccion.entrada_segundo_nivel);
			if (marco == -1){
				int nro_segunda_tabla = primer_acceso_a_memoria(pcb,logger,configuraciones,direccion.entrada_primer_nivel);
				marco = segundo_acesso_a_memoria(pcb,logger,configuraciones,nro_segunda_tabla,direccion.entrada_segundo_nivel,x);
				cargar_en_TLB(tlb,direccion.entrada_primer_nivel,direccion.entrada_segundo_nivel,marco,configuraciones);
			}
			tercer_acesso_a_memoria(pcb,logger,configuraciones,marco,direccion.desplazamiento,x);
		}
		else if (x==WRITE){
			pcb->pc++;
			int direccion_logica = list_get(pcb->lista_instrucciones,pcb->pc);
			direccion = mmu_traduccion(direccion_logica);
			int marco = buscar_en_TLB(tlb,direccion.entrada_primer_nivel,direccion.entrada_segundo_nivel);
			if(marco == -1){
				int nro_segunda_tabla = primer_acceso_a_memoria(pcb,logger,configuraciones,direccion.entrada_primer_nivel);
				marco = segundo_acesso_a_memoria(pcb,logger,configuraciones,nro_segunda_tabla,direccion.entrada_segundo_nivel,x);
				cargar_en_TLB(tlb,direccion.entrada_primer_nivel,direccion.entrada_segundo_nivel,marco,configuraciones);
			}
			pcb->pc++;
			int atributo = list_get(pcb->lista_instrucciones,pcb->pc);
			tercer_acesso_a_memoria_a_escribir(pcb,logger,configuraciones,marco,direccion.desplazamiento,x,atributo);
		}
		else if (x==COPY){
			pcb->pc++;
			int direccion_logica_destino = list_get(pcb->lista_instrucciones,pcb->pc);
			pcb->pc++;
			int direccion_logica_origen = list_get(pcb->lista_instrucciones,pcb->pc);
			int valor = fetch_operands(direccion_logica_origen,pcb,logger,configuraciones,tlb);
			direccion = mmu_traduccion(direccion_logica_destino);
			int marco = buscar_en_TLB(tlb,direccion.entrada_primer_nivel, direccion.entrada_segundo_nivel);
			if(marco == -1){
				int nro_segunda_tabla = primer_acceso_a_memoria(pcb,logger,configuraciones,direccion.entrada_primer_nivel);
				marco = segundo_acesso_a_memoria(pcb,logger,configuraciones,nro_segunda_tabla,direccion.entrada_segundo_nivel,WRITE);
				cargar_en_TLB(tlb,direccion.entrada_primer_nivel,direccion.entrada_segundo_nivel,marco,configuraciones);
			}
			tercer_acesso_a_memoria_a_escribir(pcb,logger,configuraciones,marco,direccion.desplazamiento,WRITE,valor);
		}
		else if (x==EXIT) {
			pcb->estado = TERMINATED;
			return x;}
		pcb->pc++;
		return x;
}

int fetch_operands(int direccion_logica_origen,t_pcb* pcb,t_log* logger,t_configuraciones* configuraciones,t_list* tlb){
	t_direccion direccion_origen = mmu_traduccion(direccion_logica_origen);
	int marco = buscar_en_TLB(tlb,direccion_origen.entrada_primer_nivel, direccion_origen.entrada_segundo_nivel);
	if(marco == -1){
		int nro_segunda_tabla = primer_acceso_a_memoria(pcb,logger,configuraciones,direccion_origen.entrada_primer_nivel);
		marco = segundo_acesso_a_memoria(pcb,logger,configuraciones,nro_segunda_tabla,direccion_origen.entrada_segundo_nivel,READ);
		cargar_en_TLB(tlb,direccion_origen.entrada_primer_nivel,direccion_origen.entrada_segundo_nivel,marco,configuraciones);
	}
	int valor = tercer_acesso_a_memoria(pcb,logger,configuraciones,marco,direccion_origen.desplazamiento,READ);
	return valor;
}

t_direccion mmu_traduccion(int dir_logica){

	t_direccion direccion;
	int numero_pagina = floor(dir_logica / tamano_de_pagina);
	direccion.entrada_primer_nivel = floor(numero_pagina / cantidad_de_entradas_por_tabla_de_paginas);
	direccion.entrada_segundo_nivel = numero_pagina % cantidad_de_entradas_por_tabla_de_paginas;
	direccion.desplazamiento = dir_logica - numero_pagina * tamano_de_pagina;

	return direccion;
}



int primer_acceso_a_memoria(t_pcb* pcb,t_log* logger,t_configuraciones* configuraciones,int entrada_tabla_primer_nivel){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = PRIMER_ACCESO_A_MEMORIA;
	int socket = crear_conexion(logger , "Memoria" ,configuraciones->ip_memoria , configuraciones->puerto_memoria);
	printf("Envio %d y %d\n",pcb->tabla_paginas, entrada_tabla_primer_nivel);
	agregar_entero_a_paquete(paquete,pcb->pid);
	agregar_entero_a_paquete(paquete,pcb->tabla_paginas);
	agregar_entero_a_paquete(paquete,entrada_tabla_primer_nivel);
	enviar_paquete(paquete,socket);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(socket);
	int size;
	char * buffer = recibir_buffer(&size, socket);
	int pid = leer_entero(buffer,0);
	int direccion_fisica_entrada_segunda_tabla = leer_entero(buffer,1);
	printf("La conexión fue exitosa y el pid %d leyó y trajo la dirección a la entrada de la segunda tabla: %d\n",pid,direccion_fisica_entrada_segunda_tabla );
	return direccion_fisica_entrada_segunda_tabla;
}

int segundo_acesso_a_memoria(t_pcb* pcb,t_log* logger,t_configuraciones* configuraciones, int tabla_segundo_nivel,int entrada_segunda_tabla,op_ins codigo_operacion,t_list* tlb){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = SEGUNDO_ACCESSO_A_MEMORIA;
	int socket = crear_conexion(logger , "Memoria" ,configuraciones->ip_memoria , configuraciones->puerto_memoria);
	printf("Envio %d, %d y %d \n",tabla_segundo_nivel, entrada_segunda_tabla,codigo_operacion );
	agregar_entero_a_paquete(paquete,pcb->pid);
	agregar_entero_a_paquete(paquete,tabla_segundo_nivel);
	agregar_entero_a_paquete(paquete,entrada_segunda_tabla);
	agregar_entero_a_paquete(paquete,codigo_operacion);
	enviar_paquete(paquete,socket);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(socket);
	int size;
	char * buffer = recibir_buffer(&size, socket);
	int pid = leer_entero(buffer,0);
	int marco = leer_entero(buffer,1);
	printf("La conexión fue exitosa y el pid %d trajo la dirección: %d\n",pid,marco);
	return marco;
}

int tercer_acesso_a_memoria(t_pcb* pcb,t_log* logger,t_configuraciones* configuraciones,int marco, int desplazamiento,op_ins codigo_operacion){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = TERCER_ACCESSO_A_MEMORIA;
	int socket = crear_conexion(logger , "Memoria" ,configuraciones->ip_memoria , configuraciones->puerto_memoria);
	printf("Envio %d, %d y %d \n",marco, desplazamiento,codigo_operacion );
	agregar_entero_a_paquete(paquete,pcb->pid);
	agregar_entero_a_paquete(paquete,marco);
	agregar_entero_a_paquete(paquete,desplazamiento);
	agregar_entero_a_paquete(paquete,codigo_operacion);
	enviar_paquete(paquete,socket);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(socket);
	int size;
	char * buffer = recibir_buffer(&size, socket);
	int pid = leer_entero(buffer,0);
	int valor = leer_entero(buffer,1);
	printf("La conexión fue exitosa y el pid %d leyó el valor: %d\n",pid,valor);
	return valor;
}

void tercer_acesso_a_memoria_a_escribir(t_pcb* pcb,t_log* logger,t_configuraciones* configuraciones,int marco, int desplazamiento,op_ins codigo_operacion,int valor){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = TERCER_ACCESSO_A_MEMORIA;
	int socket = crear_conexion(logger , "Memoria" ,configuraciones->ip_memoria , configuraciones->puerto_memoria);
	printf("Envio %d, %d, %d y %d \n",marco, desplazamiento,codigo_operacion,valor );
	agregar_entero_a_paquete(paquete,pcb->pid);
	agregar_entero_a_paquete(paquete,marco);
	agregar_entero_a_paquete(paquete,desplazamiento);
	agregar_entero_a_paquete(paquete,codigo_operacion);
	agregar_entero_a_paquete(paquete,valor);
	enviar_paquete(paquete,socket);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(socket);
	int size;
	char * buffer = recibir_buffer(&size, socket);
	int pid = leer_entero(buffer,0);
	valor = leer_entero(buffer,1);
	printf("La conexión por WRITE fue exitosa y el pid %d escribió el valor: %d\n",pid,valor);
}

void devolver_pcb(t_pcb* pcb,t_log* logger,int socket){
	t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = DEVOLVER_PROCESO;
    int cantidad_enteros = list_size(pcb->lista_instrucciones);
//    printf("Devuelvo proceso a Kernel\n");
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
	pcb->tabla_paginas = leer_entero(buffer,3);
//	printf("El Process Id del pcb recibido es: %d y su Program Counter es: %d\n",pcb->pid,pcb->pc);
	pcb->estado = RUNNING;
	pcb->tiempo_bloqueo = 0;
	pcb->lista_instrucciones = obtener_lista_instrucciones(buffer,pcb);
	return pcb;
}



t_list* obtener_lista_instrucciones(char* buffer, t_pcb* pcb){
	t_list* lista = list_create();
			int aux = 4;
			int cantidad_enteros = 4 + leer_entero(buffer,2);
			for(; aux <= cantidad_enteros;aux++){
				int x = leer_entero(buffer,aux);
				void* id = malloc(sizeof(int));
				id = (void*)(&x);
				if(x==NO_OP){
					list_add(lista,0);
				}else if(x==IO){
					list_add(lista,1);
					aux++;
					int y = leer_entero(buffer,aux);
					list_add(lista,y);
				}else if(x==READ){
					list_add(lista,2);
					aux++;
					int y = leer_entero(buffer,aux);
					printf("READ %d \n",y);
					list_add(lista,y);
				}else if(x==WRITE){
					list_add(lista,3);
					aux++;
					int y = leer_entero(buffer,aux);
					list_add(lista,y);
					aux++;
					int z = leer_entero(buffer,aux);
					printf("WRITE %d %d \n",y,z);
					list_add(lista,z);
				}else if(x==COPY){
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