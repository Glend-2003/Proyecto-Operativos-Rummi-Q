#ifndef UTILIDADES_H
#define UTILIDADES_H

#include <stdbool.h>
#include "jugadores.h"

//==============================================================================
// VARIABLES GLOBALES
//==============================================================================

extern int rondaActual;

//==============================================================================
// FUNCIONES DE MANEJO DE CARTAS
//==============================================================================

void imprimirCarta(Carta carta);
void imprimirMazo(Mazo *mazo);
char* obtenerNombreCarta(Carta carta, char *buffer);
int calcularPuntosCarta(Carta carta);

//==============================================================================
// FUNCIONES DE HISTORIAL Y REGISTRO
//==============================================================================

void registrarHistorial(int numRonda, Jugador *jugadores, int numJugadores);
void registrarHistorialRonda(int numRonda, Jugador *jugadores, int numJugadores);
void registrarHistorialJugador(int numRonda, Jugador *jugador);
void mostrarHistorialCompleto(void);
void registrarEvento(const char *formato, ...);
void guardarEstadisticasJuego(Jugador *jugadores, int numJugadores);

//==============================================================================
// FUNCIONES DE VISUALIZACIÓN
//==============================================================================

void colorRojo(void);
void colorVerde(void);
void colorAmarillo(void);
void colorAzul(void);
void colorMagenta(void);
void colorCian(void);
void colorReset(void);

#endif /* UTILIDADES_H */