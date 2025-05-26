#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include "jugadores.h"
#include "mesa.h"
#include "procesos.h"
#include "utilidades.h"
#include "memoria.h"
#define _DEFAULT_SOURCE

// ============== MUTEX GLOBALES ==============
pthread_mutex_t mutexApeadas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBanca = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTabla = PTHREAD_MUTEX_INITIALIZER;

// ============== INICIALIZACIÓN ==============
void inicializarJugador(Jugador *jugador, int id) {
    jugador->id = id;
    jugador->estado = LISTO;
    jugador->primeraApeada = false;
    jugador->tiempoTurno = 0;
    jugador->tiempoRestante = 0;
    jugador->tiempoES = 0;
    jugador->turnoActual = false;
    jugador->puntosTotal = 0;
    jugador->terminado = false;
    
    jugador->mano.cartas = NULL;
    jugador->mano.numCartas = 0;
    jugador->mano.capacidad = 0;
    
    jugador->bcp = crearBCP(id);
    actualizarBCPJugador(jugador);
}

// ============== HILO PRINCIPAL DEL JUGADOR ==============
void *funcionHiloJugador(void *arg) {
    Jugador *jugador = (Jugador *)arg;
    
    pthread_mutex_lock(&mutexTabla);
    registrarProcesoEnTabla(jugador->id, PROC_BLOQUEADO);
    pthread_mutex_unlock(&mutexTabla);
    
    while (!jugador->terminado && !juegoTerminado()) {
        while (!jugador->turnoActual && !jugador->terminado && !juegoTerminado()) {
            if (jugador->estado == ESPERA_ES) {
                if (jugador->tiempoES > 0) {
                    usleep(50000);
                    jugador->tiempoES -= 50;
                    actualizarBCPJugador(jugador);
                    
                    if (jugador->tiempoES <= 0) {
                        salirEsperaES(jugador);
                    }
                }
            } else {
                usleep(100000);
            }
            
            if (juegoTerminado()) break;
        }
        
        if (jugador->terminado || juegoTerminado()) break;
        
        actualizarEstadoJugador(jugador, EJECUCION);
        
        Apeada *apeadas = obtenerApeadas();
        int numApeadas = obtenerNumApeadas();
        Mazo *banca = obtenerBanca();
        
        bool turnoCompletado = realizarTurno(jugador, apeadas, numApeadas, banca);
        
        if (!turnoCompletado) {
            printf("Jugador %d se quedó sin tiempo en su turno\n", jugador->id);
        }
        
        if (jugador->mano.numCartas == 0 && banca->numCartas == 0) {
            printf("¡Jugador %d ha ganado!\n", jugador->id);
            jugador->terminado = true;
            finalizarJuego(jugador->id);
        }
        
        pasarTurno(jugador);
    }
    
    pthread_mutex_lock(&mutexTabla);
    registrarProcesoEnTabla(jugador->id, PROC_TERMINADO);
    pthread_mutex_unlock(&mutexTabla);
    
    printf("Hilo del Jugador %d ha terminado correctamente\n", jugador->id);
    return NULL;
}

