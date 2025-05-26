#ifndef MESA_H
#define MESA_H

#include <stdbool.h>
#include "jugadores.h"

#define MAX_APEADAS 50

//==============================================================================
// ESTRUCTURAS
//==============================================================================

typedef struct {
    Apeada apeadas[MAX_APEADAS];
    int numApeadas;
    Mazo banca;
} Mesa;

extern Mesa mesaJuego;

//==============================================================================
// FUNCIONES DE INICIALIZACIÓN Y LIBERACIÓN
//==============================================================================

bool inicializarMesa(void);
void liberarMesa(void);

//==============================================================================
// FUNCIONES DE MANEJO DE MAZO
//==============================================================================

void crearMazoCompleto(Mazo *mazo);
void mezclarMazo(Mazo *mazo);
Mazo* obtenerBanca(void);

//==============================================================================
// FUNCIONES DE MANEJO DE APEADAS
//==============================================================================

bool agregarApeada(Apeada *nuevaApeada);
bool modificarApeada(int indiceApeada, Carta carta, int posicion);
Apeada* obtenerApeadas(void);
int obtenerNumApeadas(void);

//==============================================================================
// FUNCIONES DE VALIDACIÓN
//==============================================================================

bool validarApeada(Apeada *apeada);
bool esEscaleraValida(Carta *cartas, int numCartas);
bool esGrupoValido(Carta *cartas, int numCartas);

//==============================================================================
// FUNCIONES DE VISUALIZACIÓN
//==============================================================================

void mostrarApeadas(void);

#endif // MESA_H