#include "jugadores.h"

// Función para inicializar un jugador
void inicializar_jugador(Jugador *jugador, int id) {
    jugador->id = id;
    jugador->turno_activo = false;
    jugador->num_fichas = 0;  // Al principio, el jugador no tiene fichas
}

// Función que ejecuta el hilo de un jugador
// Función que ejecuta el hilo de un jugador
void* turno_jugador(void *arg) {
    Jugador *jugador = (Jugador *)arg;

    while (1) {
        if (jugador->turno_activo) {
            printf("Jugador %d: Mi turno\n", jugador->id);
            
            // Obtener el PID del proceso asociado al jugador
            int pid = obtener_pid_jugador(jugador->id);

            // Simular una jugada
            bool puede_jugar = (rand() % 10) < 8;  // 80% de probabilidad de poder jugar
            
            if (!puede_jugar) {
                printf("Jugador %d no tiene fichas para jugar, va a E/S\n", jugador->id);
                
                // Calcular tiempo aleatorio para E/S
                int tiempo_espera = calcular_tiempo_aleatorio_es();
                
                // Bloquear el proceso
                bloquear_proceso(pid, tiempo_espera);
                
                // Terminar el turno
                jugador->turno_activo = false;
                printf("Jugador %d: Fin de mi turno (bloqueado en E/S)\n", jugador->id);
            } else {
                // Simular una jugada exitosa
                if (jugador->num_fichas < MAX_FICHAS) {
                    jugador->fichas[jugador->num_fichas++] = rand() % 100;  // Ficha aleatoria
                    printf("Jugador %d roba una ficha. Ahora tiene %d fichas.\n", 
                           jugador->id, jugador->num_fichas);
                    
                    // Actualizar estadísticas del proceso
                    actualizar_puntos(pid, rand() % 10);  // Puntos aleatorios
                    incrementar_fichas_colocadas(pid, 1);
                    
                    // Verificar si es la primera apeada
                    if (!ha_hecho_primera_apeada(pid) && jugador->num_fichas >= 3) {
                        marcar_primera_apeada(pid);
                        printf("Jugador %d ha realizado su primera apeada!\n", jugador->id);
                    }
                } else {
                    printf("Jugador %d no puede robar más fichas (límite alcanzado).\n", jugador->id);
                }

                // Terminar el turno
                jugador->turno_activo = false;
                printf("Jugador %d: Fin de mi turno\n", jugador->id);
            }
        }
        sleep(1);  // Esperar antes de revisar de nuevo
    }

    return NULL;
}