#ifndef MESA_H
#define MESA_H

#include <stdbool.h>

// Incluir definiciones de Carta, Grupo, Escalera, Apeada y Mazo
#include "jugadores.h"

#define MAX_APEADAS 50

// Estructura global para la mesa de juego
typedef struct {
    Apeada apeadas[MAX_APEADAS];
    int numApeadas;
    Mazo banca;
    pthread_mutex_t mutex; // Añadir mutex para sincronización
} Mesa;

// Variable global para la mesa
extern Mesa mesaJuego;

// Inicializar la mesa
bool inicializarMesa(void);

// Agregar una nueva apeada (grupo o escalera) a la mesa
bool agregarApeada(Apeada *nuevaApeada);

// Modificar una apeada existente (añadir carta)
bool modificarApeada(int indiceApeada, Carta carta, int posicion);

// Validar si una apeada cumple con las reglas del juego
bool validarApeada(Apeada *apeada);

// Verificar si un conjunto de cartas forma una escalera válida
bool esEscaleraValida(Carta *cartas, int numCartas);

// Verificar si un conjunto de cartas forma un grupo válido (terna o cuaterna)
bool esGrupoValido(Carta *cartas, int numCartas);

// Obtener acceso a las apeadas
Apeada* obtenerApeadas(void);

// Obtener el número de apeadas
int obtenerNumApeadas(void);

// Obtener acceso al mazo de la banca
Mazo* obtenerBanca(void);

// Crear un mazo completo (2 barajas + 4 comodines)
void crearMazoCompleto(Mazo *mazo);

// Mezclar un mazo
void mezclarMazo(Mazo *mazo);

// Mostrar todas las apeadas en la mesa
void mostrarApeadas(void);

// Liberar recursos de la mesa
void liberarMesa(void);

#endif // MESA_H