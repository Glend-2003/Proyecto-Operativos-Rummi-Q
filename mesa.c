#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mesa.h"

// Variable global para la mesa
Mesa mesaJuego;

// Inicializar la mesa
bool inicializarMesa(void) {
    // Inicializar el contador de apeadas
    mesaJuego.numApeadas = 0;
    
    // Inicializar el mazo de la banca
    mesaJuego.banca.cartas = NULL;
    mesaJuego.banca.numCartas = 0;
    mesaJuego.banca.capacidad = 0;
    
    return true;
}

// Agregar una nueva apeada a la mesa
bool agregarApeada(Apeada *nuevaApeada) {
    if (mesaJuego.numApeadas >= MAX_APEADAS) {
        printf("Error: No se pueden agregar más apeadas a la mesa\n");
        return false;
    }
    
    // Primero validar la apeada
    if (!validarApeada(nuevaApeada)) {
        printf("Error: La apeada no es válida\n");
        return false;
    }
    
    // Copiar la apeada a la mesa
    mesaJuego.apeadas[mesaJuego.numApeadas] = *nuevaApeada;
    mesaJuego.numApeadas++;
    
    printf("Apeada agregada correctamente. Total de apeadas: %d\n", mesaJuego.numApeadas);
    return true;
}

// Modificar una apeada existente (añadir carta)
bool modificarApeada(int indiceApeada, Carta carta, int posicion) {
    if (indiceApeada < 0 || indiceApeada >= mesaJuego.numApeadas) {
        printf("Error: Índice de apeada inválido\n");
        return false;
    }
    
    Apeada *apeada = &mesaJuego.apeadas[indiceApeada];
    
    // Verificar si es grupo o escalera
    if (apeada->esGrupo) {
        // Para grupos, solo verificamos que la carta tenga el mismo valor
        Grupo *grupo = &apeada->jugada.grupo;
        
        if (grupo->numCartas >= 4) {
            printf("Error: El grupo ya tiene el máximo de cartas (4)\n");
            return false;
        }
        
        // Verificar que la carta tenga el mismo valor que las demás del grupo
        if (!carta.esComodin && grupo->numCartas > 0 && 
            carta.valor != grupo->cartas[0].valor) {
            printf("Error: La carta no tiene el mismo valor que las demás del grupo\n");
            return false;
        }
        
        // Añadir la carta al grupo
        grupo->cartas[grupo->numCartas] = carta;
        grupo->numCartas++;
        
    } else {
        // Para escaleras, es más complejo
        Escalera *escalera = &apeada->jugada.escalera;
        
        // Verificar capacidad
        if (escalera->numCartas >= 13) {
            printf("Error: La escalera ya tiene el máximo de cartas (13)\n");
            return false;
        }
        
        // Verificar que la carta tenga el mismo palo o sea comodín
        if (!carta.esComodin && carta.palo != escalera->palo) {
            printf("Error: La carta no tiene el mismo palo que la escalera\n");
            return false;
        }
        
        // Ampliar la escalera si es necesario
        if (escalera->numCartas >= escalera->capacidad) {
            int nuevaCapacidad = escalera->capacidad == 0 ? 13 : escalera->capacidad * 2;
            escalera->cartas = realloc(escalera->cartas, nuevaCapacidad * sizeof(Carta));
            escalera->capacidad = nuevaCapacidad;
        }
        
        // Añadir la carta a la escalera (en la posición correcta)
        // Esto es una simplificación - una implementación real debería ordenar
        // las cartas y verificar que la secuencia sea válida
        if (posicion < 0 || posicion > escalera->numCartas) {
            // Añadir al final
            escalera->cartas[escalera->numCartas] = carta;
        } else {
            // Insertar en la posición especificada
            for (int i = escalera->numCartas; i > posicion; i--) {
                escalera->cartas[i] = escalera->cartas[i-1];
            }
            escalera->cartas[posicion] = carta;
        }
        escalera->numCartas++;
    }
    
    // Validar que la apeada sigue siendo válida después de la modificación
    if (!validarApeada(apeada)) {
        printf("Error: La modificación hace que la apeada no sea válida\n");
        // Aquí deberíamos revertir la modificación, pero por simplicidad no lo hacemos
        return false;
    }
    
    printf("Apeada modificada correctamente\n");
    return true;
}

// Validar si una apeada cumple con las reglas del juego
bool validarApeada(Apeada *apeada) {
    if (apeada->esGrupo) {
        // Validar grupo (terna o cuaterna)
        return esGrupoValido(apeada->jugada.grupo.cartas, apeada->jugada.grupo.numCartas);
    } else {
        // Validar escalera
        return esEscaleraValida(apeada->jugada.escalera.cartas, apeada->jugada.escalera.numCartas);
    }
}

