#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "utilidades.h"
#include "jugadores.h"
#include "juego.h"

/* Constantes para colores en terminal */
#define COLOR_ROJO     "\x1b[31m"
#define COLOR_VERDE    "\x1b[32m"
#define COLOR_AMARILLO "\x1b[33m"
#define COLOR_AZUL     "\x1b[34m"
#define COLOR_MAGENTA  "\x1b[35m"
#define COLOR_CIAN     "\x1b[36m"
#define COLOR_RESET    "\x1b[0m"

// Declarar como global (ya no static) y exportarla
int rondaActual = 0;
static const char* ARCHIVO_HISTORIAL = "historial_juego.txt";

// Función para registrar el historial de una ronda completa
// Función para registrar el historial de una ronda completa
void registrarHistorial(int numRonda, Jugador *jugadores, int numJugadores) {
    FILE *archivo;
    
    // Determinar si es la primera vez que escribimos en el archivo
    bool primerRegistro = false;
    if (numRonda == 1) {
        // Intentar abrir el archivo para ver si existe
        archivo = fopen(ARCHIVO_HISTORIAL, "r");
        if (archivo == NULL) {
            primerRegistro = true;
        } else {
            fclose(archivo);
        }
    }
    
    // Abrir archivo en modo append (o crearlo si no existe)
    archivo = fopen(ARCHIVO_HISTORIAL, "a");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir/crear el archivo de historial\n");
        return;
    }
    
    // Si es el primer registro, escribir encabezado del archivo
    if (primerRegistro) {
        fprintf(archivo, "========================================\n");
        fprintf(archivo, "===== HISTORIAL COMPLETO DEL JUEGO =====\n");
        fprintf(archivo, "========================================\n\n");
    }
    
    // Escribir separador para esta ronda
    fprintf(archivo, "----------------------------------------\n");
    if (numRonda == -1) {
        fprintf(archivo, "========== RONDA FINAL ==========\n");
    } else {
        fprintf(archivo, "========== RONDA %d ==========\n", numRonda);
    }
    fprintf(archivo, "----------------------------------------\n");
    
    // Obtener y escribir timestamp
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    char timestamp[25];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    fprintf(archivo, "Fecha y hora: %s\n\n", timestamp);
    
    // Escribir estado de la mesa
    Apeada *apeadas = obtenerApeadas();
    int numApeadas = obtenerNumApeadas();
    Mazo *banca = obtenerBanca();
    
    fprintf(archivo, "ESTADO DE LA MESA:\n");
    fprintf(archivo, "  Número de apeadas: %d\n", numApeadas);
    fprintf(archivo, "  Cartas en la banca: %d\n\n", banca->numCartas);
    
    // Mostrar detalles de las apeadas
    if (numApeadas > 0) {
        fprintf(archivo, "  Detalle de apeadas:\n");
        for (int i = 0; i < numApeadas; i++) {
            fprintf(archivo, "    Apeada %d (Jugador %d): ", i, apeadas[i].idJugador);
            
            if (apeadas[i].esGrupo) {
                fprintf(archivo, "GRUPO, %d cartas, %d puntos\n", 
                       apeadas[i].jugada.grupo.numCartas, apeadas[i].puntos);
            } else {
                fprintf(archivo, "ESCALERA, %d cartas, %d puntos\n", 
                       apeadas[i].jugada.escalera.numCartas, apeadas[i].puntos);
            }
        }
        fprintf(archivo, "\n");
    }
    
    // Escribir estado de cada jugador
    fprintf(archivo, "ESTADO DE LOS JUGADORES:\n");
    for (int i = 0; i < numJugadores; i++) {
        fprintf(archivo, "  Jugador %d:\n", jugadores[i].id);
        
        // Convertir estado a texto
        fprintf(archivo, "    Estado: ");
        switch(jugadores[i].estado) {
            case LISTO:
                fprintf(archivo, "LISTO\n");
                break;
            case EJECUCION:
                fprintf(archivo, "EJECUCIÓN\n");
                break;
            case ESPERA_ES:
                fprintf(archivo, "ESPERA E/S\n");
                break;
            case BLOQUEADO:
                fprintf(archivo, "BLOQUEADO\n");
                break;
            default:
                fprintf(archivo, "DESCONOCIDO\n");
        }
        
        fprintf(archivo, "    Primera apeada: %s\n", jugadores[i].primeraApeada ? "SÍ" : "NO");
        fprintf(archivo, "    Cartas restantes: %d\n", jugadores[i].mano.numCartas);
        
        // Información de las cartas que tiene
        if (jugadores[i].mano.numCartas > 0) {
            fprintf(archivo, "    Cartas en mano:\n");
            char cartaStr[50];
            for (int j = 0; j < jugadores[i].mano.numCartas; j++) {
                obtenerNombreCarta(jugadores[i].mano.cartas[j], cartaStr);
                fprintf(archivo, "      %d. %s\n", j+1, cartaStr);
            }
        }
        
        // Estadísticas del BCP
        if (jugadores[i].bcp != NULL) {
            fprintf(archivo, "    Estadísticas BCP:\n");
            fprintf(archivo, "      Tiempo de ejecución: %d ms\n", jugadores[i].bcp->tiempoEjecucion);
            fprintf(archivo, "      Tiempo de espera: %d ms\n", jugadores[i].bcp->tiempoEspera);
            fprintf(archivo, "      Tiempo de E/S: %d ms\n", jugadores[i].bcp->tiempoES);
            fprintf(archivo, "      Intentos fallidos: %d\n", jugadores[i].bcp->intentosFallidos);
            fprintf(archivo, "      Cartas comidas: %d\n", jugadores[i].bcp->cartasComidas);
            fprintf(archivo, "      Veces apeado: %d\n", jugadores[i].bcp->vecesApeo);
            fprintf(archivo, "      Turnos perdidos: %d\n", jugadores[i].bcp->turnosPerdidos);
            fprintf(archivo, "      Cambios de estado: %d\n", jugadores[i].bcp->cambiosEstado);
        }
        
        fprintf(archivo, "\n");
        
        // También actualizamos el historial individual del jugador
        registrarHistorialJugador(numRonda, &jugadores[i]);
    }
    
    // Añadir una línea para separar rondas
    fprintf(archivo, "\n");
    
    // Cerrar el archivo
    fclose(archivo);
    
    // Actualizar contador de rondas para futuras referencias
    if (numRonda > 0) {
        rondaActual = numRonda;
    }
    
    // Mensaje de confirmación en la consola
    printf("Historial de la ronda %s guardado en '%s'\n", 
           numRonda == -1 ? "final" : "actual", 
           ARCHIVO_HISTORIAL);
}
// Función para mostrar el historial completo
void mostrarHistorialCompleto(void) {
    printf("\n=== HISTORIAL COMPLETO DEL JUEGO ===\n");
    printf("Se han registrado %d rondas\n", rondaActual);
    printf("Los archivos de historial son:\n");
    
    // Listar archivos de rondas
    for (int i = 1; i <= rondaActual; i++) {
        printf("  historial_ronda_%d.txt\n", i);
    }
    
    // Listar archivos de jugadores
    printf("\nHistorial por jugador:\n");
    // Aquí podrías determinar dinámicamente cuántos jugadores hay,
    // pero por ahora asumiremos el máximo (MAX_JUGADORES)
    for (int i = 0; i < MAX_JUGADORES; i++) {
        printf("  historial_jugador_%d.txt\n", i);
    }
    
    printf("\nPara ver el historial completo, consulte estos archivos.\n");
}

