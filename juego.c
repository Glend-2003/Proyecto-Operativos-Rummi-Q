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
#include "memoria.h"
#define _DEFAULT_SOURCE

// ============== VARIABLES GLOBALES ==============
static bool juegoEnCurso = false;
static bool hayGanador = false;
static int idGanador = -1;
static int algoritmoActual = ALG_FCFS;
static Jugador jugadores[MAX_JUGADORES];
static int numJugadores = 0;
static int jugadorActual = 0;
static int quantum = 3000;

pthread_mutex_t mutexJuego = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condTurno = PTHREAD_COND_INITIALIZER;

// ============== INICIALIZACIÓN ==============
bool inicializarJuego(int cantidadJugadores) {
    if (cantidadJugadores <= 0 || cantidadJugadores > MAX_JUGADORES) {
        printf("Número de jugadores inválido. Debe ser entre 1 y %d\n", MAX_JUGADORES);
        return false;
    }
    
    srand((unsigned int)time(NULL));
    
    numJugadores = cantidadJugadores;
    juegoEnCurso = true;
    hayGanador = false;
    idGanador = -1;
    
    if (!inicializarMesa()) {
        printf("Error al inicializar la mesa\n");
        return false;
    }
    
    inicializarTabla();
    
    for (int i = 0; i < numJugadores; i++) {
        inicializarJugador(&jugadores[i], i);
    }
    
    repartirFichas();
    return true;
}

void repartirFichas() {
    Mazo mazoCompleto = {NULL, 0, 0};
    crearMazoCompleto(&mazoCompleto);
    
    if (mazoCompleto.cartas == NULL) {
        printf("Error: No se pudo crear el mazo completo\n");
        return;
    }

    mezclarMazo(&mazoCompleto);
    
    int cartasTotales = mazoCompleto.numCartas;
    int cartasPorJugador = (cartasTotales * 2) / (3 * numJugadores);
    
    // Repartir a jugadores
    for (int i = 0; i < numJugadores; i++) {
        for (int j = 0; j < cartasPorJugador && mazoCompleto.numCartas > 0; j++) {
            Carta carta = mazoCompleto.cartas[mazoCompleto.numCartas - 1];
            mazoCompleto.numCartas--;
            
            if (jugadores[i].mano.numCartas >= jugadores[i].mano.capacidad) {
                int nuevaCapacidad = jugadores[i].mano.capacidad == 0 ? cartasPorJugador : jugadores[i].mano.capacidad * 2;
                Carta *nuevasCartas = realloc(jugadores[i].mano.cartas, nuevaCapacidad * sizeof(Carta));
                
                if (nuevasCartas == NULL) {
                    printf("Error: No se pudo reasignar memoria para la mano del jugador %d\n", i);
                    continue;
                }
                
                jugadores[i].mano.cartas = nuevasCartas;
                jugadores[i].mano.capacidad = nuevaCapacidad;
            }
            
            jugadores[i].mano.cartas[jugadores[i].mano.numCartas] = carta;
            jugadores[i].mano.numCartas++;
        }
    }
    
    // Resto a la banca
    Mazo *banca = obtenerBanca();
    if (banca == NULL) {
        if (mazoCompleto.cartas != NULL) free(mazoCompleto.cartas);
        return;
    }

    for (int i = 0; i < mazoCompleto.numCartas; i++) {
        if (banca->numCartas >= banca->capacidad) {
            int nuevaCapacidad = banca->capacidad == 0 ? mazoCompleto.numCartas : banca->capacidad * 2;
            Carta *nuevasCartas = realloc(banca->cartas, nuevaCapacidad * sizeof(Carta));
            
            if (nuevasCartas == NULL) {
                printf("Error: No se pudo reasignar memoria para la banca\n");
                break;
            }
            
            banca->cartas = nuevasCartas;
            banca->capacidad = nuevaCapacidad;
        }
        
        banca->cartas[banca->numCartas] = mazoCompleto.cartas[i];
        banca->numCartas++;
    }
    
    if (mazoCompleto.cartas != NULL) {
        free(mazoCompleto.cartas);
    }
}

