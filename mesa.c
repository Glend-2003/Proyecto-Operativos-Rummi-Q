#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mesa.h"

Mesa mesaJuego;

//==============================================================================
// FUNCIONES DE INICIALIZACIÓN Y LIBERACIÓN
//==============================================================================

bool inicializarMesa(void) {
    mesaJuego.numApeadas = 0;
    
    mesaJuego.banca.cartas = NULL;
    mesaJuego.banca.numCartas = 0;
    mesaJuego.banca.capacidad = 0;
    
    return true;
}

void liberarMesa(void) {
    // Liberar escaleras (que usan memoria dinámica)
    for (int i = 0; i < mesaJuego.numApeadas; i++) {
        if (!mesaJuego.apeadas[i].esGrupo && mesaJuego.apeadas[i].jugada.escalera.cartas != NULL) {
            free(mesaJuego.apeadas[i].jugada.escalera.cartas);
            mesaJuego.apeadas[i].jugada.escalera.cartas = NULL;
        }
    }
    
    if (mesaJuego.banca.cartas != NULL) {
        free(mesaJuego.banca.cartas);
        mesaJuego.banca.cartas = NULL;
    }
    
    mesaJuego.numApeadas = 0;
}

//==============================================================================
// FUNCIONES DE MANEJO DE MAZO
//==============================================================================

void crearMazoCompleto(Mazo *mazo) {
    if (mazo == NULL) {
        printf("Error: Puntero de mazo nulo\n");
        return;
    }

    if (mazo->cartas != NULL) {
        free(mazo->cartas);
    }
    
    // 2 barajas * 52 cartas + 4 comodines = 108 cartas
    mazo->capacidad = 108;
    mazo->numCartas = 0;
    mazo->cartas = malloc(mazo->capacidad * sizeof(Carta));
    
    if (mazo->cartas == NULL) {
        printf("Error: No se pudo asignar memoria para el mazo\n");
        return;
    }

    char palos[] = {'C', 'D', 'T', 'E'};
    
    for (int baraja = 0; baraja < 2; baraja++) {
        for (int palo = 0; palo < 4; palo++) {
            for (int valor = 1; valor <= 13; valor++) {
                Carta carta;
                carta.valor = valor;
                carta.palo = palos[palo];
                carta.esComodin = false;
                
                mazo->cartas[mazo->numCartas] = carta;
                mazo->numCartas++;
            }
        }
        
        // Añadir 2 comodines por baraja
        for (int i = 0; i < 2; i++) {
            Carta carta;
            carta.valor = 0;
            carta.palo = 'J';
            carta.esComodin = true;
            
            mazo->cartas[mazo->numCartas] = carta;
            mazo->numCartas++;
        }
    }
}

void mezclarMazo(Mazo *mazo) {
    if (mazo->numCartas <= 1) {
        return;
    }
    
    // Algoritmo de Fisher-Yates
    for (int i = mazo->numCartas - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        
        Carta temp = mazo->cartas[i];
        mazo->cartas[i] = mazo->cartas[j];
        mazo->cartas[j] = temp;
    }
}

Mazo* obtenerBanca(void) {
    return &mesaJuego.banca;
}

//==============================================================================
// FUNCIONES DE MANEJO DE APEADAS
//==============================================================================

bool agregarApeada(Apeada *nuevaApeada) {
    if (mesaJuego.numApeadas >= MAX_APEADAS) {
        printf("Error: No se pueden agregar más apeadas a la mesa\n");
        return false;
    }
    
    if (!validarApeada(nuevaApeada)) {
        printf("Error: La apeada no es válida\n");
        return false;
    }
    
    mesaJuego.apeadas[mesaJuego.numApeadas] = *nuevaApeada;
    mesaJuego.numApeadas++;
    
    printf("Apeada agregada correctamente. Total de apeadas: %d\n", mesaJuego.numApeadas);
    return true;
}

bool modificarApeada(int indiceApeada, Carta carta, int posicion) {
    if (indiceApeada < 0 || indiceApeada >= mesaJuego.numApeadas) {
        printf("Error: Índice de apeada inválido\n");
        return false;
    }
    
    Apeada *apeada = &mesaJuego.apeadas[indiceApeada];
    
    if (apeada->esGrupo) {
        Grupo *grupo = &apeada->jugada.grupo;
        
        if (grupo->numCartas >= 4) {
            printf("Error: El grupo ya tiene el máximo de cartas (4)\n");
            return false;
        }
        
        if (!carta.esComodin && grupo->numCartas > 0 && 
            carta.valor != grupo->cartas[0].valor) {
            printf("Error: La carta no tiene el mismo valor que las demás del grupo\n");
            return false;
        }
        
        grupo->cartas[grupo->numCartas] = carta;
        grupo->numCartas++;
        
    } else {
        Escalera *escalera = &apeada->jugada.escalera;
        
        if (escalera->numCartas >= 13) {
            printf("Error: La escalera ya tiene el máximo de cartas (13)\n");
            return false;
        }
        
        if (!carta.esComodin && carta.palo != escalera->palo) {
            printf("Error: La carta no tiene el mismo palo que la escalera\n");
            return false;
        }
        
        if (escalera->numCartas >= escalera->capacidad) {
            int nuevaCapacidad = escalera->capacidad == 0 ? 13 : escalera->capacidad * 2;
            escalera->cartas = realloc(escalera->cartas, nuevaCapacidad * sizeof(Carta));
            escalera->capacidad = nuevaCapacidad;
        }
        
        if (posicion < 0 || posicion > escalera->numCartas) {
            escalera->cartas[escalera->numCartas] = carta;
        } else {
            for (int i = escalera->numCartas; i > posicion; i--) {
                escalera->cartas[i] = escalera->cartas[i-1];
            }
            escalera->cartas[posicion] = carta;
        }
        escalera->numCartas++;
    }
    
    if (!validarApeada(apeada)) {
        printf("Error: La modificación hace que la apeada no sea válida\n");
        return false;
    }
    
    printf("Apeada modificada correctamente\n");
    return true;
}

