#include<cpu.h>

int interrupcion=0;
int cantidad_de_entradas_por_tabla_de_paginas=0;
int tamano_de_pagina=0;

pthread_mutex_t interrupcion_mutex;
pthread_mutex_t peticion_memoria_mutex;



int main(int argc, char* argv[]) {
    t_log* logger = log_create("./cpu.log","CPU", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./cpu.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));

    leer_config(config,configuraciones);
	handshake_memoria(logger,configuraciones);

	t_list* tlb = crear_TLB(configuraciones->entradas_TLB);


    int servidor = iniciar_servidor(logger , "CPU Dispatch" , "127.0.0.1" , configuraciones->puerto_escucha_dispatch);
    manejar_interrupciones(logger,configuraciones);
	manejar_conexion_kernel(logger,configuraciones,servidor);
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
		fila_tlb->pagina = NULL;
		fila_tlb->marco = NULL;
		list_add(tlb,fila_tlb);
	}
	return tlb;
}

void limpiar_TLB(t_list* tlb){
	int limite = list_size(tlb);
	for(int i = 0; i < limite ; i++){
		t_fila_tlb* fila_tlb = list_get(tlb,i);
		fila_tlb->pagina = NULL;
		fila_tlb->marco = NULL;
		list_replace(tlb,i,fila_tlb);
	}
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
				int contador=0;
       			char * buffer = recibir_buffer(&size, p->socket);
       			t_pcb* pcb = recibir_pcb(buffer);
				clock_t begin = clock();
       			while (!interrupcion && instruccion != EXIT && instruccion != IO){
    			instruccion = ejecutar_instruccion(p->logger,pcb,p->configuraciones);
				if (instruccion == NO_OP) contador++;
				}
//				if(instruccion == EXIT) notificar_fin(p->logger,pcb,p->configuraciones);
				pthread_mutex_lock (&interrupcion_mutex);
				interrupcion=0;
				pthread_mutex_unlock (&interrupcion_mutex);
				clock_t end = clock();
				double rafaga = (double)(end - begin) / CLOCKS_PER_SEC;
				printf("Hizo %f rafagas\n", rafaga * 1000000 );
				pcb->rafaga_anterior= rafaga * 1000000;
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

int notificar_fin(t_log* logger,t_pcb* pcb,t_configuraciones* configuraciones){
	int socket = crear_conexion(logger , "Memoria" ,configuraciones->ip_memoria , configuraciones->puerto_memoria);
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = FINALIZAR_PROCESO;
	printf("Notifico fin de proceso \n" );
	agregar_entero_a_paquete(paquete,pcb->pid);
	enviar_paquete(paquete,socket);
	eliminar_paquete(paquete);
	int codigoOperacion = recibir_operacion(socket);
	int size;
	char * buffer = recibir_buffer(&size, socket);
	int pid = leer_entero(buffer,0);
}

int ejecutar_instruccion(t_log* logger,t_pcb* pcb,t_configuraciones* configuraciones){
		t_direccion direccion;
		int x = list_get(pcb->lista_instrucciones,pcb->pc);
		if (x==NO_OP) {
			printf("Ejecuto un NO_OP\n");
			usleep(configuraciones->retardo_NOOP * 1000);
		}
		else if (x==IO){
			pcb->pc++;
			int y = list_get(pcb->lista_instrucciones,pcb->pc);
			printf("Ejecuto un IO de argumento %d\n",y);
			pcb->tiempo_bloqueo= y;
			pcb->estado = BLOCKED;
		}
		else if (x==READ){
			pcb->pc++;
			int direccion_logica = list_get(pcb->lista_instrucciones,pcb->pc);
			direccion = mmu_traduccion(direccion_logica);
			int nro_segunda_tabla = primer_acceso_a_memoria(pcb,logger,configuraciones,direccion.entrada_primer_nivel);
			int marco = segundo_acesso_a_memoria(pcb,logger,configuraciones,nro_segunda_tabla,direccion.entrada_segundo_nivel,x);
			tercer_acesso_a_memoria(pcb,logger,configuraciones,marco,direccion.desplazamiento,x);
		}
		else if (x==WRITE){
			pcb->pc++;
			int direccion_logica = list_get(pcb->lista_instrucciones,pcb->pc);
			direccion = mmu_traduccion(direccion_logica);
			int nro_segunda_tabla = primer_acceso_a_memoria(pcb,logger,configuraciones,direccion.entrada_primer_nivel);
			int marco = segundo_acesso_a_memoria(pcb,logger,configuraciones,nro_segunda_tabla,direccion.entrada_segundo_nivel,x);
			pcb->pc++;
			int atributo = list_get(pcb->lista_instrucciones,pcb->pc);
			tercer_acesso_a_memoria_a_escribir(pcb,logger,configuraciones,marco,direccion.desplazamiento,x,atributo);
		}
		else if (x==EXIT) {
			printf("Ejecuto un EXIT\n");
			pcb->estado = TERMINATED;
			return x;}
		pcb->pc++;
		return x;
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

int segundo_acesso_a_memoria(t_pcb* pcb,t_log* logger,t_configuraciones* configuraciones, int tabla_segundo_nivel,int entrada_segunda_tabla,op_ins codigo_operacion){
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

void tercer_acesso_a_memoria(t_pcb* pcb,t_log* logger,t_configuraciones* configuraciones,int marco, int desplazamiento,op_ins codigo_operacion){
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
	pcb->tabla_paginas = leer_entero(buffer,3);
	printf("El Process Id del pcb recibido es: %d y su Program Counter es: %d\n",pcb->pid,pcb->pc);
	pcb->estado = RUNNING;
	pcb->tiempo_bloqueo = 0;
	pcb->lista_instrucciones = obtener_lista_instrucciones(buffer,pcb);
	return pcb;
}



t_list* obtener_lista_instrucciones(char* buffer, t_pcb* pcb){
	t_list* lista = list_create();
			int aux = 4;
			int cantidad_enteros = 4 + leer_entero(buffer,2);
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