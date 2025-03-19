#include "juego.h"
#include "jugadores.h"
#include "procesos.h"  // Incluir el nuevo header
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
    
    // Inicializar el sistema de procesos
    inicializar_sistema_procesos();

    // Inicializar jugadores y crear sus hilos
    for (int i = 0; i < NUM_JUGADORES; i++) {
        inicializar_jugador(&jugadores[i], i);
        pthread_create(&jugadores[i].hilo, NULL, turno_jugador, &jugadores[i]);
    }
    
    // Iniciar el monitoreo de teclado para cambiar algoritmos
    iniciar_monitoreo_teclado();

    // Hilo principal del juego
    while (juego->juego_activo) {
        // Antes de activar el turno, verificar el estado del proceso del jugador
        int pid_jugador = obtener_pid_jugador(juego->turno_actual);
        BCP *bcp_jugador = obtener_bcp(pid_jugador);
        
        if (bcp_jugador && bcp_jugador->estado == PROCESO_LISTO) {
            // Activar el turno del jugador actual solo si su proceso está listo
            jugadores[juego->turno_actual].turno_activo = true;
            
            // Actualizar el estado del proceso a EJECUTANDO
            actualizar_bcp(pid_jugador, PROCESO_EJECUTANDO);
            imprimir_estado_procesos(); // Mostrar estado actual
            
            // Esperar a que el jugador termine su turno o se agote su tiempo
            int espera = 0;
            while (jugadores[juego->turno_actual].turno_activo && espera < juego->max_tiempo_turno) {
                sleep(1);
                espera++;
            }
            
            // Si el jugador no terminó su turno, es un turno perdido
            if (jugadores[juego->turno_actual].turno_activo) {
                jugadores[juego->turno_actual].turno_activo = false;
                registrar_turno_perdido(pid_jugador);
                printf("Jugador %d agotó su tiempo y perdió el turno\n", juego->turno_actual);
            } else {
                registrar_turno_completado(pid_jugador);
            }
            
            // Actualizar el estado del proceso de nuevo a LISTO
            actualizar_bcp(pid_jugador, PROCESO_LISTO);
        } else {
            // Si el proceso no está listo, el jugador pierde su turno
            printf("Jugador %d no está listo (proceso bloqueado o terminado)\n", juego->turno_actual);
            
            // Si está bloqueado en E/S, informar
            if (bcp_jugador && bcp_jugador->estado == PROCESO_BLOQUEADO) {
                printf("Jugador %d está en E/S, tiempo restante: %ld segundos\n", 
                       juego->turno_actual, bcp_jugador->tiempo_bloqueado - time(NULL));
            }
        }

        // Pasar al siguiente jugador
        juego->turno_actual = (juego->turno_actual + 1) % NUM_JUGADORES;

        // Simular una condición de fin de juego (por ejemplo, después de 5 turnos)
        if (juego->turno_actual == 0) {
            // Verificar condiciones reales de fin de juego aquí
            // Por ahora solo simulamos con un contador
            juego->turnos_completados++;
            if (juego->turnos_completados >= 5) {
                juego->juego_activo = false;
                printf("El juego ha terminado después de %d rondas.\n", juego->turnos_completados);
            }
        }
    }

    // Finalizar el sistema de procesos
    finalizar_sistema_procesos();
    
    // Esperar a que todos los hilos de los jugadores terminen
    for (int i = 0; i < NUM_JUGADORES; i++) {
        pthread_join(jugadores[i].hilo, NULL);
    }
}