#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "utilidades.h"
#include "jugadores.h"

/* Constantes para colores en terminal */
#define COLOR_ROJO     "\x1b[31m"
#define COLOR_VERDE    "\x1b[32m"
#define COLOR_AMARILLO "\x1b[33m"
#define COLOR_AZUL     "\x1b[34m"
#define COLOR_MAGENTA  "\x1b[35m"
#define COLOR_CIAN     "\x1b[36m"
#define COLOR_RESET    "\x1b[0m"

/* Nombres de palos */
const char* nombrePalo(char palo) {
    switch(palo) {
        case 'C': return "Corazones";
        case 'D': return "Diamantes";
        case 'T': return "Tréboles";
        case 'E': return "Espadas";
        case 'J': return "Joker";
        default: return "Desconocido";
    }
}

/* Nombres de valores */
const char* nombreValor(int valor) {
    static char buffer[4];
    
    switch(valor) {
        case 1: return "As";
        case 11: return "J";
        case 12: return "Q";
        case 13: return "K";
        default: 
            sprintf(buffer, "%d", valor);
            return buffer;
    }
}

/* Imprimir carta en formato legible */
void imprimirCarta(Carta carta) {
    if (carta.esComodin) {
        printf("COMODÍN");
    } else {
        printf("%s de %s", nombreValor(carta.valor), nombrePalo(carta.palo));
    }
}

/* Imprimir mazo completo */
void imprimirMazo(Mazo *mazo) {
    int i;
    
    printf("Mazo (%d cartas):\n", mazo->numCartas);
    for (i = 0; i < mazo->numCartas; i++) {
        printf("  %d. ", i+1);
        imprimirCarta(mazo->cartas[i]);
        printf("\n");
    }
}

/* Obtener nombre de carta (para mostrar en log) */
char* obtenerNombreCarta(Carta carta, char *buffer) {
    if (carta.esComodin) {
        strcpy(buffer, "COMODÍN");
    } else {
        sprintf(buffer, "%s de %s", nombreValor(carta.valor), nombrePalo(carta.palo));
    }
    return buffer;
}

/* Calcular puntos de una carta */
int calcularPuntosCarta(Carta carta) {
    if (carta.esComodin) {
        return 20; /* Los comodines valen 20 puntos */
    } else if (carta.valor >= 11) { /* J, Q, K */
        return 10;
    } else if (carta.valor == 1) { /* As */
        return 15;
    } else {
        return carta.valor;
    }
}

/* Registrar evento en un archivo log */
void registrarEvento(const char *formato, ...) {
    FILE *archivo;
    time_t ahora;
    struct tm *t;
    char timestamp[25];
    va_list args;
    
    /* Abrir archivo en modo append */
    archivo = fopen("juego.log", "a");
    if (archivo == NULL) {
        return;
    }
    
    /* Obtener timestamp */
    ahora = time(NULL);
    t = localtime(&ahora);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    /* Escribir timestamp */
    fprintf(archivo, "[%s] ", timestamp);
    
    /* Escribir mensaje formateado */
    va_start(args, formato);
    vfprintf(archivo, formato, args);
    va_end(args);
    
    /* Añadir salto de línea */
    fprintf(archivo, "\n");
    
    /* Cerrar archivo */
    fclose(archivo);
}

/* Guardar estadísticas de juego en un archivo */
void guardarEstadisticasJuego(Jugador *jugadores, int numJugadores) {
    FILE *archivo;
    int i;
    
    archivo = fopen("estadisticas.txt", "w");
    if (archivo == NULL) {
        printf("Error: No se pudo crear el archivo de estadísticas\n");
        return;
    }
    
    fprintf(archivo, "=== ESTADÍSTICAS DEL JUEGO ===\n\n");
    
    /* Estadísticas por jugador */
    for (i = 0; i < numJugadores; i++) {
        fprintf(archivo, "Jugador %d:\n", jugadores[i].id);
        fprintf(archivo, "  Cartas restantes: %d\n", jugadores[i].mano.numCartas);
        fprintf(archivo, "  Puntos totales: %d\n", jugadores[i].puntosTotal);
        
        if (jugadores[i].bcp != NULL) {
            fprintf(archivo, "  Tiempo de ejecución: %d ms\n", jugadores[i].bcp->tiempoEjecucion);
            fprintf(archivo, "  Tiempo de espera: %d ms\n", jugadores[i].bcp->tiempoEspera);
            fprintf(archivo, "  Tiempo de bloqueo: %d ms\n", jugadores[i].bcp->tiempoBloqueo);
            fprintf(archivo, "  Veces apeado: %d\n", jugadores[i].bcp->vecesApeo);
            fprintf(archivo, "  Cartas comidas: %d\n", jugadores[i].bcp->cartasComidas);
            fprintf(archivo, "  Intentos fallidos: %d\n", jugadores[i].bcp->intentosFallidos);
            fprintf(archivo, "  Turnos perdidos: %d\n", jugadores[i].bcp->turnosPerdidos);
        }
        
        fprintf(archivo, "\n");
    }
    
    fclose(archivo);
    printf("Estadísticas del juego guardadas en 'estadisticas.txt'\n");
}

/* Funciones para colorear la salida en terminal */
void colorRojo(void) { printf(COLOR_ROJO); }
void colorVerde(void) { printf(COLOR_VERDE); }
void colorAmarillo(void) { printf(COLOR_AMARILLO); }
void colorAzul(void) { printf(COLOR_AZUL); }
void colorMagenta(void) { printf(COLOR_MAGENTA); }
void colorCian(void) { printf(COLOR_CIAN); }
void colorReset(void) { printf(COLOR_RESET); }