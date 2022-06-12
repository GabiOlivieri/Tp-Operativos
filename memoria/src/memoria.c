#include<memoria.h>


int main(int argc, char* argv[]) {
    t_log* logger = log_create("./memoria.log","MEMORIA", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./memoria.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));

    leer_config(config,configuraciones);

    int servidor = iniciar_servidor(logger , "Memoria" , "127.0.0.1" , configuraciones->puerto_escucha);
    manejar_conexion(logger,configuraciones,servidor);
    liberar_memoria(logger,config,configuraciones,servidor);
    return 0;
}

void manejar_conexion(t_log* logger, t_configuraciones* configuraciones, int socket){
   	while(1){
        int client_socket = esperar_cliente(logger , "Memoria" , socket);
		t_hilo_struct* hilo = malloc(sizeof(t_hilo_struct));
		hilo->logger = logger;
		hilo->socket = client_socket;
		hilo->configuraciones = configuraciones;
        pthread_t hilo_servidor;
        pthread_create (&hilo_servidor, NULL , (void*) atender_cliente,(void*) hilo);
        pthread_detach(hilo_servidor);  
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
    		case PAQUETE:
    			log_info(p->logger, "Me llegaron los siguientes valores:\n");
    			break;

    		case INICIAR_PROCESO:
    			log_info(p->logger, "Me llego un INICIAR_PROCESO\n");
                int size;
       			char * buffer = recibir_buffer(&size, p->socket);
       			t_pcb* pcb = recibir_pcb(buffer);
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


void devolver_pcb(t_pcb* pcb,t_log* logger,int socket){
	t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = DEVOLVER_PROCESO;
    int cantidad_enteros = list_size(pcb->lista_instrucciones);
    printf("Devuelvo proceso a Kernel\n");
    agregar_entero_a_paquete(paquete,pcb->pc);
    agregar_entero_a_paquete(paquete,pcb->estado);
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

t_pcb* recibir_pcb(char* buffer){
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->pid = leer_entero(buffer,0); //Variable global que se incrementa
	pcb->pc = leer_entero(buffer,1);
	printf("El Process Id del pcb recibido es: %d y se va a quedar: %d \n",pcb->pid,pcb->pc);
	pcb->estado = RUNNING;
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
					list_add(lista,id);
					aux++;
					int y = leer_entero(buffer,aux);
					void* parametro = malloc(sizeof(int));
					parametro = (void*)(&y);
					printf("Me llego un parametro: %d \n",y);
					list_add(lista,parametro);
				}else if(x==WRITE){
					printf("Me llego un %d por lo que es un WRITE\n",x);
					list_add(lista,id);
					aux++;
					int y = leer_entero(buffer,aux);
					void* parametro = malloc(sizeof(int));
					parametro = (void*)(&y);
					printf("Me llego un parametro: %d \n",y);
					list_add(lista,parametro);
					aux++;
					int z = leer_entero(buffer,aux);
					void* parametro1 = malloc(sizeof(int));
					parametro1 = (void*)(&z);
					printf("Me llego un parametro: %d \n",z);
					list_add(lista,parametro1);
				}else if(x==COPY){
					printf("Me llego un %d por lo que es un COPY\n",x);
					list_add(lista,id);
					aux++;
					int y = leer_entero(buffer,aux);
					void* parametro = malloc(sizeof(int));
					parametro = (void*)(&y);
					printf("Me llego un parametro: %d \n",y);
					list_add(lista,parametro);
					aux++;
					int z = leer_entero(buffer,aux);
					void* parametro1 = malloc(sizeof(int));
					parametro1 = (void*)(&z);
					printf("Me llego un parametro: %d \n",z);
					list_add(lista,parametro1);
				}else if(x==EXIT){
					printf("EXIT\n",x);
					list_add(lista,5);
				}
			}
			return lista;
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
	configuraciones->path_swap = config_get_string_value(config , "PATH_SWAP");
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
