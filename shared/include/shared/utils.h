#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <stdint.h> 

/**
* @NAME: leer_archivo_completo
* @DESC: lee y devuelve en formato de string una archivo.
*/
char* leer_archivo_completo(char*);

/**
* @NAME: config_has_all_properties
* @DESC: se fija si el config contiene todas las propiedades pasadas como en array en el segundo paramtro.
*/
bool config_has_all_properties(t_config*, char**);

#endif