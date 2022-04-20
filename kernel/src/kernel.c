#include<kernel.h>

int main(int argc, char* argv[]) {
	///home/utnso/tp-2022-1c-Champagne-SO/kernel/kernel.log Harcodeada la ruta
    t_log* logger = log_create("./kernel.log","KERNEL", false , LOG_LEVEL_TRACE);
    int servidor = iniciar_servidor(logger , "un nombre" , "127.0.0.1" , "8000");
    int client_socket = esperar_cliente(logger , "un nombre" , servidor);

	t_list* lista;
    while (client_socket != -1) {
		int cod_op = recibir_operacion(client_socket);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(client_socket , logger);
			break;
		case PAQUETE:
			log_info(logger, "Me llegaron los siguientes valores:\n");
			// list_iterate(lista, (void*) iterator);
			break;
		
		case INICIAR_PROCESO:
			log_info(logger, "Me llego un INICIAR_PROCESO\n");
			int size;
   			char * buffer = recibir_buffer(&size, client_socket);
			int x = leer_entero(buffer,0);
			log_info(logger, "Me llego un entero %d\n",x);
			// TO DO -> crearProceso
			break;
		
		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
    return 0;
}