void iniciarJuego() {
    for (int i = 0; i < numJugadores; i++) {
        if (pthread_create(&jugadores[i].hilo, NULL, funcionHiloJugador, (void *)&jugadores[i]) != 0) {
            printf("Error al crear el hilo del jugador %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    
    bucleJuego();
}

// ============== BUCLE PRINCIPAL ==============
void bucleJuego() {
    printf("¡Iniciando el juego con %d jugadores!\n", numJugadores);
    
    int numRonda = 0;
    clock_t ultimaActualizacion = clock();
    const int INTERVALO_ACTUALIZACION = 5000;
    
    while (juegoEnCurso) {
        if (juegoTerminado()) break;
        
        numRonda++;
        rondaActual = numRonda;
        
        int siguienteJugador = (algoritmoActual == ALG_FCFS) ? 
                              seleccionarJugadorFCFS() : seleccionarJugadorRR();
        
        if (siguienteJugador == -1) {
            usleep(100000);
            continue;
        }
        
        asignarTurno(siguienteJugador);
        esperarFinTurno(siguienteJugador);
        
        if (hayGanador || juegoTerminado()) break;
        
        clock_t ahora = clock();
        int tiempoTranscurrido = (ahora - ultimaActualizacion) * 1000 / CLOCKS_PER_SEC;
        
        if (tiempoTranscurrido >= INTERVALO_ACTUALIZACION) {
            registrarHistorial(numRonda, jugadores, numJugadores);
            imprimirEstadisticasTabla();
            ultimaActualizacion = ahora;
        }
        
        usleep(10000);
    }
    
    printf("Esperando a que terminen los hilos de los jugadores...\n");
    
    for (int i = 0; i < numJugadores; i++) {
        time_t startTime = time(NULL);
        bool joined = false;
        
        #ifdef __USE_GNU
        int result = pthread_tryjoin_np(jugadores[i].hilo, NULL);
        if (result == 0) {
            joined = true;
        } else {
            while (time(NULL) - startTime < 3) {
                usleep(100000);
                result = pthread_tryjoin_np(jugadores[i].hilo, NULL);
                if (result == 0) {
                    joined = true;
                    break;
                }
            }
        }
        #else
        jugadores[i].terminado = true;
        pthread_join(jugadores[i].hilo, NULL);
        joined = true;
        #endif
        
        if (!joined) {
            printf("No se pudo esperar al hilo del Jugador %d, continuando...\n", i);
        }
    }
    
    mostrarResultados();
    printf("\nEl historial completo del juego se ha guardado en 'historial_juego.txt'\n");
    printf("El juego ha terminado. ¡Gracias por jugar!\n");
}

// ============== ALGORITMOS DE PLANIFICACIÓN ==============
int seleccionarJugadorFCFS() {
    for (int i = 0; i < numJugadores; i++) {
        int idx = (jugadorActual + i) % numJugadores;
        if (jugadores[idx].estado == LISTO && !jugadores[idx].terminado) {
            return idx;
        }
    }
    return -1;
}

int seleccionarJugadorRR() {
    int inicio = (jugadorActual + 1) % numJugadores;
    
    for (int i = 0; i < numJugadores; i++) {
        int idx = (inicio + i) % numJugadores;
        if (!jugadores[idx].terminado) {
            return idx;
        }
    }
    return -1;
}

void asignarTurno(int idJugador) {
    if (idJugador < 0 || idJugador >= numJugadores) return;
    
    jugadorActual = idJugador;
    
    if (algoritmoActual == ALG_FCFS) {
        jugadores[idJugador].tiempoTurno = 10000;
    } else {
        // Quantum dinámico basado en cartas
        int quantumDinamico = 1000 + (jugadores[idJugador].mano.numCartas * 100);
        if (quantumDinamico < 1500) quantumDinamico = 1500;
        if (quantumDinamico > 5000) quantumDinamico = 5000;
        
        jugadores[idJugador].tiempoTurno = quantumDinamico;
        quantum = quantumDinamico;
    }
    
    jugadores[idJugador].tiempoRestante = jugadores[idJugador].tiempoTurno;
    jugadores[idJugador].turnoActual = true;
    
    printf("Turno asignado al Jugador %d por %d ms\n", idJugador, jugadores[idJugador].tiempoTurno);
}

void esperarFinTurno(int idJugador) {
    if (idJugador < 0 || idJugador >= numJugadores) return;
    
    clock_t inicio = clock();
    
    while (jugadores[idJugador].turnoActual) {
        if (juegoTerminado()) {
            jugadores[idJugador].turnoActual = false;
            break;
        }
        
        int tiempoTranscurrido = (clock() - inicio) * 1000 / CLOCKS_PER_SEC;
        
        if (tiempoTranscurrido >= jugadores[idJugador].tiempoTurno) {
            jugadores[idJugador].tiempoRestante = 0;
            jugadores[idJugador].turnoActual = false;
            printf("Tiempo agotado para Jugador %d\n", idJugador);
            break;
        }
        
        usleep(50000);
    }
}

void cambiarAlgoritmo(int nuevoAlgoritmo) {
    if (nuevoAlgoritmo != ALG_FCFS && nuevoAlgoritmo != ALG_RR) {
        printf("Algoritmo no válido\n");
        return;
    }
    
    algoritmoActual = nuevoAlgoritmo;
    
    const char *nombres[] = {"FCFS", "Round Robin"};
    printf("Algoritmo cambiado a: %s\n", nombres[algoritmoActual]);
    
    if (nuevoAlgoritmo == ALG_RR) {
        printf("Usando quantum dinámico basado en el número de cartas:\n");
        printf("  - Base: 1000ms + 100ms por carta\n");
        printf("  - Mínimo: 1500ms, Máximo: 5000ms\n");
    }
}

// ============== FINALIZACIÓN ==============
void finalizarJuego(int idJugadorGanador) {
    hayGanador = true;
    idGanador = idJugadorGanador;
    juegoEnCurso = false;
    
    if (idJugadorGanador >= 0) {
        registrarEvento("¡El Jugador %d ha ganado el juego!", idJugadorGanador);
    } else {
        registrarEvento("Juego finalizado por el usuario");
    }
    
    registrarHistorial(-1, jugadores, numJugadores);
    imprimirEstadisticasTabla();
    imprimirEstadoMemoria();
    imprimirEstadoMemoriaVirtual();
    
    for (int i = 0; i < numJugadores; i++) {
        jugadores[i].terminado = true;
        jugadores[i].turnoActual = false;
        liberarMemoria(i);
        actualizarEstadoJugador(&jugadores[i], BLOQUEADO);
    }
    
    printf("Se guardaron todas las estadísticas. El juego ha terminado.\n");
}

bool juegoTerminado() {
    return !juegoEnCurso;
}

void mostrarResultados() {
    FILE *archivo = fopen("historial_juego.txt", "a");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir/crear el archivo de historial para resultados finales\n");
    } else {
        fprintf(archivo, "----------------------------------------\n");
        fprintf(archivo, "========== RESULTADOS FINALES ==========\n");
        fprintf(archivo, "----------------------------------------\n\n");
        
        if (hayGanador) {
            fprintf(archivo, "¡El Jugador %d ha ganado!\n\n", idGanador);
        } else {
            fprintf(archivo, "Juego terminado sin ganador\n\n");
        }
        
        fprintf(archivo, "Puntuaciones finales:\n");
        for (int i = 0; i < numJugadores; i++) {
            fprintf(archivo, "Jugador %d: %d puntos, %d cartas restantes\n", 
                   i, jugadores[i].puntosTotal, jugadores[i].mano.numCartas);
        }
        
        fclose(archivo);
    }
    
    printf("\n=== RESULTADOS FINALES ===\n");
    
    if (hayGanador) {
        printf("¡El Jugador %d ha ganado!\n", idGanador);
    } else {
        printf("Juego terminado sin ganador\n");
        
        printf("\nPuntuaciones finales:\n");
        for (int i = 0; i < numJugadores; i++) {
            printf("Jugador %d: %d puntos, %d cartas restantes\n", 
                   i, jugadores[i].puntosTotal, jugadores[i].mano.numCartas);
        }
        
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
    
    imprimirEstadisticasTabla();
}

void liberarJuego() {
    for (int i = 0; i < numJugadores; i++) {
        liberarJugador(&jugadores[i]);
        liberarMemoria(i);
    }
    
    liberarMesa();
    liberarTabla();
    imprimirEstadoMemoria();
    imprimirEstadoMemoriaVirtual();
    
    printf("Todos los recursos liberados correctamente.\n");
}

// ============== UTILIDADES ==============
int calcularPuntosMano(Mazo *mano) {
    int total = 0;
    
    for (int i = 0; i < mano->numCartas; i++) {
        Carta carta = mano->cartas[i];
        
        if (carta.esComodin) {
            total += 20;
        } else if (carta.valor >= 11) {
            total += 10;
        } else if (carta.valor == 1) {
            total += 15;
        } else {
            total += carta.valor;
        }
    }
    
    return total;
}