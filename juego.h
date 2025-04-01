#ifndef JUEGO_H
#define JUEGO_H

#include <stdbool.h>
#include "jugadores.h"

#define MAX_JUGADORES 4

// Algoritmos de planificación
#define ALG_FCFS 0
#define ALG_RR 1

// Inicializar el juego
bool inicializarJuego(int cantidadJugadores);

// Repartir fichas a los jugadores
void repartirFichas();

// Iniciar el juego, creando los hilos de los jugadores
void iniciarJuego();

// Bucle principal del juego
void bucleJuego();

// Seleccionar el próximo jugador según FCFS
int seleccionarJugadorFCFS();

// Seleccionar el próximo jugador según Round Robin
int seleccionarJugadorRR();

// Asignar turno a un jugador
void asignarTurno(int idJugador);

// Esperar a que un jugador termine su turno
void esperarFinTurno(int idJugador);

// Cambiar el algoritmo de planificación
void cambiarAlgoritmo(int nuevoAlgoritmo);

// Finalizar el juego con un ganador
void finalizarJuego(int idJugadorGanador);

// Verificar si el juego ha terminado
bool juegoTerminado();

// Mostrar resultados finales
void mostrarResultados();

// Liberar recursos del juego
void liberarJuego();

// Calcular los puntos totales de una mano
int calcularPuntosMano(Mazo *mano);

#endif // JUEGO_H