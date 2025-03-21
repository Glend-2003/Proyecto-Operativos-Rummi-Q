#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "jugadores.h"
#include "procesos.h"
#include "procesos_stub.h"

// Stubs (implementaciones simuladas) de funciones que dependen de otros módulos
// Estas implementaciones temporales nos permitirán probar nuestro código

// Simulación de banca y apeadas
Mazo bancaSimulada;
Apeada apeadasSimuladas[10];
int numApeadasSimuladas = 0;

// Variables para controlar el juego
bool juegoSimuladoTerminado = false;
int numTurnosSimulados = 0;
const int MAX_TURNOS_SIMULADOS = 20;  // Número máximo de turnos para la simulación

// Funciones stub para simular el comportamiento de otros módulos
Mazo* obtenerBanca() {
    return &bancaSimulada;
}

Apeada* obtenerApeadas() {
    return apeadasSimuladas;
}

int obtenerNumApeadas() {
    return numApeadasSimuladas;
}

bool juegoTerminado() {
    return juegoSimuladoTerminado || numTurnosSimulados >= MAX_TURNOS_SIMULADOS;
}

void finalizarJuego(int idJugadorGanador) {
    printf("¡Juego finalizado! Ganador: Jugador %d\n", idJugadorGanador);
    juegoSimuladoTerminado = true;
}

// Función para inicializar cartas de prueba
void inicializarCartasPrueba(Mazo *mazo, int numCartas) {
    mazo->capacidad = numCartas;
    mazo->numCartas = numCartas;
    mazo->cartas = (Carta*)malloc(numCartas * sizeof(Carta));
    
    // Crear cartas de prueba
    for (int i = 0; i < numCartas; i++) {
        Carta carta;
        carta.valor = (i % 13) + 1;  // Valores entre 1 y 13
        
        // Asignar palos (C, D, T, E)
        switch (i % 4) {
            case 0: carta.palo = 'C'; break;
            case 1: carta.palo = 'D'; break;
            case 2: carta.palo = 'T'; break;
            case 3: carta.palo = 'E'; break;
        }
        
        carta.esComodin = (i >= numCartas - 2);  // Los últimos 2 son comodines
        mazo->cartas[i] = carta;
    }
}

// Función para simular el comportamiento del hilo principal del juego
void simularHiloJuego(Jugador *jugadores, int numJugadores) {
    printf("Iniciando simulación del juego con %d jugadores\n", numJugadores);
    
    // Bucle principal de turnos
    while (!juegoTerminado()) {
        // Seleccionar un jugador al azar
        int jugadorActual = rand() % numJugadores;
        
        printf("\n--- Turno %d ---\n", numTurnosSimulados + 1);
        printf("Asignando turno al Jugador %d\n", jugadorActual);
        
        // Asignar tiempo de turno
        jugadores[jugadorActual].tiempoTurno = 2000;  // 2 segundos
        jugadores[jugadorActual].tiempoRestante = 2000;
        jugadores[jugadorActual].turnoActual = true;
        
        // Esperar a que el jugador termine su turno
        int tiempoEspera = 0;
        while (jugadores[jugadorActual].turnoActual && tiempoEspera < 3000) {
            usleep(100000);  // 100ms
            tiempoEspera += 100;
        }
        
        // Si el jugador no terminó a tiempo
        if (jugadores[jugadorActual].turnoActual) {
            printf("El Jugador %d no terminó su turno a tiempo\n", jugadorActual);
            jugadores[jugadorActual].turnoActual = false;
        }
        
        // Incrementar contador de turnos
        numTurnosSimulados++;
        
        // Pequeña pausa entre turnos
        usleep(500000);  // 500ms
    }
    
    printf("\n=== Simulación finalizada tras %d turnos ===\n", numTurnosSimulados);
}

int main() {
    // Inicializar semilla para números aleatorios
    srand(time(NULL));
    
    // Inicializar tabla de procesos
    inicializarTabla();
    
    // Inicializar banca simulada
    inicializarCartasPrueba(&bancaSimulada, 30);

    // Crear jugadores de prueba
    //hola
    const int NUM_JUGADORES_PRUEBA = 4;
    Jugador jugadores[NUM_JUGADORES_PRUEBA];
    
    for (int i = 0; i < NUM_JUGADORES_PRUEBA; i++) {
        inicializarJugador(&jugadores[i], i);
        
        // Asignar cartas a cada jugador
        inicializarCartasPrueba(&jugadores[i].mano, 10);
        
        // Registrar en la tabla de procesos
        registrarProcesoEnTabla(i, LISTO);
    }
    
    // Crear hilos para los jugadores
    for (int i = 0; i < NUM_JUGADORES_PRUEBA; i++) {
        if (pthread_create(&jugadores[i].hilo, NULL, funcionHiloJugador, (void *)&jugadores[i]) != 0) {
            printf("Error al crear el hilo del jugador %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    
    // Simular el comportamiento del hilo principal del juego
    simularHiloJuego(jugadores, NUM_JUGADORES_PRUEBA);
    
    // Esperar a que terminen todos los hilos
    for (int i = 0; i < NUM_JUGADORES_PRUEBA; i++) {
        pthread_join(jugadores[i].hilo, NULL);
    }
    
    // Mostrar estadísticas finales
    imprimirEstadisticasTabla();
    
    // Liberar recursos
    for (int i = 0; i < NUM_JUGADORES_PRUEBA; i++) {
        liberarJugador(&jugadores[i]);
    }
    
    // Liberar la tabla de procesos
    liberarTabla();
    
    // Liberar la banca simulada
    free(bancaSimulada.cartas);
    
    return 0;
    
}