// Verificar si un conjunto de cartas forma un grupo válido (terna o cuaterna)
bool esGrupoValido(Carta *cartas, int numCartas) {
    if (numCartas < 3 || numCartas > 4) {
        return false;  // Un grupo debe tener 3 o 4 cartas
    }
    
    // Contar comodines
    int numComodines = 0;
    for (int i = 0; i < numCartas; i++) {
        if (cartas[i].esComodin) {
            numComodines++;
        }
    }
    
    // Si todas son comodines, no es válido
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
                return false;  // Valores diferentes
            }
        }
    }
    
    // Verificar que no haya palos repetidos (excepto comodines)
    for (int i = 0; i < numCartas; i++) {
        if (!cartas[i].esComodin) {
            for (int j = i + 1; j < numCartas; j++) {
                if (!cartas[j].esComodin && cartas[i].palo == cartas[j].palo) {
                    return false;  // Palos repetidos
                }
            }
        }
    }
    
    return true;
}

// Verificar si un conjunto de cartas forma una escalera válida
bool esEscaleraValida(Carta *cartas, int numCartas) {
    if (numCartas < 3) {
        return false;  // Una escalera debe tener al menos 3 cartas
    }
    
    // Contar comodines
    int numComodines = 0;
    for (int i = 0; i < numCartas; i++) {
        if (cartas[i].esComodin) {
            numComodines++;
        }
    }
    
    // Si todas son comodines, no es válido
    if (numComodines == numCartas) {
        return false;
    }
    
    // Crear una copia de las cartas para ordenarlas
    Carta *cartasOrdenadas = malloc(numCartas * sizeof(Carta));
    memcpy(cartasOrdenadas, cartas, numCartas * sizeof(Carta));
    
    // Ordenar las cartas por valor (burbuja simple)
    for (int i = 0; i < numCartas - 1; i++) {
        for (int j = 0; j < numCartas - i - 1; j++) {
            if (!cartasOrdenadas[j].esComodin && !cartasOrdenadas[j+1].esComodin && 
                cartasOrdenadas[j].valor > cartasOrdenadas[j+1].valor) {
                // Intercambiar
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
                return false;  // Palos diferentes
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
                // Calcular huecos
                int huecos = cartasOrdenadas[i].valor - valorEsperado - 1;
                
                if (huecos > 0) {
                    // Verificar si tenemos suficientes comodines para llenar los huecos
                    if (huecos > comodinesDisponibles) {
                        free(cartasOrdenadas);
                        return false;  // No hay suficientes comodines
                    }
                    
                    comodinesDisponibles -= huecos;
                }
                
                valorEsperado = cartasOrdenadas[i].valor;
            }
            
            valorEsperado++;  // Para la próxima carta
        }
    }
    
    free(cartasOrdenadas);
    return true;
}

// Obtener acceso a las apeadas
Apeada* obtenerApeadas(void) {
    return mesaJuego.apeadas;
}

// Obtener el número de apeadas
int obtenerNumApeadas(void) {
    return mesaJuego.numApeadas;
}

// Obtener acceso al mazo de la banca
Mazo* obtenerBanca(void) {
    return &mesaJuego.banca;
}

// Crear un mazo completo (2 barajas + 4 comodines)
void crearMazoCompleto(Mazo *mazo) {

    // Verificar si el puntero es nulo
    if (mazo == NULL) {
        printf("Error: Puntero de mazo nulo\n");
        return;
    }

    // Liberar memoria si ya hay cartas
    if (mazo->cartas != NULL) {
        free(mazo->cartas);
    }
    
    // 2 barajas * 52 cartas + 4 comodines = 108 cartas
    mazo->capacidad = 108;
    mazo->numCartas = 0;
    mazo->cartas = malloc(mazo->capacidad * sizeof(Carta));
    

    // Verificar si malloc tuvo éxito
    if (mazo->cartas == NULL) {
        printf("Error: No se pudo asignar memoria para el mazo\n");
        return;
    }

    // Crear las 2 barajas
    char palos[] = {'C', 'D', 'T', 'E'};  // Corazones, Diamantes, Tréboles, Espadas
    
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
            carta.palo = 'J';  // Joker
            carta.esComodin = true;
            
            mazo->cartas[mazo->numCartas] = carta;
            mazo->numCartas++;
        }
    }
}

// Mezclar un mazo
void mezclarMazo(Mazo *mazo) {
    if (mazo->numCartas <= 1) {
        return;  // No hay nada que mezclar
    }
    
    // Algoritmo de Fisher-Yates
    for (int i = mazo->numCartas - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        
        // Intercambiar cartas[i] y cartas[j]
        Carta temp = mazo->cartas[i];
        mazo->cartas[i] = mazo->cartas[j];
        mazo->cartas[j] = temp;
    }
}

// Mostrar todas las apeadas en la mesa
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

// 3. En mesa.c - Corregir liberarMesa()
void liberarMesa(void) {
    // Liberar escaleras (que usan memoria dinámica)
    for (int i = 0; i < mesaJuego.numApeadas; i++) {
        if (!mesaJuego.apeadas[i].esGrupo && mesaJuego.apeadas[i].jugada.escalera.cartas != NULL) {
            free(mesaJuego.apeadas[i].jugada.escalera.cartas);
            mesaJuego.apeadas[i].jugada.escalera.cartas = NULL;  // Importante!
        }
    }
    
    // Liberar banca
    if (mesaJuego.banca.cartas != NULL) {
        free(mesaJuego.banca.cartas);
        mesaJuego.banca.cartas = NULL;
    }
    
    mesaJuego.numApeadas = 0;
}