// Función para registrar el historial de un jugador específico
// Función para registrar el historial de un jugador específico
// Función para registrar el historial de un jugador específico
void registrarHistorialJugador(int numRonda, Jugador *jugador) {
    FILE *archivo;
    char nombreArchivo[100];
    
    // Crear nombre del archivo para este jugador
    sprintf(nombreArchivo, "historial_jugador_%d.txt", jugador->id);
    
    // Abrir archivo en modo append
    archivo = fopen(nombreArchivo, "a");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir el archivo de historial para el jugador %d\n", jugador->id);
        return;
    }
    
    // Si es la primera ronda, escribir encabezado
    if (numRonda == 1) {
        fprintf(archivo, "=== HISTORIAL DEL JUGADOR %d ===\n\n", jugador->id);
    }
    
    // Escribir información de la ronda
    if (numRonda == -1) {
        fprintf(archivo, "--- RONDA FINAL ---\n");
    } else {
        fprintf(archivo, "--- RONDA %d ---\n", numRonda);
    }
    
    // Obtener y escribir timestamp
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    char timestamp[25];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    fprintf(archivo, "Fecha y hora: %s\n", timestamp);
    
    // Escribir estado y estadísticas
    fprintf(archivo, "Estado: ");
    switch(jugador->estado) {
        case LISTO:
            fprintf(archivo, "LISTO\n");
            break;
        case EJECUCION:
            fprintf(archivo, "EJECUCIÓN\n");
            break;
        case ESPERA_ES:
            fprintf(archivo, "ESPERA E/S\n");
            break;
        case BLOQUEADO:
            fprintf(archivo, "BLOQUEADO\n");
            break;
        default:
            fprintf(archivo, "DESCONOCIDO\n");
    }
    
    fprintf(archivo, "Cartas restantes: %d\n", jugador->mano.numCartas);
    
    // Imprimir las cartas del jugador
    if (jugador->mano.numCartas > 0) {
        fprintf(archivo, "Cartas en mano:\n");
        char cartaStr[50];
        for (int i = 0; i < jugador->mano.numCartas; i++) {
            obtenerNombreCarta(jugador->mano.cartas[i], cartaStr);
            fprintf(archivo, "  %d. %s\n", i+1, cartaStr);
        }
    }
    
    // Escribir información del BCP
    if (jugador->bcp != NULL) {
        fprintf(archivo, "Estadísticas:\n");
        fprintf(archivo, "  Primera apeada: %s\n", jugador->primeraApeada ? "SÍ" : "NO");
        fprintf(archivo, "  Intentos fallidos: %d\n", jugador->bcp->intentosFallidos);
        fprintf(archivo, "  Cartas comidas: %d\n", jugador->bcp->cartasComidas);
        fprintf(archivo, "  Veces apeado: %d\n", jugador->bcp->vecesApeo);
        fprintf(archivo, "  Turnos perdidos: %d\n", jugador->bcp->turnosPerdidos);
        fprintf(archivo, "  Tiempo de ejecución: %d ms\n", jugador->bcp->tiempoEjecucion);
        fprintf(archivo, "  Tiempo de espera: %d ms\n", jugador->bcp->tiempoEspera);
        fprintf(archivo, "  Cambios de estado: %d\n", jugador->bcp->cambiosEstado);
    }
    
    fprintf(archivo, "\n");
    
    // Cerrar archivo
    fclose(archivo);
}


