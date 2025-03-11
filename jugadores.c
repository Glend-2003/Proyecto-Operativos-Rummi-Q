#include "jugadores.h"

// Función para inicializar un jugador
void inicializar_jugador(Jugador *jugador, int id) {
    jugador->id = id;
    jugador->turno_activo = false;
    jugador->num_fichas = 0;  // Al principio, el jugador no tiene fichas
}

// Función que ejecuta el hilo de un jugador
void* turno_jugador(void *arg) {
    Jugador *jugador = (Jugador *)arg;

    while (1) {
        if (jugador->turno_activo) {
            printf("Jugador %d: Mi turno\n", jugador->id);

            // Simular una jugada (robar una ficha)
            if (jugador->num_fichas < MAX_FICHAS) {
                jugador->fichas[jugador->num_fichas++] = rand() % 100;  // Ficha aleatoria
                printf("Jugador %d roba una ficha. Ahora tiene %d fichas.\n", jugador->id, jugador->num_fichas);
            } else {
                printf("Jugador %d no puede robar más fichas (límite alcanzado).\n", jugador->id);
            }

            // Terminar el turno
            jugador->turno_activo = false;
            printf("Jugador %d: Fin de mi turno\n", jugador->id);
        }
        sleep(1);  // Esperar antes de revisar de nuevo
    }

    return NULL;
}