// ============== LÓGICA DE TURNO ==============
bool realizarTurno(Jugador *jugador, Apeada *apeadas, int numApeadas, Mazo *banca) {
    printf("\n--- Jugador %d está ejecutando su turno ---\n", jugador->id);
    
    if (jugador->tiempoRestante <= 0) {
        printf("Jugador %d no tiene tiempo suficiente para su turno\n", jugador->id);
        return false;
    }
    
    clock_t inicio = clock();
    char cartaStr[50];
    bool turnoCompletado = false;
    bool hizoJugada = false;
    
    printf("Mano del Jugador %d (%d cartas):\n", jugador->id, jugador->mano.numCartas);
    for (int i = 0; i < jugador->mano.numCartas; i++) {
        printf("  %d. %s\n", i+1, obtenerNombreCarta(jugador->mano.cartas[i], cartaStr));
    }
    
    while ((clock() - inicio) * 1000 / CLOCKS_PER_SEC < jugador->tiempoRestante) {
        if (!jugador->primeraApeada) {
            if (intentarPrimeraApeada(jugador, &hizoJugada)) {
                registrarEvento("Jugador %d realizó una jugada en una apeada", jugador->id);
                return true;
            }
        } else {
            if (intentarJugadaExistente(jugador, apeadas, numApeadas, &hizoJugada)) {
                // Jugada realizada
            }
        }
        
        if (!hizoJugada) {
            if (intentarComerFicha(jugador, banca)) {
                entrarEsperaES(jugador);
                turnoCompletado = true;
                break;
            } else {
                turnoCompletado = true;
                break;
            }
        }
        
        if (hizoJugada) {
            if (jugador->mano.numCartas == 0) {
                colorVerde();
                printf("¡Jugador %d se ha quedado sin cartas!\n", jugador->id);
                colorReset();
                turnoCompletado = true;
                break;
            }
            hizoJugada = false;
        } else {
            colorAmarillo();
            printf("Jugador %d no pudo hacer ninguna jugada, fin del turno\n", jugador->id);
            colorReset();
            turnoCompletado = true;
            break;
        }
        
        usleep(10000);
    }
    
    int tiempoTranscurrido = (clock() - inicio) * 1000 / CLOCKS_PER_SEC;
    jugador->tiempoRestante -= tiempoTranscurrido;
    
    if (jugador->tiempoRestante <= 0) {
        jugador->tiempoRestante = 0;
        colorAmarillo();
        printf("Jugador %d se quedó sin tiempo en su turno\n", jugador->id);
        colorReset();
        
        if (jugador->bcp != NULL) {
            jugador->bcp->turnosPerdidos++;
            actualizarBCPJugador(jugador);
        }
    }
    
    printf("Mano final del Jugador %d (%d cartas):\n", jugador->id, jugador->mano.numCartas);
    for (int i = 0; i < jugador->mano.numCartas; i++) {
        char cartaStr2[50];
        printf("  %d. %s\n", i+1, obtenerNombreCarta(jugador->mano.cartas[i], cartaStr2));
    }
    
    printf("--- Fin del turno del Jugador %d (tiempo restante: %d ms) ---\n", 
           jugador->id, jugador->tiempoRestante);
    
    return turnoCompletado;
}

bool intentarPrimeraApeada(Jugador *jugador, bool *hizoJugada) {
    colorAzul();
    printf("Jugador %d intenta hacer su primera apeada (necesita 30+ puntos)\n", jugador->id);
    colorReset();
    
    if (puedeApearse(jugador)) {
        Apeada *nuevaApeada = crearApeada(jugador);
        
        if (nuevaApeada != NULL) {
            pthread_mutex_lock(&mutexApeadas);
            if (agregarApeada(nuevaApeada)) {
                colorVerde();
                printf("¡Jugador %d ha realizado su primera apeada!\n", jugador->id);
                colorReset();
                jugador->primeraApeada = true;
                *hizoJugada = true;
                free(nuevaApeada);
            } else {
                colorRojo();
                printf("Error: No se pudo agregar la apeada a la mesa\n");
                colorReset();
                free(nuevaApeada);
            }
            pthread_mutex_unlock(&mutexApeadas);
        } else {
            colorRojo();
            printf("Jugador %d no pudo formar una apeada con 30+ puntos\n", jugador->id);
            colorReset();
            
            if (jugador->bcp != NULL) {
                jugador->bcp->intentosFallidos++;
                jugador->bcp->puntosAcumulados += 5;
                actualizarBCPJugador(jugador);
            }
            return true;
        }
    } else {
        colorAmarillo();
        printf("Jugador %d no tiene cartas para hacer su primera apeada\n", jugador->id);
        colorReset();
    }
    return false;
}

bool intentarJugadaExistente(Jugador *jugador, Apeada *apeadas, int numApeadas, bool *hizoJugada) {
    colorAzul();
    printf("Jugador %d busca jugadas en las apeadas existentes\n", jugador->id);
    colorReset();
    
    pthread_mutex_lock(&mutexApeadas);
    
    bool hizoBusqueda = false;
    for (int i = 0; i < numApeadas; i++) {
        if (verificarApeada(jugador, &apeadas[i])) {
            colorVerde();
            printf("Jugador %d puede modificar la apeada %d\n", jugador->id, i);
            colorReset();
            
            if (realizarJugadaApeada(jugador, &apeadas[i])) {
                colorVerde();
                printf("¡Jugador %d ha realizado una jugada en la apeada %d!\n", jugador->id, i);
                colorReset();
                *hizoJugada = true;
                hizoBusqueda = true;
                break;
            }
        }
    }
    
    if (!hizoBusqueda && jugador->mano.numCartas > 0) {
        colorAzul();
        printf("Jugador %d intenta crear una nueva apeada\n", jugador->id);
        colorReset();
        
        Apeada *nuevaApeada = crearApeada(jugador);
        
        if (nuevaApeada != NULL) {
            if (agregarApeada(nuevaApeada)) {
                colorVerde();
                printf("¡Jugador %d ha creado una nueva apeada!\n", jugador->id);
                colorReset();
                *hizoJugada = true;
                free(nuevaApeada);
            } else {
                colorRojo();
                printf("Error: No se pudo agregar la apeada a la mesa\n");
                colorReset();
                free(nuevaApeada);
            }
        }
    }
    
    pthread_mutex_unlock(&mutexApeadas);
    return *hizoJugada;
}