/* Nombres de palos */
const char* nombrePalo(char palo) {
    switch(palo) {
        case 'C': return "Negro";
        case 'D': return "Naranja";
        case 'T': return "Azul";
        case 'E': return "Rojo";
        case 'J': return "Comodin";
        default: return "Desconocido";
    }
}

/* Nombres de valores */
const char* nombreValor(int valor) {
    static char buffer[4];
    
    switch(valor) {
        case 1: return "1";
        case 11: return "11";
        case 12: return "12";
        case 13: return "13";
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
/* Registrar evento en un archivo log con número de ronda */
void registrarEvento(const char *formato, ...) {
    FILE *archivo;
    time_t ahora;
    struct tm *t;
    char timestamp[25];
    va_list args;
    
    /* Abrir archivo en modo append */
    archivo = fopen("juego.log", "a");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir el archivo de log\n");
        return;
    }
    
    /* Obtener timestamp */
    ahora = time(NULL);
    t = localtime(&ahora);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    /* Escribir timestamp y número de ronda */
    fprintf(archivo, "[%s] [Ronda %d] ", timestamp, rondaActual);
    
    /* Escribir mensaje formateado */
    va_start(args, formato);
    vfprintf(archivo, formato, args);
    va_end(args);
    
    /* Añadir salto de línea */
    fprintf(archivo, "\n");
    
    /* Forzar la escritura y cerrar archivo */
    fflush(archivo);
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