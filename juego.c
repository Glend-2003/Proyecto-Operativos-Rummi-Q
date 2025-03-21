#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#include "juego.h"
#include "jugadores.h"
#include "mesa.h"
#include "procesos.h"
#include "utilidades.h"

// Variables globales para el manejo del juego
static bool juegoEnCurso = false;
static bool hayGanador = false;
static int idGanador = -1;
static int algoritmoActual = ALG_FCFS;  // FCFS por defecto

// Mutex y variables de condición para sincronización
pthread_mutex_t mutexJuego = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condTurno = PTHREAD_COND_INITIALIZER;

// Jugadores
static Jugador jugadores[MAX_JUGADORES];
static int numJugadores = 0;
static int jugadorActual = 0;

// Quantum para Round Robin
static int quantum = 3000;  // 3 segundos

// Inicializar el juego
bool inicializarJuego(int cantidadJugadores) {
    if (cantidadJugadores <= 0 || cantidadJugadores > MAX_JUGADORES) {
        printf("Número de jugadores inválido. Debe ser entre 1 y %d\n", MAX_JUGADORES);
        return false;
    }
    
    numJugadores = cantidadJugadores;
    juegoEnCurso = true;
    hayGanador = false;
    idGanador = -1;
    
    // Inicializar la mesa
    if (!inicializarMesa()) {
        printf("Error al inicializar la mesa\n");
        return false;
    }
    
    // Inicializar la tabla de procesos
    inicializarTabla();
    
    // Inicializar los jugadores
    for (int i = 0; i < numJugadores; i++) {
        inicializarJugador(&jugadores[i], i);
    }
    
    // Repartir fichas a los jugadores
    repartirFichas();
    
    return true;
}

// Repartir fichas a los jugadores
void repartirFichas() {
    // Crear el mazo completo (2 barajas + 4 comodines)
    Mazo mazoCompleto;
    crearMazoCompleto(&mazoCompleto);
    
    // Mezclar el mazo
    mezclarMazo(&mazoCompleto);
    
    // Calcular cuántas cartas repartir a cada jugador (2/3 del total)
    int cartasTotales = mazoCompleto.numCartas;
    int cartasPorJugador = (cartasTotales * 2) / (3 * numJugadores);
    
    // Repartir a cada jugador
    for (int i = 0; i < numJugadores; i++) {
        for (int j = 0; j < cartasPorJugador && mazoCompleto.numCartas > 0; j++) {
            // Tomar la última carta del mazo
            Carta carta = mazoCompleto.cartas[mazoCompleto.numCartas - 1];
            mazoCompleto.numCartas--;
            
            // Añadir a la mano del jugador
            if (jugadores[i].mano.numCartas >= jugadores[i].mano.capacidad) {
                int nuevaCapacidad = jugadores[i].mano.capacidad == 0 ? cartasPorJugador : jugadores[i].mano.capacidad * 2;
                jugadores[i].mano.cartas = realloc(jugadores[i].mano.cartas, nuevaCapacidad * sizeof(Carta));
                jugadores[i].mano.capacidad = nuevaCapacidad;
            }
            
            jugadores[i].mano.cartas[jugadores[i].mano.numCartas] = carta;
            jugadores[i].mano.numCartas++;
        }
    }
    
    // El resto de cartas van a la banca
    Mazo *banca = obtenerBanca();
    for (int i = 0; i < mazoCompleto.numCartas; i++) {
        if (banca->numCartas >= banca->capacidad) {
            int nuevaCapacidad = banca->capacidad == 0 ? mazoCompleto.numCartas : banca->capacidad * 2;
            banca->cartas = realloc(banca->cartas, nuevaCapacidad * sizeof(Carta));
            banca->capacidad = nuevaCapacidad;
        }
        
        banca->cartas[banca->numCartas] = mazoCompleto.cartas[i];
        banca->numCartas++;
    }
    
    // Liberar el mazo completo
    free(mazoCompleto.cartas);
}