bool intentarComerFicha(Jugador *jugador, Mazo *banca) {
    colorAmarillo();
    printf("Jugador %d no pudo hacer jugada, intenta comer ficha\n", jugador->id);
    colorReset();
    
    pthread_mutex_lock(&mutexBanca);
    
    if (banca->numCartas > 0) {
        bool comio = comerFicha(jugador, banca);
        if (comio) {
            colorVerde();
            printf("Jugador %d comió una ficha. Entrando en E/S\n", jugador->id);
            colorReset();
            
            if (jugador->bcp != NULL) {
                jugador->bcp->cartasComidas++;
                actualizarBCPJugador(jugador);
            }
            
            pthread_mutex_unlock(&mutexBanca);
            return true;
        }
    } else {
        colorRojo();
        printf("Jugador %d no puede hacer jugada y no hay fichas para comer\n", jugador->id);
        colorReset();
    }
    
    pthread_mutex_unlock(&mutexBanca);
    return false;
}

// ============== VALIDACIÓN DE APEADAS ==============
bool puedeApearse(Jugador *jugador) {
    if (jugador->primeraApeada) return true;
    
    Carta *cartas = (Carta*)malloc(jugador->mano.numCartas * sizeof(Carta));
    if (cartas == NULL) {
        printf("Error: No se pudo asignar memoria para las cartas\n");
        return false;
    }
    
    memcpy(cartas, jugador->mano.cartas, jugador->mano.numCartas * sizeof(Carta));
    
    // Ordenar cartas
    for (int i = 0; i < jugador->mano.numCartas - 1; i++) {
        for (int j = 0; j < jugador->mano.numCartas - i - 1; j++) {
            if (cartas[j].palo > cartas[j+1].palo || 
                (cartas[j].palo == cartas[j+1].palo && cartas[j].valor > cartas[j+1].valor)) {
                Carta temp = cartas[j];
                cartas[j] = cartas[j+1];
                cartas[j+1] = temp;
            }
        }
    }
    
    int maxPuntos = 0;
    
    // Buscar grupos
    for (int i = 1; i <= 13; i++) {
        Carta grupo[4];
        int numCartasGrupo = 0;
        int puntosGrupo = 0;
        
        for (int j = 0; j < jugador->mano.numCartas && numCartasGrupo < 4; j++) {
            if (!cartas[j].esComodin && cartas[j].valor == i) {
                bool paloRepetido = false;
                for (int k = 0; k < numCartasGrupo; k++) {
                    if (grupo[k].palo == cartas[j].palo) {
                        paloRepetido = true;
                        break;
                    }
                }
                
                if (!paloRepetido) {
                    grupo[numCartasGrupo] = cartas[j];
                    numCartasGrupo++;
                    
                    if (cartas[j].valor >= 11) {
                        puntosGrupo += 10;
                    } else if (cartas[j].valor == 1) {
                        puntosGrupo += 15;
                    } else {
                        puntosGrupo += cartas[j].valor;
                    }
                }
            }
        }
        
        // Contar comodines disponibles
        int comodinesDisponibles = 0;
        for (int j = 0; j < jugador->mano.numCartas; j++) {
            if (cartas[j].esComodin) {
                comodinesDisponibles++;
            }
        }
        
        if (numCartasGrupo == 2 && comodinesDisponibles >= 1) {
            numCartasGrupo++;
            puntosGrupo += 20;
        }
        
        if (numCartasGrupo >= 3) {
            maxPuntos += puntosGrupo;
            if (maxPuntos >= 30) {
                free(cartas);
                return true;
            }
        }
    }
    
    // Buscar escaleras
    for (char palo = 'C'; palo <= 'T'; palo++) {
        Carta escaleraTemp[13];
        int numCartasEscalera = 0;
        
        for (int j = 0; j < jugador->mano.numCartas; j++) {
            if (!cartas[j].esComodin && cartas[j].palo == palo) {
                escaleraTemp[numCartasEscalera] = cartas[j];
                numCartasEscalera++;
            }
        }
        
        // Ordenar por valor
        for (int i = 0; i < numCartasEscalera - 1; i++) {
            for (int j = 0; j < numCartasEscalera - i - 1; j++) {
                if (escaleraTemp[j].valor > escaleraTemp[j+1].valor) {
                    Carta temp = escaleraTemp[j];
                    escaleraTemp[j] = escaleraTemp[j+1];
                    escaleraTemp[j+1] = temp;
                }
            }
        }
        
        // Buscar secuencias
        int inicioEscalera = 0;
        while (inicioEscalera < numCartasEscalera) {
            int finEscalera = inicioEscalera;
            
            while (finEscalera + 1 < numCartasEscalera && 
                   escaleraTemp[finEscalera + 1].valor == escaleraTemp[finEscalera].valor + 1) {
                finEscalera++;
            }
            
            int longitudEscalera = finEscalera - inicioEscalera + 1;
            
            int comodinesDisponibles = 0;
            for (int j = 0; j < jugador->mano.numCartas; j++) {
                if (cartas[j].esComodin) {
                    comodinesDisponibles++;
                }
            }
            
            int huecos = 0;
            for (int j = inicioEscalera; j < finEscalera; j++) {
                huecos += escaleraTemp[j+1].valor - escaleraTemp[j].valor - 1;
            }
            
            int longitudFinal = longitudEscalera;
            if (huecos <= comodinesDisponibles) {
                longitudFinal += huecos;
            }
            
            if (longitudFinal >= 3) {
                int puntosEscalera = 0;
                
                for (int j = inicioEscalera; j <= finEscalera; j++) {
                    if (escaleraTemp[j].valor >= 11) {
                        puntosEscalera += 10;
                    } else if (escaleraTemp[j].valor == 1) {
                        puntosEscalera += 15;
                    } else {
                        puntosEscalera += escaleraTemp[j].valor;
                    }
                }
                
                puntosEscalera += huecos * 20;
                maxPuntos += puntosEscalera;
                
                if (maxPuntos >= 30) {
                    free(cartas);
                    return true;
                }
            }
            
            inicioEscalera = finEscalera + 1;
        }
    }
    
    free(cartas);
    
    if (jugador->bcp != NULL) {
        jugador->bcp->intentosFallidos++;
        actualizarBCPJugador(jugador);
    }
    
    return false;
}

