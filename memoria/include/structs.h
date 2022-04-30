#ifndef PROJECT_STRUCTS_H_
#define PROJECT_STRUCTS_H_

#include <stdio.h>

typedef struct configuraciones_memoria {
    char *puerto_escucha;
    char *tam_memoria;
    char *tam_pagina;
    char *paginas_por_tabla;
    char *retardo_memoria;
    char *algoritmo_reemplazo;
    char *marcos_por_proceso;
    char *retardo_swap;
    char *path_swap;
} t_configuraciones_memoria;


#endif