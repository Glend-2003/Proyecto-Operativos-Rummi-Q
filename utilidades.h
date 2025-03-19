#ifndef UTILIDADES_H
#define UTILIDADES_H

#include <stdbool.h>
#include <time.h>

// Funciones de utilidad para manejar tiempo
void esperar_milisegundos(int milisegundos);
time_t obtener_tiempo_actual();
char* obtener_timestamp_formateado(time_t tiempo);
char* obtener_timestamp_actual();

// Funciones para manejo de archivos
bool escribir_a_archivo(const char* nombre_archivo, const char* contenido);
char* leer_de_archivo(const char* nombre_archivo);
bool archivo_existe(const char* nombre_archivo);

// Funciones de generación aleatoria
int numero_aleatorio(int min, int max);
int seleccionar_aleatorio_de_arreglo(int arreglo[], int tamano);

// Funciones de depuración
void mostrar_debug(const char* formato, ...);
void registrar_evento(const char* tipo_evento, const char* descripcion);

// Funciones de manejo de strings
char* concatenar_strings(const char* str1, const char* str2);
char* int_a_string(int valor);
bool strings_iguales(const char* str1, const char* str2);

#endif