bool verificarApeada(Jugador *jugador, Apeada *apeada) {
    if (apeada->esGrupo) {
        Grupo *grupo = &apeada->jugada.grupo;
        
        if (grupo->numCartas >= 4) return false;
        
        int valorGrupo = grupo->cartas[0].valor;
        
        for (int i = 0; i < jugador->mano.numCartas; i++) {
            Carta *carta = &jugador->mano.cartas[i];
            
            if (!carta->esComodin && carta->valor == valorGrupo) {
                bool paloRepetido = false;
                for (int j = 0; j < grupo->numCartas; j++) {
                    if (grupo->cartas[j].palo == carta->palo) {
                        paloRepetido = true;
                        break;
                    }
                }
                
                if (!paloRepetido) return true;
            }
        }
        
        for (int i = 0; i < jugador->mano.numCartas; i++) {
            if (jugador->mano.cartas[i].esComodin) return true;
        }
    } else {
        Escalera *escalera = &apeada->jugada.escalera;
        char paloEscalera = escalera->palo;
        
        if (escalera->numCartas > 0) {
            int valorMinimo = INT_MAX;
            int valorMaximo = INT_MIN;
            
            for (int i = 0; i < escalera->numCartas; i++) {
                if (!escalera->cartas[i].esComodin) {
                    if (escalera->cartas[i].valor < valorMinimo) {
                        valorMinimo = escalera->cartas[i].valor;
                    }
                    if (escalera->cartas[i].valor > valorMaximo) {
                        valorMaximo = escalera->cartas[i].valor;
                    }
                }
            }
            
            if (valorMinimo > 1) {
                for (int i = 0; i < jugador->mano.numCartas; i++) {
                    Carta *carta = &jugador->mano.cartas[i];
                    if (!carta->esComodin && carta->palo == paloEscalera && carta->valor == valorMinimo - 1) {
                        return true;
                    }
                }
            }
            
            if (valorMaximo < 13) {
                for (int i = 0; i < jugador->mano.numCartas; i++) {
                    Carta *carta = &jugador->mano.cartas[i];
                    if (!carta->esComodin && carta->palo == paloEscalera && carta->valor == valorMaximo + 1) {
                        return true;
                    }
                }
            }
            
            for (int i = 0; i < jugador->mano.numCartas; i++) {
                if (jugador->mano.cartas[i].esComodin) return true;
            }
        }
    }
    
    return false;
}

