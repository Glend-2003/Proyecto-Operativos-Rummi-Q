#ifndef JUEGO_H
#define JUEGO_H

#include <stdbool.h>
#include "jugadores.h"

#define MAX_JUGADORES 4
#define ALG_FCFS 0
#define ALG_RR 1

// ============== INICIALIZACIÓN ==============
bool inicializarJuego(int cantidadJugadores);
void repartirFichas();
void iniciarJuego();

// ============== BUCLE PRINCIPAL ==============
void bucleJuego();

// ============== PLANIFICACIÓN ==============
int seleccionarJugadorFCFS();
int seleccionarJugadorRR();
void asignarTurno(int idJugador);
void esperarFinTurno(int idJugador);
void cambiarAlgoritmo(int nuevoAlgoritmo);

// ============== FINALIZACIÓN ==============
void finalizarJuego(int idJugadorGanador);
bool juegoTerminado();
void mostrarResultados();
void liberarJuego();

// ============== UTILIDADES ==============
int calcularPuntosMano(Mazo *mano);

#endif // JUEGO_H