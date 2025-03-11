#ifndef JUGADORES_H
#define JUGADORES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_JUGADORES 4
#define MAX_FICHAS 50  // Máximo número de fichas por jugador

// Estructura para representar jugadores
typedef struct {
    int id;                     // ID del jugador
    pthread_t hilo;             // Hilo del jugador
    bool turno_activo;          // Indica si es el turno del jugador
    int fichas[MAX_FICHAS];     // Fichas del jugador
    int num_fichas;             // Número de fichas del jugador
} Jugador;

// Funciones
void inicializar_jugador(Jugador *jugador, int id);
void* turno_jugador(void *arg);

#endif