bool realizarJugadaApeada(Jugador *jugador, Apeada *apeada) {
    if (apeada->esGrupo) {
        Grupo *grupo = &apeada->jugada.grupo;
        
        if (grupo->numCartas >= 4) return false;
        
        int valorGrupo = grupo->cartas[0].valor;
        
        for (int i = 0; i < jugador->mano.numCartas; i++) {
            Carta *carta = &jugador->mano.cartas[i];
            
            if (!carta->esComodin && carta->valor == valorGrupo) {
                bool paloRepetido = false;
                for (int j = 0; j < grupo->numCartas; j++) {
                    if (grupo->cartas[j].palo == carta->palo) {
                        paloRepetido = true;
                        break;
                    }
                }
                
                if (!paloRepetido) {
                    grupo->cartas[grupo->numCartas] = *carta;
                    grupo->numCartas++;
                    
                    for (int j = i; j < jugador->mano.numCartas - 1; j++) {
                        jugador->mano.cartas[j] = jugador->mano.cartas[j+1];
                    }
                    jugador->mano.numCartas--;
                    
                    return true;
                }
            }
        }
        
        for (int i = 0; i < jugador->mano.numCartas; i++) {
            if (jugador->mano.cartas[i].esComodin) {
                grupo->cartas[grupo->numCartas] = jugador->mano.cartas[i];
                grupo->numCartas++;
                
                for (int j = i; j < jugador->mano.numCartas - 1; j++) {
                    jugador->mano.cartas[j] = jugador->mano.cartas[j+1];
                }
                jugador->mano.numCartas--;
                
                return true;
            }
        }
    } else {
        // Lógica simplificada para escaleras
        Escalera *escalera = &apeada->jugada.escalera;
        char paloEscalera = escalera->palo;
        
        if (escalera->numCartas > 0) {
            int valorMinimo = INT_MAX;
            int valorMaximo = INT_MIN;
            
            for (int i = 0; i < escalera->numCartas; i++) {
                if (!escalera->cartas[i].esComodin) {
                    if (escalera->cartas[i].valor < valorMinimo) {
                        valorMinimo = escalera->cartas[i].valor;
                    }
                    if (escalera->cartas[i].valor > valorMaximo) {
                        valorMaximo = escalera->cartas[i].valor;
                    }
                }
            }
            
            // Intentar añadir al final
            if (valorMaximo < 13) {
                for (int i = 0; i < jugador->mano.numCartas; i++) {
                    Carta *carta = &jugador->mano.cartas[i];
                    if (!carta->esComodin && carta->palo == paloEscalera && carta->valor == valorMaximo + 1) {
                        if (escalera->numCartas >= escalera->capacidad) {
                            int nuevaCapacidad = escalera->capacidad == 0 ? 13 : escalera->capacidad * 2;
                            escalera->cartas = realloc(escalera->cartas, nuevaCapacidad * sizeof(Carta));
                            
                            if (escalera->cartas == NULL) {
                                printf("Error: No se pudo ampliar la capacidad de la escalera\n");
                                return false;
                            }
                            
                            escalera->capacidad = nuevaCapacidad;
                        }
                        
                        escalera->cartas[escalera->numCartas] = *carta;
                        escalera->numCartas++;
                        
                        for (int j = i; j < jugador->mano.numCartas - 1; j++) {
                            jugador->mano.cartas[j] = jugador->mano.cartas[j+1];
                        }
                        jugador->mano.numCartas--;
                        
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

// ============== CREACIÓN DE APEADAS ==============
Apeada* crearApeada(Jugador *jugador) {
    if (!jugador->primeraApeada && !puedeApearse(jugador)) {
        return NULL;
    }
    
    // Buscar grupos primero
    for (int i = 1; i <= 13; i++) {
        Carta grupoTemp[4];
        int numCartasGrupo = 0;
        int indiceMano[4];
        
        for (int j = 0; j < jugador->mano.numCartas && numCartasGrupo < 4; j++) {
            if (!jugador->mano.cartas[j].esComodin && jugador->mano.cartas[j].valor == i) {
                bool paloRepetido = false;
                for (int k = 0; k < numCartasGrupo; k++) {
                    if (grupoTemp[k].palo == jugador->mano.cartas[j].palo) {
                        paloRepetido = true;
                        break;
                    }
                }
                
                if (!paloRepetido) {
                    grupoTemp[numCartasGrupo] = jugador->mano.cartas[j];
                    indiceMano[numCartasGrupo] = j;
                    numCartasGrupo++;
                }
            }
        }
        
        if (numCartasGrupo >= 3) {
            Apeada *nuevaApeada = (Apeada*)malloc(sizeof(Apeada));
            if (nuevaApeada == NULL) return NULL;
            
            nuevaApeada->esGrupo = true;
            nuevaApeada->jugada.grupo.numCartas = numCartasGrupo;
            nuevaApeada->idJugador = jugador->id;
            
            for (int j = 0; j < numCartasGrupo; j++) {
                nuevaApeada->jugada.grupo.cartas[j] = grupoTemp[j];
            }
            
            nuevaApeada->puntos = 0;
            for (int j = 0; j < numCartasGrupo; j++) {
                if (grupoTemp[j].valor >= 11) {
                    nuevaApeada->puntos += 10;
                } else if (grupoTemp[j].valor == 1) {
                    nuevaApeada->puntos += 15;
                } else {
                    nuevaApeada->puntos += grupoTemp[j].valor;
                }
            }
            
            // Eliminar cartas de la mano
            for (int j = numCartasGrupo - 1; j >= 0; j--) {
                for (int k = indiceMano[j]; k < jugador->mano.numCartas - 1; k++) {
                    jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                }
                jugador->mano.numCartas--;
            }
            
            if (jugador->bcp != NULL) {
                jugador->bcp->vecesApeo++;
                actualizarBCPJugador(jugador);
            }
            
            return nuevaApeada;
        }
        
        // Intentar formar terna con 2 cartas + comodín
        if (numCartasGrupo == 2) {
            int indiceComodin = -1;
            for (int j = 0; j < jugador->mano.numCartas; j++) {
                if (jugador->mano.cartas[j].esComodin) {
                    indiceComodin = j;
                    break;
                }
            }
            
            if (indiceComodin != -1) {
                Apeada *nuevaApeada = (Apeada*)malloc(sizeof(Apeada));
                if (nuevaApeada == NULL) return NULL;
                
                nuevaApeada->esGrupo = true;
                nuevaApeada->jugada.grupo.numCartas = 3;
                nuevaApeada->idJugador = jugador->id;
                
                for (int j = 0; j < 2; j++) {
                    nuevaApeada->jugada.grupo.cartas[j] = grupoTemp[j];
                }
                nuevaApeada->jugada.grupo.cartas[2] = jugador->mano.cartas[indiceComodin];
                
                nuevaApeada->puntos = 20; // Comodín
                for (int j = 0; j < 2; j++) {
                    if (grupoTemp[j].valor >= 11) {
                        nuevaApeada->puntos += 10;
                    } else if (grupoTemp[j].valor == 1) {
                        nuevaApeada->puntos += 15;
                    } else {
                        nuevaApeada->puntos += grupoTemp[j].valor;
                    }
                }
                
                // Eliminar comodín
                for (int k = indiceComodin; k < jugador->mano.numCartas - 1; k++) {
                    jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                }
                jugador->mano.numCartas--;
                
                // Eliminar cartas normales
                for (int j = 1; j >= 0; j--) {
                    int indiceReal = indiceMano[j];
                    if (indiceComodin < indiceReal) indiceReal--;
                    
                    for (int k = indiceReal; k < jugador->mano.numCartas - 1; k++) {
                        jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                    }
                    jugador->mano.numCartas--;
                }
                
                if (jugador->bcp != NULL) {
                    jugador->bcp->vecesApeo++;
                    actualizarBCPJugador(jugador);
                }
                
                return nuevaApeada;
            }
        }
    }
    
    // Buscar escaleras
    for (char palo = 'C'; palo <= 'T'; palo++) {
        Carta escaleraTemp[13];
        int indicesMano[13];
        int numCartasEscalera = 0;
        
        for (int j = 0; j < jugador->mano.numCartas; j++) {
            if (!jugador->mano.cartas[j].esComodin && jugador->mano.cartas[j].palo == palo) {
                escaleraTemp[numCartasEscalera] = jugador->mano.cartas[j];
                indicesMano[numCartasEscalera] = j;
                numCartasEscalera++;
            }
        }
        
        // Ordenar por valor
        for (int i = 0; i < numCartasEscalera - 1; i++) {
            for (int j = 0; j < numCartasEscalera - i - 1; j++) {
                if (escaleraTemp[j].valor > escaleraTemp[j+1].valor) {
                    Carta tempCarta = escaleraTemp[j];
                    escaleraTemp[j] = escaleraTemp[j+1];
                    escaleraTemp[j+1] = tempCarta;
                    
                    int tempIndice = indicesMano[j];
                    indicesMano[j] = indicesMano[j+1];
                    indicesMano[j+1] = tempIndice;
                }
            }
        }
        
        // Buscar secuencias de 3+ cartas
        int inicioEscalera = 0;
        while (inicioEscalera < numCartasEscalera) {
            int finEscalera = inicioEscalera;
            
            while (finEscalera + 1 < numCartasEscalera && 
                   escaleraTemp[finEscalera + 1].valor == escaleraTemp[finEscalera].valor + 1) {
                finEscalera++;
            }
            
            int longitudEscalera = finEscalera - inicioEscalera + 1;
            
            if (longitudEscalera >= 3) {
                Apeada *nuevaApeada = (Apeada*)malloc(sizeof(Apeada));
                if (nuevaApeada == NULL) return NULL;
                
                nuevaApeada->esGrupo = false;
                nuevaApeada->idJugador = jugador->id;
                nuevaApeada->jugada.escalera.capacidad = longitudEscalera;
                nuevaApeada->jugada.escalera.numCartas = longitudEscalera;
                nuevaApeada->jugada.escalera.palo = palo;
                nuevaApeada->jugada.escalera.cartas = (Carta*)malloc(longitudEscalera * sizeof(Carta));
                
                if (nuevaApeada->jugada.escalera.cartas == NULL) {
                    free(nuevaApeada);
                    return NULL;
                }
                
                for (int j = 0; j < longitudEscalera; j++) {
                    nuevaApeada->jugada.escalera.cartas[j] = escaleraTemp[inicioEscalera + j];
                }
                
                nuevaApeada->puntos = 0;
                for (int j = 0; j < longitudEscalera; j++) {
                    if (escaleraTemp[inicioEscalera + j].valor >= 11) {
                        nuevaApeada->puntos += 10;
                    } else if (escaleraTemp[inicioEscalera + j].valor == 1) {
                        nuevaApeada->puntos += 15;
                    } else {
                        nuevaApeada->puntos += escaleraTemp[inicioEscalera + j].valor;
                    }
                }
                
                // Eliminar cartas de la mano (de mayor índice a menor)
                int indicesAEliminar[13];
                for (int j = 0; j < longitudEscalera; j++) {
                    indicesAEliminar[j] = indicesMano[inicioEscalera + j];
                }
                
                // Ordenar índices de mayor a menor
                for (int i = 0; i < longitudEscalera - 1; i++) {
                    for (int j = 0; j < longitudEscalera - i - 1; j++) {
                        if (indicesAEliminar[j] < indicesAEliminar[j+1]) {
                            int temp = indicesAEliminar[j];
                            indicesAEliminar[j] = indicesAEliminar[j+1];
                            indicesAEliminar[j+1] = temp;
                        }
                    }
                }
                
                for (int j = 0; j < longitudEscalera; j++) {
                    int indice = indicesAEliminar[j];
                    for (int k = indice; k < jugador->mano.numCartas - 1; k++) {
                        jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                    }
                    jugador->mano.numCartas--;
                }
                
                if (jugador->bcp != NULL) {
                    jugador->bcp->vecesApeo++;
                    actualizarBCPJugador(jugador);
                }
                
                return nuevaApeada;
            }
            
            inicioEscalera = finEscalera + 1;
        }
    }
    
    return NULL;
}

// ============== OPERACIONES BÁSICAS ==============
bool comerFicha(Jugador *jugador, Mazo *banca) {
    if (banca->numCartas <= 0) return false;
    
    Carta carta = banca->cartas[banca->numCartas - 1];
    banca->numCartas--;
    
    if (jugador->mano.numCartas >= jugador->mano.capacidad) {
        int nuevaCapacidad = jugador->mano.capacidad == 0 ? 10 : jugador->mano.capacidad * 2;
        Carta *nuevasCartas = realloc(jugador->mano.cartas, nuevaCapacidad * sizeof(Carta));
        
        if (nuevasCartas == NULL) {
            banca->numCartas++;
            return false;
        }
        
        jugador->mano.cartas = nuevasCartas;
        jugador->mano.capacidad = nuevaCapacidad;
    }
    
    jugador->mano.cartas[jugador->mano.numCartas] = carta;
    jugador->mano.numCartas++;
    
    char cartaStr[50];
    obtenerNombreCarta(carta, cartaStr);
    printf("Jugador %d comió una ficha: %s\n", jugador->id, cartaStr);
    
    return true;
}

// ============== CONTROL DE ESTADO ==============
void actualizarEstadoJugador(Jugador *jugador, EstadoJugador nuevoEstado) {
    const char *estados[] = {"LISTO", "EJECUCION", "ESPERA_ES", "BLOQUEADO"};
    EstadoJugador estadoAnterior = jugador->estado;
    
    jugador->estado = nuevoEstado;
    actualizarBCPJugador(jugador);
    
    pthread_mutex_lock(&mutexTabla);
    actualizarProcesoEnTabla(jugador->id, nuevoEstado);
    pthread_mutex_unlock(&mutexTabla);
    
    printf("Jugador %d cambió a estado: %s\n", jugador->id, estados[nuevoEstado]);
    registrarEvento("Jugador %d cambió de estado: %s -> %s", 
                   jugador->id, estados[estadoAnterior], estados[nuevoEstado]);
}

void pasarTurno(Jugador *jugador) {
    jugador->turnoActual = false;
    
    if (jugador->estado != ESPERA_ES) {
        actualizarEstadoJugador(jugador, LISTO);
    }
}

void entrarEsperaES(Jugador *jugador) {
    jugador->tiempoES = (rand() % 5000 + 1000);
    actualizarEstadoJugador(jugador, ESPERA_ES);
    
    colorMagenta();
    printf("Jugador %d entró en E/S por %d ms\n", jugador->id, jugador->tiempoES);
    colorReset();
    
    registrarEvento("Jugador %d entró en E/S por %d ms", jugador->id, jugador->tiempoES);
    
    if (jugador->bcp != NULL) {
        jugador->bcp->tiempoES = jugador->tiempoES;
        actualizarBCPJugador(jugador);
    }
    
    // Asignar memoria para E/S
    if (asignarMemoriaES(jugador->id)) {
        colorVerde();
        printf("Jugador %d: Memoria asignada para operación E/S\n", jugador->id);
        colorReset();
    } else {
        colorRojo();
        printf("Jugador %d: Error al asignar memoria para operación E/S\n", jugador->id);
        colorReset();
    }
    
    // Simular accesos a páginas
    for (int i = 0; i < jugador->mano.numCartas && i < 5; i++) {
        int idCarta = i;
        int numPagina = i % PAGINAS_POR_MARCO;
        accederPagina(jugador->id, numPagina, idCarta);
    }
}

void salirEsperaES(Jugador *jugador) {
    jugador->tiempoES = 0;
    actualizarEstadoJugador(jugador, LISTO);
    
    colorMagenta();
    printf("Jugador %d salió de E/S\n", jugador->id);
    colorReset();
    
    registrarEvento("Jugador %d salió de E/S", jugador->id);
    
    if (jugador->bcp != NULL) {
        jugador->bcp->tiempoES = 0;
        actualizarBCPJugador(jugador);
    }
    
    liberarMemoria(jugador->id);
}

void actualizarBCPJugador(Jugador *jugador) {
    if (jugador->bcp != NULL) {
        jugador->bcp->estado = jugador->estado;
        jugador->bcp->tiempoEspera = jugador->tiempoES;
        jugador->bcp->tiempoRestante = jugador->tiempoRestante;
        jugador->bcp->prioridad = jugador->id;
        jugador->bcp->numCartas = jugador->mano.numCartas;
        jugador->bcp->turnoActual = jugador->turnoActual ? 1 : 0;
        
        guardarBCP(jugador->bcp);
    }
}

// ============== LIMPIEZA ==============
void liberarJugador(Jugador *jugador) {
    if (jugador->mano.cartas != NULL) {
        free(jugador->mano.cartas);
        jugador->mano.cartas = NULL;
    }
    
    if (jugador->bcp != NULL) {
        liberarBCP(jugador->bcp);
        jugador->bcp = NULL;
    }
}