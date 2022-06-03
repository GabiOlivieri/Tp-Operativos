#include<cpu.h>

int main(int argc, char* argv[]) {
    t_log* logger = log_create("./cpu.log","CPU", false , LOG_LEVEL_TRACE);
    t_config* config = config_create("./cpu.conf");
    t_configuraciones* configuraciones = malloc(sizeof(t_configuraciones));


    char* config_properties[] = {
            "IP_KERNEL",
            "PUERTO_KERNEL",
        };

    leer_config(config,configuraciones);

    int servidor = iniciar_servidor(logger , "CPU Dispatch" , "127.0.0.1" , configuraciones->puerto_escucha_dispatch);
    int client_socket = esperar_cliente(logger , "CPU Dispatch" , servidor);


    t_list* lista;
        while (client_socket != -1 ) {
    		int cod_op = recibir_operacion(client_socket);
    		switch (cod_op) {
    		case MENSAJE:
    			recibir_mensaje(client_socket , logger);
    			break;
    		case PAQUETE:
    			log_info(logger, "Me llegaron los siguientes valores:\n");
    			break;

    		case INICIAR_PROCESO:
    			log_info(logger, "Me llego un INICIAR_PROCESO\n");
    			int instruccion=0;
    			int size;
       			char * buffer = recibir_buffer(&size, client_socket);
       			t_pcb* pcb = recibir_pcb(buffer);
       			while (!hay_interrupcion() && instruccion != EXIT && instruccion != IO){
    			instruccion = ejecutar_instruccion(pcb,configuraciones);}
       			devolver_pcb(pcb,logger);
    			break;
			
    		case -1:
    			log_error(logger, "el cliente se desconecto. Terminando servidor");
    			return EXIT_FAILURE;
    		default:
    			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
    			break;
    		}
    	}
        liberar_memoria(logger,config,configuraciones,servidor);

    return 0;
}

int ejecutar_instruccion(t_pcb* pcb,t_configuraciones* configuraciones){
		printf("Ejecuta instrucción\n");
		int x = list_get(pcb->lista_instrucciones,pcb->pc);
		if (x==NO_OP) {
			printf("Llegó un NO_OP %d\n",x);
			sleep(configuraciones->retardo_NOOP);
		}
		else if (x==IO){
			printf("Llegó un IO %d\n",x);
			pcb->pc++;
			int y = list_get(pcb->lista_instrucciones,pcb->pc);
			printf("El argumento recibido es %d\n",y);
			pcb->estado = BLOCKED;
		}
		else if (x==EXIT) {
			pcb->estado = TERMINATED;
			return x;}
		pcb->pc++;
		return x;

}

void devolver_pcb(t_pcb* pcb,t_log* logger){
	t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = DEVOLVER_PROCESO;
    int cantidad_enteros = list_size(pcb->lista_instrucciones);
    printf("El process enviado a kernel es: %d\n",pcb->pid);
    agregar_entero_a_paquete(paquete,pcb->pid);
    agregar_entero_a_paquete(paquete,pcb->pc);
    agregar_entero_a_paquete(paquete,pcb->estado);
    t_list_iterator* iterator = list_iterator_create(pcb->lista_instrucciones);
    while(list_iterator_has_next(iterator)){
        int ins = list_iterator_next(iterator);
        printf("El entero es: %d\n",ins);
        agregar_entero_a_paquete(paquete,ins);
    }
    list_iterator_destroy(iterator);
    int conexion = crear_conexion(logger , "CPU" , "127.0.0.1" , "8000" );
    enviar_paquete(paquete,conexion);
    eliminar_paquete(paquete);
    close(conexion);
}


int hay_interrupcion(){
	// Aća va toda la logica cuando llega una interrupción para que devuelva un true
	return 0;
}

t_pcb* recibir_pcb(char* buffer){
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->pid = leer_entero(buffer,0); //Variable global que se incrementa
	printf("El id de pcb recibido es: %d\n",pcb->pid);
	pcb->pc = leer_entero(buffer,1);
	printf("El pc recibido es: %d\n",pcb->pc);
	pcb->estado = RUNNING;

	pcb->lista_instrucciones = obtener_lista_instrucciones(buffer,pcb);

	return pcb;
}



t_list* obtener_lista_instrucciones(char* buffer, t_pcb* pcb){
	t_list* lista = list_create();
			int aux = pcb->pc + 3;
			int cantidad_enteros = 3 + leer_entero(buffer,2);
			printf("La cantidad de enteros es: %d\n",cantidad_enteros);
			for(; aux <= cantidad_enteros;aux++){
				int x = leer_entero(buffer,aux);
				void* id = malloc(sizeof(int));
				id = (void*)(&x);
				if(x==NO_OP){
					printf("Me llego un %d por lo que es un NO_OP\n",x);
					list_add(lista,0);
				}else if(x==IO){
					printf("Me llego un %d por lo que es un IO\n",x);
					list_add(lista,1);
					aux++;
					int y = leer_entero(buffer,aux);
					printf("Me llego un parametro: %d \n",y);
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
					printf("Me llego un %d por lo que es un EXIT\n",x);
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
	configuraciones->puerto_memoria = config_get_int_value(config , "PUERTO_MEMORIA");
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
