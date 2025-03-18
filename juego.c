#include "juego.h"
#include "jugadores.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

Jugador jugadores[NUM_JUGADORES];  // Array de jugadores

// Función para iniciar el juego
void iniciar_juego(Juego *juego) {
    juego->juego_activo = true;
    juego->turno_actual = 0;

    // Inicializar la semilla para números aleatorios
    srand(time(NULL));

    // Inicializar jugadores y crear sus hilos
    for (int i = 0; i < NUM_JUGADORES; i++) {
        inicializar_jugador(&jugadores[i], i);
        pthread_create(&jugadores[i].hilo, NULL, turno_jugador, &jugadores[i]);
    }

    // Hilo principal del juego
    while (juego->juego_activo) {
        // Activar el turno del jugador actual
        jugadores[juego->turno_actual].turno_activo = true;

        // Esperar a que el jugador termine su turno
        while (jugadores[juego->turno_actual].turno_activo) {
            sleep(1);
        }

        // Pasar al siguiente jugador
        juego->turno_actual = (juego->turno_actual + 1) % NUM_JUGADORES;

        // Simular una condición de fin de juego (por ejemplo, después de 5 turnos)
        if (juego->turno_actual == 0) {
            juego->juego_activo = false;
            printf("El juego ha terminado.\n");
        }
    }

    // Esperar a que todos los hilos de los jugadores terminen
    for (int i = 0; i < NUM_JUGADORES; i++) {
        pthread_join(jugadores[i].hilo, NULL);
    }
}