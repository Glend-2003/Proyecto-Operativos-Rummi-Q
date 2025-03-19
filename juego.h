#ifndef JUEGO_H
#define JUEGO_H

#include <stdbool.h>
#include <pthread.h>

#define NUM_JUGADORES 4  // Número fijo de jugadores

// Estructura para representar el estado del juego
typedef struct {
    pthread_t hilo_principal;  // Hilo principal del juego
    bool juego_activo;         // Indica si el juego está activo
    int turno_actual;          // Índice del jugador actual
    int turnos_completados;    // Contador de rondas completadas
    int max_tiempo_turno;          // Índice del jugador actual
} Juego;

// Funciones
void iniciar_juego(Juego *juego);

#endif