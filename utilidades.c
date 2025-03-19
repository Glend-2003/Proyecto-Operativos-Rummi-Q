#include "utilidades.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

// Variable global para controlar la depuración
static bool modo_debug = false;

// Esperar un número específico de milisegundos
void esperar_milisegundos(int milisegundos) {
    struct timespec ts;
    ts.tv_sec = milisegundos / 1000;
    ts.tv_nsec = (milisegundos % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// Obtener el tiempo actual
time_t obtener_tiempo_actual() {
    return time(NULL);
}

// Obtener un timestamp formateado
char* obtener_timestamp_formateado(time_t tiempo) {
    static char buffer[30];
    struct tm *tm_info = localtime(&tiempo);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

// Obtener el timestamp actual formateado
char* obtener_timestamp_actual() {
    return obtener_timestamp_formateado(obtener_tiempo_actual());
}

// Escribir contenido a un archivo
bool escribir_a_archivo(const char* nombre_archivo, const char* contenido) {
    FILE *file = fopen(nombre_archivo, "w");
    if (!file) {
        mostrar_debug("Error: No se pudo abrir el archivo %s para escritura\n", nombre_archivo);
        return false;
    }
    
    fprintf(file, "%s", contenido);
    fclose(file);
    return true;
}

// Leer contenido de un archivo
char* leer_de_archivo(const char* nombre_archivo) {
    FILE *file = fopen(nombre_archivo, "r");
    if (!file) {
        mostrar_debug("Error: No se pudo abrir el archivo %s para lectura\n", nombre_archivo);
        return NULL;
    }
    
    // Determinar el tamaño del archivo
    fseek(file, 0, SEEK_END);
    long tamano = ftell(file);
    rewind(file);
    
    // Asignar memoria para el contenido
    char *contenido = (char*)malloc(tamano + 1);
    if (!contenido) {
        mostrar_debug("Error: No se pudo asignar memoria para el contenido del archivo\n");
        fclose(file);
        return NULL;
    }
    
    // Leer el contenido
    size_t bytes_leidos = fread(contenido, 1, tamano, file);
    if (bytes_leidos != tamano) {
        mostrar_debug("Error: No se pudo leer el archivo completo\n");
        free(contenido);
        fclose(file);
        return NULL;
    }
    
    // Agregar terminador nulo
    contenido[tamano] = '\0';
    
    fclose(file);
    return contenido;
}

// Verificar si un archivo existe
bool archivo_existe(const char* nombre_archivo) {
    struct stat buffer;
    return (stat(nombre_archivo, &buffer) == 0);
}

// Generar un número aleatorio en un rango
int numero_aleatorio(int min, int max) {
    return min + (rand() % (max - min + 1));
}

// Seleccionar un elemento aleatorio de un arreglo
int seleccionar_aleatorio_de_arreglo(int arreglo[], int tamano) {
    int indice = rand() % tamano;
    return arreglo[indice];
}

// Activar o desactivar el modo de depuración
void establecer_modo_debug(bool activar) {
    modo_debug = activar;
    mostrar_debug("Modo debug %s\n", activar ? "activado" : "desactivado");
}

// Mostrar mensaje de depuración
void mostrar_debug(const char* formato, ...) {
    if (!modo_debug) return;
    
    va_list args;
    va_start(args, formato);
    
    printf("[DEBUG] ");
    vprintf(formato, args);
    
    va_end(args);
}

// Registrar evento en archivo de log
void registrar_evento(const char* tipo_evento, const char* descripcion) {
    FILE *file = fopen("eventos.log", "a");
    if (!file) {
        mostrar_debug("Error: No se pudo abrir el archivo de log\n");
        return;
    }
    
    fprintf(file, "[%s] %s: %s\n", obtener_timestamp_actual(), tipo_evento, descripcion);
    fclose(file);
}

// Concatenar dos strings
char* concatenar_strings(const char* str1, const char* str2) {
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    
    char *resultado = (char*)malloc(len1 + len2 + 1);
    if (!resultado) {
        mostrar_debug("Error: No se pudo asignar memoria para concatenar strings\n");
        return NULL;
    }
    
    strcpy(resultado, str1);
    strcat(resultado, str2);
    
    return resultado;
}

// Convertir un entero a string
char* int_a_string(int valor) {
    // Calcular el tamaño necesario para el string
    int longitud = snprintf(NULL, 0, "%d", valor);
    
    char *str = (char*)malloc(longitud + 1);
    if (!str) {
        mostrar_debug("Error: No se pudo asignar memoria para convertir int a string\n");
        return NULL;
    }
    
    sprintf(str, "%d", valor);
    return str;
}

// Comparar si dos strings son iguales
bool strings_iguales(const char* str1, const char* str2) {
    return strcmp(str1, str2) == 0;
}

// Función para dividir un string en tokens
char** dividir_string(const char* str, const char* delimitador, int* num_tokens) {
    // Copiar el string original ya que strtok modifica el string
    char *copia = strdup(str);
    if (!copia) {
        mostrar_debug("Error: No se pudo asignar memoria para copiar string\n");
        *num_tokens = 0;
        return NULL;
    }
    
    // Contar el número de tokens
    *num_tokens = 0;
    char *temp = strdup(str);
    char *token = strtok(temp, delimitador);
    while (token) {
        (*num_tokens)++;
        token = strtok(NULL, delimitador);
    }
    free(temp);
    
    // Crear arreglo de tokens
    char **tokens = (char**)malloc((*num_tokens) * sizeof(char*));
    if (!tokens) {
        mostrar_debug("Error: No se pudo asignar memoria para tokens\n");
        free(copia);
        *num_tokens = 0;
        return NULL;
    }
    
    // Extraer los tokens
    int i = 0;
    token = strtok(copia, delimitador);
    while (token && i < *num_tokens) {
        tokens[i] = strdup(token);
        i++;
        token = strtok(NULL, delimitador);
    }
    
    free(copia);
    return tokens;
}

// Liberar memoria de los tokens
void liberar_tokens(char** tokens, int num_tokens) {
    for (int i = 0; i < num_tokens; i++) {
        free(tokens[i]);
    }
    free(tokens);
}