// Iniciar el juego, creando los hilos de los jugadores
void iniciarJuego() {
    // Crear los hilos de los jugadores
    for (int i = 0; i < numJugadores; i++) {
        if (pthread_create(&jugadores[i].hilo, NULL, funcionHiloJugador, (void *)&jugadores[i]) != 0) {
            printf("Error al crear el hilo del jugador %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    
    // Iniciar el bucle principal del juego (hilo juego)
    bucleJuego();
}

// Bucle principal del juego
void bucleJuego() {
    printf("¡Iniciando el juego con %d jugadores!\n", numJugadores);
    
    while (juegoEnCurso) {
        // Seleccionar el próximo jugador según el algoritmo de planificación
        int siguienteJugador;
        
        if (algoritmoActual == ALG_FCFS) {
            // FCFS: Seleccionar el primer jugador en estado LISTO
            siguienteJugador = seleccionarJugadorFCFS();
        } else {
            // Round Robin: Seleccionar siguiente jugador y asignar quantum
            siguienteJugador = seleccionarJugadorRR();
        }
        
        // Si no hay jugadores disponibles, esperar un poco
        if (siguienteJugador == -1) {
            usleep(100000);  // 100ms
            continue;
        }
        
        // Asignar turno al jugador seleccionado
        asignarTurno(siguienteJugador);
        
        // Esperar a que el jugador termine su turno o se agote su tiempo
        esperarFinTurno(siguienteJugador);
        
        // Verificar si hay un ganador
        if (hayGanador) {
            break;
        }
        
        // Pequeña pausa para no consumir demasiado CPU
        usleep(10000);  // 10ms
    }
    
    // Esperar a que todos los hilos de jugadores terminen
    for (int i = 0; i < numJugadores; i++) {
        pthread_join(jugadores[i].hilo, NULL);
    }
    
    // Mostrar resultados finales
    mostrarResultados();
}

// Seleccionar el próximo jugador según FCFS
int seleccionarJugadorFCFS() {
    for (int i = 0; i < numJugadores; i++) {
        // Comenzamos desde el siguiente al último jugador que tuvo turno
        int idx = (jugadorActual + i) % numJugadores;
        
        // Si el jugador está listo y no ha terminado
        if (jugadores[idx].estado == LISTO && !jugadores[idx].terminado) {
            return idx;
        }
    }
    
    return -1;  // No hay jugadores listos
}

// Seleccionar el próximo jugador según Round Robin
int seleccionarJugadorRR() {
    // En Round Robin, simplemente tomamos el siguiente jugador que no haya terminado
    int inicio = (jugadorActual + 1) % numJugadores;
    
    for (int i = 0; i < numJugadores; i++) {
        int idx = (inicio + i) % numJugadores;
        
        // Si el jugador no ha terminado
        if (!jugadores[idx].terminado) {
            return idx;
        }
    }
    
    return -1;  // No hay jugadores disponibles
}

// Asignar turno a un jugador
void asignarTurno(int idJugador) {
    if (idJugador < 0 || idJugador >= numJugadores) {
        return;
    }
    
    jugadorActual = idJugador;
    
    // Asignar tiempo según el algoritmo
    if (algoritmoActual == ALG_FCFS) {
        // En FCFS, tiempo ilimitado (o un valor alto)
        jugadores[idJugador].tiempoTurno = 10000;  // 10 segundos
    } else {
        // En Round Robin, asignar quantum
        jugadores[idJugador].tiempoTurno = quantum;
    }
    
    // Establecer tiempo restante
    jugadores[idJugador].tiempoRestante = jugadores[idJugador].tiempoTurno;
    
    // Marcar como turno actual
    jugadores[idJugador].turnoActual = true;
    
    printf("Turno asignado al Jugador %d por %d ms\n", idJugador, jugadores[idJugador].tiempoTurno);
}

// Esperar a que un jugador termine su turno
void esperarFinTurno(int idJugador) {
    if (idJugador < 0 || idJugador >= numJugadores) {
        return;
    }
    
    // Esperar hasta que el jugador termine su turno o se agote su tiempo
    clock_t inicio = clock();
    
    while (jugadores[idJugador].turnoActual) {
        // Verificar si se ha agotado el tiempo
        int tiempoTranscurrido = (clock() - inicio) * 1000 / CLOCKS_PER_SEC;
        
        if (tiempoTranscurrido >= jugadores[idJugador].tiempoTurno) {
            // Forzar fin de turno por tiempo agotado
            jugadores[idJugador].tiempoRestante = 0;
            jugadores[idJugador].turnoActual = false;
            printf("Tiempo agotado para Jugador %d\n", idJugador);
            break;
        }
        
        // Pequeña pausa para no consumir demasiado CPU
        usleep(50000);  // 50ms
    }
}

// Cambiar el algoritmo de planificación
void cambiarAlgoritmo(int nuevoAlgoritmo) {
    if (nuevoAlgoritmo != ALG_FCFS && nuevoAlgoritmo != ALG_RR) {
        printf("Algoritmo no válido\n");
        return;
    }
    
    algoritmoActual = nuevoAlgoritmo;
    
    const char *nombres[] = {"FCFS", "Round Robin"};
    printf("Algoritmo cambiado a: %s\n", nombres[algoritmoActual]);
}

// Finalizar el juego con un ganador
void finalizarJuego(int idJugadorGanador) {
    hayGanador = true;
    idGanador = idJugadorGanador;
    juegoEnCurso = false;
}

// Verificar si el juego ha terminado
bool juegoTerminado() {
    return !juegoEnCurso;
}

// Mostrar resultados finales (continuación)
void mostrarResultados() {
    printf("\n=== RESULTADOS FINALES ===\n");
    
    if (hayGanador) {
        printf("¡El Jugador %d ha ganado!\n", idGanador);
    } else {
        printf("Juego terminado sin ganador\n");
        
        // Mostrar puntuaciones
        printf("\nPuntuaciones finales:\n");
        for (int i = 0; i < numJugadores; i++) {
            printf("Jugador %d: %d puntos, %d cartas restantes\n", 
                   i, jugadores[i].puntosTotal, jugadores[i].mano.numCartas);
        }
        
        // Determinar quien tiene menos puntos si no hay un ganador claro
        int menorPuntos = INT_MAX;
        int jugadorMenorPuntos = -1;
        
        for (int i = 0; i < numJugadores; i++) {
            int puntosTotales = calcularPuntosMano(&jugadores[i].mano);
            if (puntosTotales < menorPuntos) {
                menorPuntos = puntosTotales;
                jugadorMenorPuntos = i;
            }
        }
        
        if (jugadorMenorPuntos != -1) {
            printf("El Jugador %d tiene la menor cantidad de puntos (%d)\n", 
                   jugadorMenorPuntos, menorPuntos);
        }
    }
    
    // Mostrar estadísticas de los procesos
    imprimirEstadisticasTabla();
}

// Liberar recursos del juego
void liberarJuego() {
    // Liberar recursos de los jugadores
    for (int i = 0; i < numJugadores; i++) {
        liberarJugador(&jugadores[i]);
    }
    
    // Liberar recursos de la mesa
    liberarMesa();
    
    // Liberar la tabla de procesos
    liberarTabla();
}

// Calcular los puntos totales de una mano
int calcularPuntosMano(Mazo *mano) {
    int total = 0;
    
    for (int i = 0; i < mano->numCartas; i++) {
        Carta carta = mano->cartas[i];
        
        if (carta.esComodin) {
            total += 20;  // Los comodines valen 20 puntos
        } else {
            // Asignar valor según la carta
            if (carta.valor >= 11) {  // J, Q, K
                total += 10;
            } else if (carta.valor == 1) {  // As
                total += 15;
            } else {
                total += carta.valor;
            }
        }
    }
    
    return total;
}