Apeada* obtenerApeadas(void) {
    return mesaJuego.apeadas;
}

int obtenerNumApeadas(void) {
    return mesaJuego.numApeadas;
}

//==============================================================================
// FUNCIONES DE VALIDACIÓN
//==============================================================================

bool validarApeada(Apeada *apeada) {
    if (apeada->esGrupo) {
        return esGrupoValido(apeada->jugada.grupo.cartas, apeada->jugada.grupo.numCartas);
    } else {
        return esEscaleraValida(apeada->jugada.escalera.cartas, apeada->jugada.escalera.numCartas);
    }
}

bool esGrupoValido(Carta *cartas, int numCartas) {
    if (numCartas < 3 || numCartas > 4) {
        return false;
    }
    
    int numComodines = 0;
    for (int i = 0; i < numCartas; i++) {
        if (cartas[i].esComodin) {
            numComodines++;
        }
    }
    
    if (numComodines == numCartas) {
        return false;
    }
    
    // Verificar que todas las cartas no comodines tengan el mismo valor
    int valorReferencia = -1;
    for (int i = 0; i < numCartas; i++) {
        if (!cartas[i].esComodin) {
            if (valorReferencia == -1) {
                valorReferencia = cartas[i].valor;
            } else if (cartas[i].valor != valorReferencia) {
                return false;
            }
        }
    }
    
    // Verificar que no haya palos repetidos (excepto comodines)
    for (int i = 0; i < numCartas; i++) {
        if (!cartas[i].esComodin) {
            for (int j = i + 1; j < numCartas; j++) {
                if (!cartas[j].esComodin && cartas[i].palo == cartas[j].palo) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

bool esEscaleraValida(Carta *cartas, int numCartas) {
    if (numCartas < 3) {
        return false;
    }
    
    int numComodines = 0;
    for (int i = 0; i < numCartas; i++) {
        if (cartas[i].esComodin) {
            numComodines++;
        }
    }
    
    if (numComodines == numCartas) {
        return false;
    }
    
    Carta *cartasOrdenadas = malloc(numCartas * sizeof(Carta));
    memcpy(cartasOrdenadas, cartas, numCartas * sizeof(Carta));
    
    // Ordenar las cartas por valor
    for (int i = 0; i < numCartas - 1; i++) {
        for (int j = 0; j < numCartas - i - 1; j++) {
            if (!cartasOrdenadas[j].esComodin && !cartasOrdenadas[j+1].esComodin && 
                cartasOrdenadas[j].valor > cartasOrdenadas[j+1].valor) {
                Carta temp = cartasOrdenadas[j];
                cartasOrdenadas[j] = cartasOrdenadas[j+1];
                cartasOrdenadas[j+1] = temp;
            }
        }
    }
    
    // Verificar que todas las cartas no comodines tengan el mismo palo
    char paloReferencia = '\0';
    for (int i = 0; i < numCartas; i++) {
        if (!cartasOrdenadas[i].esComodin) {
            if (paloReferencia == '\0') {
                paloReferencia = cartasOrdenadas[i].palo;
            } else if (cartasOrdenadas[i].palo != paloReferencia) {
                free(cartasOrdenadas);
                return false;
            }
        }
    }
    
    // Verificar secuencia (considerando comodines)
    int valorEsperado = -1;
    int comodinesDisponibles = numComodines;
    
    for (int i = 0; i < numCartas; i++) {
        if (!cartasOrdenadas[i].esComodin) {
            if (valorEsperado == -1) {
                valorEsperado = cartasOrdenadas[i].valor;
            } else {
                int huecos = cartasOrdenadas[i].valor - valorEsperado - 1;
                
                if (huecos > 0) {
                    if (huecos > comodinesDisponibles) {
                        free(cartasOrdenadas);
                        return false;
                    }
                    comodinesDisponibles -= huecos;
                }
                valorEsperado = cartasOrdenadas[i].valor;
            }
            valorEsperado++;
        }
    }
    
    free(cartasOrdenadas);
    return true;
}

//==============================================================================
// FUNCIONES DE VISUALIZACIÓN
//==============================================================================

void mostrarApeadas(void) {
    printf("\n=== APEADAS EN LA MESA (%d) ===\n", mesaJuego.numApeadas);
    
    for (int i = 0; i < mesaJuego.numApeadas; i++) {
        Apeada *apeada = &mesaJuego.apeadas[i];
        printf("Apeada %d (Jugador %d): ", i, apeada->idJugador);
        
        if (apeada->esGrupo) {
            printf("GRUPO: ");
            for (int j = 0; j < apeada->jugada.grupo.numCartas; j++) {
                Carta carta = apeada->jugada.grupo.cartas[j];
                if (carta.esComodin) {
                    printf("[COMODÍN] ");
                } else {
                    printf("[%d%c] ", carta.valor, carta.palo);
                }
            }
        } else {
            printf("ESCALERA: ");
            for (int j = 0; j < apeada->jugada.escalera.numCartas; j++) {
                Carta carta = apeada->jugada.escalera.cartas[j];
                if (carta.esComodin) {
                    printf("[COMODÍN] ");
                } else {
                    printf("[%d%c] ", carta.valor, carta.palo);
                }
            }
        }
        
        printf("(%d puntos)\n", apeada->puntos);
    }
    
    printf("\nBanca: %d cartas\n", mesaJuego.banca.numCartas);
}