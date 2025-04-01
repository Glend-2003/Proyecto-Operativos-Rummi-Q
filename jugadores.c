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

/* Mutex para acceso a recursos compartidos */
pthread_mutex_t mutexApeadas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBanca = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTabla = PTHREAD_MUTEX_INITIALIZER;

/* Inicializa un jugador con sus valores por defecto */
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
    
    /* Inicializar el mazo del jugador */
    jugador->mano.cartas = NULL;
    jugador->mano.numCartas = 0;
    jugador->mano.capacidad = 0;
    
    /* Crear y asociar el BCP */
    jugador->bcp = crearBCP(id);
    
    /* Actualizar el estado inicial en el BCP */
    actualizarBCPJugador(jugador);
}

/* Función principal que ejecutará cada hilo de jugador */
void *funcionHiloJugador(void *arg) {
    Jugador *jugador = (Jugador *)arg;
    
    /* Registrar en tabla de procesos que el hilo ha iniciado */
    pthread_mutex_lock(&mutexTabla);
    registrarProcesoEnTabla(jugador->id, PROC_BLOQUEADO);
    pthread_mutex_unlock(&mutexTabla);
    
    /* Bucle principal del jugador */
    while (!jugador->terminado && !juegoTerminado()) {
        /* Esperar a que sea su turno */
        while (!jugador->turnoActual && !jugador->terminado && !juegoTerminado()) {
            /* El jugador está en espera */
            if (jugador->estado == ESPERA_ES) {
                /* Simular tiempo en E/S */
                if (jugador->tiempoES > 0) {
                    usleep(50000);  /* 50ms */
                    jugador->tiempoES -= 50;
                    
                    /* Actualizar el BCP */
                    actualizarBCPJugador(jugador);
                    
                    /* Si ya terminó su tiempo en E/S */
                    if (jugador->tiempoES <= 0) {
                        salirEsperaES(jugador);
                    }
                }
            } else {
                /* Esperar a que le asignen su turno */
                usleep(100000);  /* 100ms */
            }
        }
        
        /* Si el juego terminó o el jugador terminó, salir del bucle */
        if (jugador->terminado || juegoTerminado()) {
            break;
        }
        
        /* Cambiar estado a EJECUCION */
        actualizarEstadoJugador(jugador, EJECUCION);
        
        /* Obtener referencia a las apeadas y banca */
        Apeada *apeadas = obtenerApeadas();
        int numApeadas = obtenerNumApeadas();
        Mazo *banca = obtenerBanca();
        
        /* Realizar el turno */
        bool turnoCompletado = realizarTurno(jugador, apeadas, numApeadas, banca);
        
        /* Si el jugador no pudo completar su turno en el tiempo asignado */
        if (!turnoCompletado) {
            printf("Jugador %d se quedó sin tiempo en su turno\n", jugador->id);
        }
        
        /* Verificar si el jugador ha terminado sus cartas */
        if (jugador->mano.numCartas == 0 && banca->numCartas == 0) {
            printf("¡Jugador %d ha ganado!\n", jugador->id);
            jugador->terminado = true;
            finalizarJuego(jugador->id);
        }
        
        /* Pasar el turno */
        pasarTurno(jugador);
    }
    
    /* Registrar en tabla de procesos que el hilo ha terminado */
    pthread_mutex_lock(&mutexTabla);
    registrarProcesoEnTabla(jugador->id, PROC_BLOQUEADO);
    pthread_mutex_unlock(&mutexTabla);
    
    return NULL;
}

/* Realizar el turno del jugador */
bool realizarTurno(Jugador *jugador, Apeada *apeadas, int numApeadas, Mazo *banca) {
    int i;
    char cartaStr[50];
    clock_t inicio;
    int tiempoTranscurrido;
    bool turnoCompletado = false;
    bool hizoJugada = false;
    
    printf("\n--- Jugador %d está ejecutando su turno ---\n", jugador->id);
    
    /* Verificar si tiene tiempo suficiente */
    if (jugador->tiempoRestante <= 0) {
        printf("Jugador %d no tiene tiempo suficiente para su turno\n", jugador->id);
        return false;
    }
    
    /* Tiempo de inicio del turno */
    inicio = clock();
    
    /* Mostrar mano actual del jugador */
    printf("Mano del Jugador %d (%d cartas):\n", jugador->id, jugador->mano.numCartas);
    for (i = 0; i < jugador->mano.numCartas; i++) {
        printf("  %d. %s\n", i+1, obtenerNombreCarta(jugador->mano.cartas[i], cartaStr));
    }
    
    /* Intentar realizar jugadas mientras tenga tiempo */
    while ((clock() - inicio) * 1000 / CLOCKS_PER_SEC < jugador->tiempoRestante) {
        /* Si es la primera vez que se apea */
        if (!jugador->primeraApeada) {
            colorAzul();
            printf("Jugador %d intenta hacer su primera apeada (necesita 30+ puntos)\n", jugador->id);
            colorReset();
            
            /* Verificar si puede apearse con 30 puntos o más */
            if (puedeApearse(jugador)) {
                /* Crear una nueva apeada */
                Apeada *nuevaApeada = crearApeada(jugador);
                
                if (nuevaApeada != NULL) {
                    /* Añadir la apeada a la mesa */
                    pthread_mutex_lock(&mutexApeadas);
                    if (agregarApeada(nuevaApeada)) {
                        colorVerde();
                        printf("¡Jugador %d ha realizado su primera apeada!\n", jugador->id);
                        colorReset();
                        jugador->primeraApeada = true;
                        hizoJugada = true;
                        
                        /* Liberar memoria de la nueva apeada (ya está copiada en la mesa) */
                        free(nuevaApeada);
                    } else {
                        colorRojo();
                        printf("Error: No se pudo agregar la apeada a la mesa\n");
                        colorReset();
                        
                        /* Aquí habría que devolver las cartas al jugador, pero por simplicidad no lo hacemos */
                        free(nuevaApeada);
                    }
                    pthread_mutex_unlock(&mutexApeadas);
                } else {
                    colorRojo();
                    printf("Jugador %d no pudo formar una apeada con 30+ puntos\n", jugador->id);
                    colorReset();
                    
                    /* Actualizar BCP con intento fallido */
                    if (jugador->bcp != NULL) {
                        jugador->bcp->intentosFallidos++;
                        actualizarBCPJugador(jugador);
                    }
                }
            } else {
                colorAmarillo();
                printf("Jugador %d no tiene cartas para hacer su primera apeada\n", jugador->id);
                colorReset();
            }
        } else {
            /* Ya se apeó anteriormente, buscar jugadas posibles */
            colorAzul();
            printf("Jugador %d busca jugadas en las apeadas existentes\n", jugador->id);
            colorReset();
            
            /* Mutex para acceder a las apeadas */
            pthread_mutex_lock(&mutexApeadas);
            
            /* Buscar en cada apeada si puede hacer embones o modificar */
            bool hizoBusqueda = false;
            for (i = 0; i < numApeadas; i++) {
                /* Verificar si puede hacer jugada en esta apeada */
                if (verificarApeada(jugador, &apeadas[i])) {
                    colorVerde();
                    printf("Jugador %d puede modificar la apeada %d\n", jugador->id, i);
                    colorReset();
                    
                    /* Realizar jugada en esta apeada */
                    if (realizarJugadaApeada(jugador, &apeadas[i])) {
                        colorVerde();
                        printf("¡Jugador %d ha realizado una jugada en la apeada %d!\n", jugador->id, i);
                        colorReset();
                        hizoJugada = true;
                        hizoBusqueda = true;
                        break;
                    }
                }
            }
            
            /* Si no encontró ninguna apeada que modificar, intentar crear una nueva */
            if (!hizoBusqueda && jugador->mano.numCartas > 0) {
                colorAzul();
                printf("Jugador %d intenta crear una nueva apeada\n", jugador->id);
                colorReset();
                
                /* Crear una nueva apeada */
                Apeada *nuevaApeada = crearApeada(jugador);
                
                if (nuevaApeada != NULL) {
                    /* Añadir la apeada a la mesa */
                    if (agregarApeada(nuevaApeada)) {
                        colorVerde();
                        printf("¡Jugador %d ha creado una nueva apeada!\n", jugador->id);
                        colorReset();
                        hizoJugada = true;
                        
                        /* Liberar memoria de la nueva apeada (ya está copiada en la mesa) */
                        free(nuevaApeada);
                    } else {
                        colorRojo();
                        printf("Error: No se pudo agregar la apeada a la mesa\n");
                        colorReset();
                        
                        /* Aquí habría que devolver las cartas al jugador, pero por simplicidad no lo hacemos */
                        free(nuevaApeada);
                    }
                }
            }
            
            pthread_mutex_unlock(&mutexApeadas);
        }
        
        /* Si no pudo hacer ninguna jugada, comer ficha si hay disponibles */
        if (!hizoJugada) {
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
                    
                    /* Actualizar BCP */
                    if (jugador->bcp != NULL) {
                        jugador->bcp->cartasComidas++;
                        actualizarBCPJugador(jugador);
                    }
                    
                    /* Entrar en estado de E/S después de comer */
                    pthread_mutex_unlock(&mutexBanca);
                    entrarEsperaES(jugador);
                    turnoCompletado = true;
                    break;
                }
            } else {
                /* No hay fichas para comer y no puede hacer jugada */
                colorRojo();
                printf("Jugador %d no puede hacer jugada y no hay fichas para comer\n", jugador->id);
                colorReset();
            }
            
            pthread_mutex_unlock(&mutexBanca);
            
            /* Si no pudo hacer jugada ni comer, terminar el turno */
            turnoCompletado = true;
            break;
        }
        
        /* Si hizo alguna jugada, verificar si terminó sus cartas */
        if (hizoJugada) {
            if (jugador->mano.numCartas == 0) {
                colorVerde();
                printf("¡Jugador %d se ha quedado sin cartas!\n", jugador->id);
                colorReset();
                turnoCompletado = true;
                break;
            }
            
            /* Intentar otra jugada */
            hizoJugada = false;
        } else {
            /* Si no pudo hacer jugada, terminar el turno */
            colorAmarillo();
            printf("Jugador %d no pudo hacer ninguna jugada, fin del turno\n", jugador->id);
            colorReset();
            turnoCompletado = true;
            break;
        }
        
        /* Pequeña pausa para no consumir demasiado CPU */
        usleep(10000);  /* 10ms */
    }
    
    /* Actualizar tiempo restante */
    tiempoTranscurrido = (clock() - inicio) * 1000 / CLOCKS_PER_SEC;
    jugador->tiempoRestante -= tiempoTranscurrido;
    
    /* Si el tiempo llegó a 0, actualizar BCP con turno perdido */
    if (jugador->tiempoRestante <= 0) {
        jugador->tiempoRestante = 0;
        colorAmarillo();
        printf("Jugador %d se quedó sin tiempo en su turno\n", jugador->id);
        colorReset();
        
        /* Actualizar BCP */
        if (jugador->bcp != NULL) {
            jugador->bcp->turnosPerdidos++;
            actualizarBCPJugador(jugador);
        }
    }
    
    /* Mostrar mano final del jugador */
    printf("Mano final del Jugador %d (%d cartas):\n", jugador->id, jugador->mano.numCartas);
    for (i = 0; i < jugador->mano.numCartas; i++) {
        printf("  %d. %s\n", i+1, obtenerNombreCarta(jugador->mano.cartas[i], cartaStr));
    }
    
    printf("--- Fin del turno del Jugador %d (tiempo restante: %d ms) ---\n", 
           jugador->id, jugador->tiempoRestante);
    
    return turnoCompletado;
}

/* Verificar si el jugador puede hacer su primera apeada */
bool puedeApearse(Jugador *jugador) {
    /* Para apearse por primera vez, se necesitan 30 puntos o más */
    Carta *cartas;
    int maxPuntos = 0;
    int i, j, k;
    Carta grupo[4];
    int numCartasGrupo;
    int puntosGrupo;
    bool paloRepetido;
    int comodinesDisponibles;
    Carta escaleraTemp[13];
    int numCartasEscalera;
    int inicioEscalera, finEscalera;
    int longitudEscalera;
    int huecos;
    int longitudFinal;
    int puntosEscalera;
    
    /* Verificar si ya se ha apeado antes */
    if (jugador->primeraApeada) {
        return true;  /* Ya se ha apeado antes, no necesita 30 puntos */
    }
    
    /* Ordenar cartas por palo y valor para facilitar la búsqueda */
    cartas = (Carta*)malloc(jugador->mano.numCartas * sizeof(Carta));
    if (cartas == NULL) {
        printf("Error: No se pudo asignar memoria para las cartas\n");
        return false;
    }
    
    memcpy(cartas, jugador->mano.cartas, jugador->mano.numCartas * sizeof(Carta));
    
    /* Ordenar por palo y luego por valor */
    for (i = 0; i < jugador->mano.numCartas - 1; i++) {
        for (j = 0; j < jugador->mano.numCartas - i - 1; j++) {
            if (cartas[j].palo > cartas[j+1].palo || 
                (cartas[j].palo == cartas[j+1].palo && cartas[j].valor > cartas[j+1].valor)) {
                Carta temp = cartas[j];
                cartas[j] = cartas[j+1];
                cartas[j+1] = temp;
            }
        }
    }
    
    /* Buscar grupos (ternas o cuaternas) */
    for (i = 1; i <= 13; i++) {  /* Valores del 1 al 13 */
        numCartasGrupo = 0;
        puntosGrupo = 0;
        
        for (j = 0; j < jugador->mano.numCartas && numCartasGrupo < 4; j++) {
            if (!cartas[j].esComodin && cartas[j].valor == i) {
                /* Verificar que no haya dos cartas del mismo palo */
                paloRepetido = false;
                for (k = 0; k < numCartasGrupo; k++) {
                    if (grupo[k].palo == cartas[j].palo) {
                        paloRepetido = true;
                        break;
                    }
                }
                
                if (!paloRepetido) {
                    grupo[numCartasGrupo] = cartas[j];
                    numCartasGrupo++;
                    
                    /* Calcular puntos */
                    if (cartas[j].valor >= 11) {  /* J, Q, K */
                        puntosGrupo += 10;
                    } else if (cartas[j].valor == 1) {  /* As */
                        puntosGrupo += 15;
                    } else {
                        puntosGrupo += cartas[j].valor;
                    }
                }
            }
        }
        
        /* Añadir comodines si es necesario para completar una terna */
        comodinesDisponibles = 0;
        for (j = 0; j < jugador->mano.numCartas; j++) {
            if (cartas[j].esComodin) {
                comodinesDisponibles++;
            }
        }
        
        /* Si tenemos 2 cartas y un comodín, podemos formar una terna */
        if (numCartasGrupo == 2 && comodinesDisponibles >= 1) {
            numCartasGrupo++;
            puntosGrupo += 20;  /* Valor del comodín */
            comodinesDisponibles--;
        }
        
        /* Verificar si es un grupo válido (terna o cuaterna) */
        if (numCartasGrupo >= 3) {
            maxPuntos += puntosGrupo;
            
            /* Si ya tenemos 30 puntos o más, podemos apearnos */
            if (maxPuntos >= 30) {
                free(cartas);
                return true;
            }
        }
    }
    
    /* Buscar escaleras (tres o más cartas consecutivas del mismo palo) */
    for (char palo = 'C'; palo <= 'T'; palo++) {
        /* Seleccionar cartas del mismo palo */
        numCartasEscalera = 0;
        
        for (j = 0; j < jugador->mano.numCartas; j++) {
            if (!cartas[j].esComodin && cartas[j].palo == palo) {
                escaleraTemp[numCartasEscalera] = cartas[j];
                numCartasEscalera++;
            }
        }
        
        /* Ordenar por valor */
        for (i = 0; i < numCartasEscalera - 1; i++) {
            for (j = 0; j < numCartasEscalera - i - 1; j++) {
                if (escaleraTemp[j].valor > escaleraTemp[j+1].valor) {
                    Carta temp = escaleraTemp[j];
                    escaleraTemp[j] = escaleraTemp[j+1];
                    escaleraTemp[j+1] = temp;
                }
            }
        }
        
        /* Buscar secuencias */
        inicioEscalera = 0;
        while (inicioEscalera < numCartasEscalera) {
            finEscalera = inicioEscalera;
            
            /* Encontrar el final de la secuencia */
            while (finEscalera + 1 < numCartasEscalera && 
                   escaleraTemp[finEscalera + 1].valor == escaleraTemp[finEscalera].valor + 1) {
                finEscalera++;
            }
            
            /* Verificar si es una escalera válida (3 o más cartas) */
            longitudEscalera = finEscalera - inicioEscalera + 1;
            
            /* Ver si podemos completar con comodines */
            comodinesDisponibles = 0;
            for (j = 0; j < jugador->mano.numCartas; j++) {
                if (cartas[j].esComodin) {
                    comodinesDisponibles++;
                }
            }
            
            /* Calcular huecos en la escalera */
            huecos = 0;
            for (j = inicioEscalera; j < finEscalera; j++) {
                huecos += escaleraTemp[j+1].valor - escaleraTemp[j].valor - 1;
            }
            
            /* Verificar si podemos llenar los huecos con comodines */
            longitudFinal = longitudEscalera;
            if (huecos <= comodinesDisponibles) {
                longitudFinal += huecos;
                comodinesDisponibles -= huecos;
            }
            
            /* Si la escalera es válida (3 o más cartas), calcular puntos */
            if (longitudFinal >= 3) {
                puntosEscalera = 0;
                
                /* Calcular puntos de la escalera */
                for (j = inicioEscalera; j <= finEscalera; j++) {
                    if (escaleraTemp[j].valor >= 11) {  /* J, Q, K */
                        puntosEscalera += 10;
                    } else if (escaleraTemp[j].valor == 1) {  /* As */
                        puntosEscalera += 15;
                    } else {
                        puntosEscalera += escaleraTemp[j].valor;
                    }
                }
                
                /* Añadir puntos de los comodines */
                puntosEscalera += huecos * 20;
                
                maxPuntos += puntosEscalera;
                
                /* Si ya tenemos 30 puntos o más, podemos apearnos */
                if (maxPuntos >= 30) {
                    free(cartas);
                    return true;
                }
            }
            
            /* Continuar con la siguiente secuencia */
            inicioEscalera = finEscalera + 1;
        }
    }
    
    free(cartas);
    
    /* Actualizar BCP con intento fallido */
    if (jugador->bcp != NULL) {
        jugador->bcp->intentosFallidos++;
        actualizarBCPJugador(jugador);
    }
    
    return false;  /* No se encontraron combinaciones que sumen 30 puntos o más */
}

/* Verificar si una apeada puede ser modificada por un jugador */
bool verificarApeada(Jugador *jugador, Apeada *apeada) {
    int i, j;
    int valorGrupo;
    char paloEscalera;
    int valorMinimo, valorMaximo;
    Carta *carta;
    bool paloRepetido;
    Grupo *grupo;
    Escalera *escalera;
    
    /* Esta función verifica si el jugador puede modificar la apeada */
    
    if (apeada->esGrupo) {
        /* Es un grupo (terna o cuaterna) */
        grupo = &apeada->jugada.grupo;
        
        /* Si ya es una cuaterna, no se puede modificar */
        if (grupo->numCartas >= 4) {
            return false;
        }
        
        /* Buscar si el jugador tiene una carta del mismo valor y diferente palo */
        valorGrupo = grupo->cartas[0].valor;
        
        for (i = 0; i < jugador->mano.numCartas; i++) {
            carta = &jugador->mano.cartas[i];
            
            /* Verificar si es del mismo valor */
            if (!carta->esComodin && carta->valor == valorGrupo) {
                /* Verificar que no haya una carta del mismo palo en el grupo */
                paloRepetido = false;
                for (j = 0; j < grupo->numCartas; j++) {
                    if (grupo->cartas[j].palo == carta->palo) {
                        paloRepetido = true;
                        break;
                    }
                }
                
                if (!paloRepetido) {
                    return true;  /* Puede añadir esta carta al grupo */
                }
            }
        }
        
        /* Verificar si tiene un comodín */
        for (i = 0; i < jugador->mano.numCartas; i++) {
            if (jugador->mano.cartas[i].esComodin) {
                return true;  /* Puede añadir un comodín al grupo */
            }
        }
        
    } else {
        /* Es una escalera */
        escalera = &apeada->jugada.escalera;
        paloEscalera = escalera->palo;
        
        /* Verificar si puede añadir cartas al principio o al final de la escalera */
        if (escalera->numCartas > 0) {
            valorMinimo = INT_MAX;
            valorMaximo = INT_MIN;
            
            /* Encontrar los valores mínimo y máximo de la escalera */
            for (i = 0; i < escalera->numCartas; i++) {
                if (!escalera->cartas[i].esComodin) {
                    if (escalera->cartas[i].valor < valorMinimo) {
                        valorMinimo = escalera->cartas[i].valor;
                    }
                    if (escalera->cartas[i].valor > valorMaximo) {
                        valorMaximo = escalera->cartas[i].valor;
                    }
                }
            }
            
            /* Verificar si tiene una carta que pueda añadir al principio (valor - 1) */
            if (valorMinimo > 1) {  /* No se puede añadir antes del As (1) */
                for (i = 0; i < jugador->mano.numCartas; i++) {
                    carta = &jugador->mano.cartas[i];
                    if (!carta->esComodin && carta->palo == paloEscalera && carta->valor == valorMinimo - 1) {
                        return true;  /* Puede añadir esta carta al principio */
                    }
                }
            }
            
            /* Verificar si tiene una carta que pueda añadir al final (valor + 1) */
            if (valorMaximo < 13) {  /* No se puede añadir después del Rey (13) */
                for (i = 0; i < jugador->mano.numCartas; i++) {
                    carta = &jugador->mano.cartas[i];
                    if (!carta->esComodin && carta->palo == paloEscalera && carta->valor == valorMaximo + 1) {
                        return true;  /* Puede añadir esta carta al final */
                    }
                }
            }
            
            /* Verificar si tiene un comodín */
            for (i = 0; i < jugador->mano.numCartas; i++) {
                if (jugador->mano.cartas[i].esComodin) {
                    return true;  /* Puede añadir un comodín a la escalera */
                }
            }
        }
    }
    
    return false;  /* No puede modificar la apeada */
}

/* Realizar una jugada en una apeada */
bool realizarJugadaApeada(Jugador *jugador, Apeada *apeada) {
    int i, j;
    int valorGrupo;
    Carta *carta;
    bool paloRepetido;
    Grupo *grupo;
    Escalera *escalera;
    char paloEscalera;
    int valorMinimo = INT_MAX;
    int valorMaximo = INT_MIN;
    int nuevaCapacidad;
    
    /* Esta función realiza la jugada en la apeada seleccionada */
    
    if (apeada->esGrupo) {
        /* Es un grupo (terna o cuaterna) */
        grupo = &apeada->jugada.grupo;
        
        /* Si ya es una cuaterna, no se puede modificar */
        if (grupo->numCartas >= 4) {
            return false;
        }
        
        /* Buscar si el jugador tiene una carta del mismo valor y diferente palo */
        valorGrupo = grupo->cartas[0].valor;
        
        for (i = 0; i < jugador->mano.numCartas; i++) {
            carta = &jugador->mano.cartas[i];
            
            /* Verificar si es del mismo valor */
            if (!carta->esComodin && carta->valor == valorGrupo) {
                /* Verificar que no haya una carta del mismo palo en el grupo */
                paloRepetido = false;
                for (j = 0; j < grupo->numCartas; j++) {
                    if (grupo->cartas[j].palo == carta->palo) {
                        paloRepetido = true;
                        break;
                    }
                }
                
                if (!paloRepetido) {
                    /* Añadir la carta al grupo */
                    grupo->cartas[grupo->numCartas] = *carta;
                    grupo->numCartas++;
                    
                    /* Eliminar la carta de la mano del jugador */
                    for (j = i; j < jugador->mano.numCartas - 1; j++) {
                        jugador->mano.cartas[j] = jugador->mano.cartas[j+1];
                    }
                    jugador->mano.numCartas--;
                    
                    return true;
                }
            }
        }
        
        /* Verificar si tiene un comodín */
        for (i = 0; i < jugador->mano.numCartas; i++) {
            if (jugador->mano.cartas[i].esComodin) {
                /* Añadir el comodín al grupo */
                grupo->cartas[grupo->numCartas] = jugador->mano.cartas[i];
                grupo->numCartas++;
                
                /* Eliminar el comodín de la mano del jugador */
                for (j = i; j < jugador->mano.numCartas - 1; j++) {
                    jugador->mano.cartas[j] = jugador->mano.cartas[j+1];
                }
                jugador->mano.numCartas--;
                
                return true;
            }
        }
        
    } else {
        /* Es una escalera */
        escalera = &apeada->jugada.escalera;
        paloEscalera = escalera->palo;
        
        /* Verificar si puede añadir cartas al principio o al final de la escalera */
        if (escalera->numCartas > 0) {
            /* Encontrar los valores mínimo y máximo de la escalera */
            for (i = 0; i < escalera->numCartas; i++) {
                if (!escalera->cartas[i].esComodin) {
                    if (escalera->cartas[i].valor < valorMinimo) {
                        valorMinimo = escalera->cartas[i].valor;
                    }
                    if (escalera->cartas[i].valor > valorMaximo) {
                        valorMaximo = escalera->cartas[i].valor;
                    }
                }
            }
            
            /* Verificar si tiene una carta que pueda añadir al principio (valor - 1) */
            if (valorMinimo > 1) {  /* No se puede añadir antes del As (1) */
                for (i = 0; i < jugador->mano.numCartas; i++) {
                    carta = &jugador->mano.cartas[i];
                    if (!carta->esComodin && carta->palo == paloEscalera && carta->valor == valorMinimo - 1) {
                        /* Añadir la carta al principio de la escalera */
                        if (escalera->numCartas >= escalera->capacidad) {
                            /* Ampliar capacidad si es necesario */
                            nuevaCapacidad = escalera->capacidad == 0 ? 13 : escalera->capacidad * 2;
                            escalera->cartas = realloc(escalera->cartas, nuevaCapacidad * sizeof(Carta));
                            
                            if (escalera->cartas == NULL) {
                                printf("Error: No se pudo ampliar la capacidad de la escalera\n");
                                return false;
                            }
                            
                            escalera->capacidad = nuevaCapacidad;
                        }
                        
                        /* Desplazar todas las cartas una posición */
                        for (j = escalera->numCartas; j > 0; j--) {
                            escalera->cartas[j] = escalera->cartas[j-1];
                        }
                        
                        /* Añadir la nueva carta al principio */
                        escalera->cartas[0] = *carta;
                        escalera->numCartas++;
                        
                        /* Eliminar la carta de la mano del jugador */
                        for (j = i; j < jugador->mano.numCartas - 1; j++) {
                            jugador->mano.cartas[j] = jugador->mano.cartas[j+1];
                        }
                        jugador->mano.numCartas--;
                        
                        return true;
                    }
                }
            }
            
            /* Verificar si tiene una carta que pueda añadir al final (valor + 1) */
            if (valorMaximo < 13) {  /* No se puede añadir después del Rey (13) */
                for (i = 0; i < jugador->mano.numCartas; i++) {
                    carta = &jugador->mano.cartas[i];
                    if (!carta->esComodin && carta->palo == paloEscalera && carta->valor == valorMaximo + 1) {
                        /* Añadir la carta al final de la escalera */
                        if (escalera->numCartas >= escalera->capacidad) {
                            /* Ampliar capacidad si es necesario */
                            nuevaCapacidad = escalera->capacidad == 0 ? 13 : escalera->capacidad * 2;
                            escalera->cartas = realloc(escalera->cartas, nuevaCapacidad * sizeof(Carta));
                            
                            if (escalera->cartas == NULL) {
                                printf("Error: No se pudo ampliar la capacidad de la escalera\n");
                                return false;
                            }
                            
                            escalera->capacidad = nuevaCapacidad;
                        }
                        
                        /* Añadir la nueva carta al final */
                        escalera->cartas[escalera->numCartas] = *carta;
                        escalera->numCartas++;
                        
                        /* Eliminar la carta de la mano del jugador */
                        for (j = i; j < jugador->mano.numCartas - 1; j++) {
                            jugador->mano.cartas[j] = jugador->mano.cartas[j+1];
                        }
                        jugador->mano.numCartas--;
                        
                        return true;
                    }
                }
            }
            
            /* Verificar si tiene un comodín */
            for (i = 0; i < jugador->mano.numCartas; i++) {
                if (jugador->mano.cartas[i].esComodin) {
                    /* Añadir el comodín al final de la escalera */
                    if (escalera->numCartas >= escalera->capacidad) {
                        /* Ampliar capacidad si es necesario */
                        nuevaCapacidad = escalera->capacidad == 0 ? 13 : escalera->capacidad * 2;
                        escalera->cartas = realloc(escalera->cartas, nuevaCapacidad * sizeof(Carta));
                        
                        if (escalera->cartas == NULL) {
                            printf("Error: No se pudo ampliar la capacidad de la escalera\n");
                            return false;
                        }
                        
                        escalera->capacidad = nuevaCapacidad;
                    }
                    
                    /* Añadir el comodín al final (podría ser al principio también) */
                    escalera->cartas[escalera->numCartas] = jugador->mano.cartas[i];
                    escalera->numCartas++;
                    
                    /* Eliminar el comodín de la mano del jugador */
                    for (j = i; j < jugador->mano.numCartas - 1; j++) {
                        jugador->mano.cartas[j] = jugador->mano.cartas[j+1];
                    }
                    jugador->mano.numCartas--;
                    
                    return true;
                }
            }
        }
    }
    
    return false;  /* No se pudo realizar ninguna jugada */
}

/* Crear una nueva apeada a partir de las cartas del jugador */
Apeada* crearApeada(Jugador *jugador) {
    int i, j, k;
    Carta grupoTemp[4];
    int numCartasGrupo;
    int indiceMano[4];
    int indiceComodin;
    int indiceReal;
    Apeada *nuevaApeada;
    Carta escaleraTemp[13];
    int numCartasEscalera;
    int indicesMano[13];
    int inicioEscalera, finEscalera;
    int longitudEscalera;
    int indicesAEliminar[13];
    int temp;
    int huecos;
    int comodinesDisponibles;
    int indicesComodines[4];
    int longitudFinal;
    int indiceEscalera;
    int comodinesUsados;
    int huecoActual;
    int indice;
    bool paloRepetido = false;
    
    /* Esta función crea una nueva apeada a partir de las cartas del jugador */
    
    /* Verificar si el jugador puede apearse */
    if (!jugador->primeraApeada && !puedeApearse(jugador)) {
        return NULL;  /* No puede apearse por primera vez (menos de 30 puntos) */
    }
    
    /* Buscar grupos (ternas o cuaternas) */
    for (i = 1; i <= 13; i++) {  /* Valores del 1 al 13 */
        /* Buscar cartas del mismo valor */
        numCartasGrupo = 0;
        
        for (j = 0; j < jugador->mano.numCartas && numCartasGrupo < 4; j++) {
            if (!jugador->mano.cartas[j].esComodin && jugador->mano.cartas[j].valor == i) {
                /* Verificar que no haya dos cartas del mismo palo */
                paloRepetido = false;
                for (k = 0; k < numCartasGrupo; k++) {
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
        
        /* Si encontramos 3 o 4 cartas, podemos formar un grupo válido */
        if (numCartasGrupo >= 3) {
            nuevaApeada = (Apeada*)malloc(sizeof(Apeada));
            if (nuevaApeada == NULL) {
                printf("Error: No se pudo asignar memoria para la apeada\n");
                return NULL;
            }
            
            nuevaApeada->esGrupo = true;
            nuevaApeada->jugada.grupo.numCartas = numCartasGrupo;
            nuevaApeada->idJugador = jugador->id;
            
            /* Copiar las cartas al grupo */
            for (j = 0; j < numCartasGrupo; j++) {
                nuevaApeada->jugada.grupo.cartas[j] = grupoTemp[j];
            }
            
            /* Calcular puntos */
            nuevaApeada->puntos = 0;
            for (j = 0; j < numCartasGrupo; j++) {
                if (grupoTemp[j].valor >= 11) {  /* J, Q, K */
                    nuevaApeada->puntos += 10;
                } else if (grupoTemp[j].valor == 1) {  /* As */
                    nuevaApeada->puntos += 15;
                } else {
                    nuevaApeada->puntos += grupoTemp[j].valor;
                }
            }
            
            /* Eliminar las cartas de la mano del jugador (de atrás hacia adelante) */
            for (j = numCartasGrupo - 1; j >= 0; j--) {
                for (k = indiceMano[j]; k < jugador->mano.numCartas - 1; k++) {
                    jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                }
                jugador->mano.numCartas--;
            }
            
            /* Actualizar BCP */
            if (jugador->bcp != NULL) {
                jugador->bcp->vecesApeo++;
                actualizarBCPJugador(jugador);
            }
            
            return nuevaApeada;
        }
        
        /* Intentar formar una terna con 2 cartas + 1 comodín */
        if (numCartasGrupo == 2) {
            /* Buscar un comodín */
            indiceComodin = -1;
            for (j = 0; j < jugador->mano.numCartas; j++) {
                if (jugador->mano.cartas[j].esComodin) {
                    indiceComodin = j;
                    break;
                }
            }
            
            if (indiceComodin != -1) {
                nuevaApeada = (Apeada*)malloc(sizeof(Apeada));
                if (nuevaApeada == NULL) {
                    printf("Error: No se pudo asignar memoria para la apeada\n");
                    return NULL;
                }
                
                nuevaApeada->esGrupo = true;
                nuevaApeada->jugada.grupo.numCartas = 3;
                nuevaApeada->idJugador = jugador->id;
                
                /* Copiar las 2 cartas normales */
                for (j = 0; j < 2; j++) {
                    nuevaApeada->jugada.grupo.cartas[j] = grupoTemp[j];
                }
                
                /* Añadir el comodín */
                nuevaApeada->jugada.grupo.cartas[2] = jugador->mano.cartas[indiceComodin];
                
                /* Calcular puntos */
                nuevaApeada->puntos = 0;
                for (j = 0; j < 2; j++) {
                    if (grupoTemp[j].valor >= 11) {  /* J, Q, K */
                        nuevaApeada->puntos += 10;
                    } else if (grupoTemp[j].valor == 1) {  /* As */
                        nuevaApeada->puntos += 15;
                    } else {
                        nuevaApeada->puntos += grupoTemp[j].valor;
                    }
                }
                nuevaApeada->puntos += 20;  /* Valor del comodín */
                
                /* Eliminar las cartas de la mano del jugador (de atrás hacia adelante) */
                /* Primero el comodín */
                for (k = indiceComodin; k < jugador->mano.numCartas - 1; k++) {
                    jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                }
                jugador->mano.numCartas--;
                
                /* Luego las cartas normales (de atrás hacia adelante) */
                for (j = 1; j >= 0; j--) {
                    /* Ajustar el índice si el comodín estaba antes */
                    indiceReal = indiceMano[j];
                    if (indiceComodin < indiceReal) {
                        indiceReal--;
                    }
                    
                    for (k = indiceReal; k < jugador->mano.numCartas - 1; k++) {
                        jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                    }
                    jugador->mano.numCartas--;
                }
                
                /* Actualizar BCP */
                if (jugador->bcp != NULL) {
                    jugador->bcp->vecesApeo++;
                    actualizarBCPJugador(jugador);
                }
                
                return nuevaApeada;
            }
        }
    }
    
    /* Buscar escaleras (tres o más cartas consecutivas del mismo palo) */
    for (char palo = 'C'; palo <= 'T'; palo++) {
        /* Seleccionar cartas del mismo palo */
        numCartasEscalera = 0;
        
        for (j = 0; j < jugador->mano.numCartas; j++) {
            if (!jugador->mano.cartas[j].esComodin && jugador->mano.cartas[j].palo == palo) {
                escaleraTemp[numCartasEscalera] = jugador->mano.cartas[j];
                indicesMano[numCartasEscalera] = j;
                numCartasEscalera++;
            }
        }
        
        /* Ordenar por valor */
        for (i = 0; i < numCartasEscalera - 1; i++) {
            for (j = 0; j < numCartasEscalera - i - 1; j++) {
                if (escaleraTemp[j].valor > escaleraTemp[j+1].valor) {
                    /* Intercambiar cartas */
                    Carta tempCarta = escaleraTemp[j];
                    escaleraTemp[j] = escaleraTemp[j+1];
                    escaleraTemp[j+1] = tempCarta;
                    
                    /* Intercambiar índices */
                    int tempIndice = indicesMano[j];
                    indicesMano[j] = indicesMano[j+1];
                    indicesMano[j+1] = tempIndice;
                }
            }
        }
        
        /* Buscar secuencias */
        inicioEscalera = 0;
        while (inicioEscalera < numCartasEscalera) {
            finEscalera = inicioEscalera;
            
            /* Encontrar el final de la secuencia */
            while (finEscalera + 1 < numCartasEscalera && 
                   escaleraTemp[finEscalera + 1].valor == escaleraTemp[finEscalera].valor + 1) {
                finEscalera++;
            }
            
            /* Verificar si es una escalera válida (3 o más cartas) */
            longitudEscalera = finEscalera - inicioEscalera + 1;
            
            /* Si la escalera tiene al menos 3 cartas, es válida */
            if (longitudEscalera >= 3) {
                nuevaApeada = (Apeada*)malloc(sizeof(Apeada));
                if (nuevaApeada == NULL) {
                    printf("Error: No se pudo asignar memoria para la apeada\n");
                    return NULL;
                }
                
                nuevaApeada->esGrupo = false;
                nuevaApeada->idJugador = jugador->id;
                
                /* Crear la escalera */
                nuevaApeada->jugada.escalera.capacidad = longitudEscalera;
                nuevaApeada->jugada.escalera.numCartas = longitudEscalera;
                nuevaApeada->jugada.escalera.palo = palo;
                nuevaApeada->jugada.escalera.cartas = (Carta*)malloc(longitudEscalera * sizeof(Carta));
                
                if (nuevaApeada->jugada.escalera.cartas == NULL) {
                    printf("Error: No se pudo asignar memoria para las cartas de la escalera\n");
                    free(nuevaApeada);
                    return NULL;
                }
                
                /* Copiar las cartas a la escalera */
                for (j = 0; j < longitudEscalera; j++) {
                    nuevaApeada->jugada.escalera.cartas[j] = escaleraTemp[inicioEscalera + j];
                }
                
                /* Calcular puntos */
                nuevaApeada->puntos = 0;
                for (j = 0; j < longitudEscalera; j++) {
                    if (escaleraTemp[inicioEscalera + j].valor >= 11) {  /* J, Q, K */
                        nuevaApeada->puntos += 10;
                    } else if (escaleraTemp[inicioEscalera + j].valor == 1) {  /* As */
                        nuevaApeada->puntos += 15;
                    } else {
                        nuevaApeada->puntos += escaleraTemp[inicioEscalera + j].valor;
                    }
                }
                
                /* Eliminar las cartas de la mano del jugador (de atrás hacia adelante) */
                for (j = 0; j < longitudEscalera; j++) {
                    indicesAEliminar[j] = indicesMano[inicioEscalera + j];
                }
                
                /* Ordenar índices de mayor a menor para eliminar correctamente */
                for (i = 0; i < longitudEscalera - 1; i++) {
                    for (j = 0; j < longitudEscalera - i - 1; j++) {
                        if (indicesAEliminar[j] < indicesAEliminar[j+1]) {
                            temp = indicesAEliminar[j];
                            indicesAEliminar[j] = indicesAEliminar[j+1];
                            indicesAEliminar[j+1] = temp;
                        }
                    }
                }
                
                /* Eliminar las cartas */
                for (j = 0; j < longitudEscalera; j++) {
                    indice = indicesAEliminar[j];
                    for (k = indice; k < jugador->mano.numCartas - 1; k++) {
                        jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                    }
                    jugador->mano.numCartas--;
                }
                
                /* Actualizar BCP */
                if (jugador->bcp != NULL) {
                    jugador->bcp->vecesApeo++;
                    actualizarBCPJugador(jugador);
                }
                
                return nuevaApeada;
            }
            
            /* Intentar completar la escalera con comodines */
            huecos = 0;
            
            /* Calcular huecos entre cartas */
            for (j = inicioEscalera; j < finEscalera; j++) {
                huecos += escaleraTemp[j+1].valor - escaleraTemp[j].valor - 1;
            }
            
            /* Verificar si hay comodines disponibles para llenar los huecos */
            comodinesDisponibles = 0;
            
            for (j = 0; j < jugador->mano.numCartas; j++) {
                if (jugador->mano.cartas[j].esComodin && comodinesDisponibles < 4) {
                    indicesComodines[comodinesDisponibles] = j;
                    comodinesDisponibles++;
                }
            }
            
            /* Si tenemos suficientes comodines para hacer una escalera válida */
            if (longitudEscalera + comodinesDisponibles >= 3 && huecos <= comodinesDisponibles) {
                longitudFinal = longitudEscalera + huecos;
                
                nuevaApeada = (Apeada*)malloc(sizeof(Apeada));
                if (nuevaApeada == NULL) {
                    printf("Error: No se pudo asignar memoria para la apeada\n");
                    return NULL;
                }
                
                nuevaApeada->esGrupo = false;
                nuevaApeada->idJugador = jugador->id;
                
                /* Crear la escalera */
                nuevaApeada->jugada.escalera.capacidad = longitudFinal;
                nuevaApeada->jugada.escalera.numCartas = longitudFinal;
                nuevaApeada->jugada.escalera.palo = palo;
                nuevaApeada->jugada.escalera.cartas = (Carta*)malloc(longitudFinal * sizeof(Carta));
                
                // Al liberar memoria, siempre verifica y establece a NULL
                if (nuevaApeada != NULL) {
                    free(nuevaApeada);
                    nuevaApeada = NULL;
                }

                if (nuevaApeada != NULL && nuevaApeada->jugada.escalera.cartas == NULL) {
                    if (nuevaApeada->jugada.escalera.cartas != NULL) {
                        free(nuevaApeada->jugada.escalera.cartas);
                    }
                    free(nuevaApeada);
                    return NULL;
                }
                
                /* Copiar las cartas a la escalera, insertando comodines donde corresponda */
                indiceEscalera = 0;
                comodinesUsados = 0;
                
                for (j = inicioEscalera; j <= finEscalera; j++) {
                    /* Si es la primera carta o la siguiente en secuencia, añadirla directamente */
                    if (j == inicioEscalera || escaleraTemp[j].valor == escaleraTemp[j-1].valor + 1) {
                        nuevaApeada->jugada.escalera.cartas[indiceEscalera] = escaleraTemp[j];
                        indiceEscalera++;
                    } else {
                        /* Insertar comodines para llenar los huecos */
                        huecoActual = escaleraTemp[j].valor - escaleraTemp[j-1].valor - 1;
                        
                        for (k = 0; k < huecoActual; k++) {
                            nuevaApeada->jugada.escalera.cartas[indiceEscalera] = jugador->mano.cartas[indicesComodines[comodinesUsados]];
                            indiceEscalera++;
                            comodinesUsados++;
                        }
                        
                        /* Luego añadir la carta actual */
                        nuevaApeada->jugada.escalera.cartas[indiceEscalera] = escaleraTemp[j];
                        indiceEscalera++;
                    }
                }
                
                /* Calcular puntos */
                nuevaApeada->puntos = 0;
                for (j = 0; j < longitudEscalera; j++) {
                    if (escaleraTemp[inicioEscalera + j].valor >= 11) {  /* J, Q, K */
                        nuevaApeada->puntos += 10;
                    } else if (escaleraTemp[inicioEscalera + j].valor == 1) {  /* As */
                        nuevaApeada->puntos += 15;
                    } else {
                        nuevaApeada->puntos += escaleraTemp[inicioEscalera + j].valor;
                    }
                }
                nuevaApeada->puntos += comodinesUsados * 20;  /* Valor de los comodines */
                
                /* Eliminar los comodines de la mano del jugador (de atrás hacia adelante) */
                for (j = comodinesUsados - 1; j >= 0; j--) {
                    indice = indicesComodines[j];
                    for (k = indice; k < jugador->mano.numCartas - 1; k++) {
                        jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                    }
                    jugador->mano.numCartas--;
                    
                    /* Ajustar los índices restantes */
                    for (k = j + 1; k < comodinesUsados; k++) {
                        if (indicesComodines[k] > indice) {
                            indicesComodines[k]--;
                        }
                    }
                }
                
                /* Eliminar las cartas normales de la mano del jugador (de atrás hacia adelante) */
                for (j = 0; j < longitudEscalera; j++) {
                    indicesAEliminar[j] = indicesMano[inicioEscalera + j];
                }
                
                /* Ordenar índices de mayor a menor para eliminar correctamente */
                for (i = 0; i < longitudEscalera - 1; i++) {
                    for (j = 0; j < longitudEscalera - i - 1; j++) {
                        if (indicesAEliminar[j] < indicesAEliminar[j+1]) {
                            temp = indicesAEliminar[j];
                            indicesAEliminar[j] = indicesAEliminar[j+1];
                            indicesAEliminar[j+1] = temp;
                        }
                    }
                }
                
                /* Eliminar las cartas, ajustando índices por los comodines ya eliminados */
                for (j = 0; j < longitudEscalera; j++) {
                    indice = indicesAEliminar[j];
                    
                    /* Ajustar el índice basado en los comodines eliminados */
                    for (k = 0; k < comodinesUsados; k++) {
                        if (indicesComodines[k] < indice) {
                            indice--;
                        }
                    }
                    
                    for (k = indice; k < jugador->mano.numCartas - 1; k++) {
                        jugador->mano.cartas[k] = jugador->mano.cartas[k+1];
                    }
                    jugador->mano.numCartas--;
                }
                
                /* Actualizar BCP */
                if (jugador->bcp != NULL) {
                    jugador->bcp->vecesApeo++;
                    actualizarBCPJugador(jugador);
                }
                
                return nuevaApeada;
            }
            
            /* Continuar con la siguiente secuencia */
            inicioEscalera = finEscalera + 1;
        }
    }
    
    return NULL;  /* No se pudo crear ninguna apeada */
}

/* Comer una ficha de la banca */
bool comerFicha(Jugador *jugador, Mazo *banca) {
    char cartaStr[50];
    
    if (banca->numCartas <= 0) {
        return false;
    }
    
    /* Tomar una carta de la banca */
    Carta carta = banca->cartas[banca->numCartas - 1];
    banca->numCartas--;
    
    /* Añadir carta a la mano del jugador */
    if (jugador->mano.numCartas >= jugador->mano.capacidad) {
        /* Ampliar capacidad si es necesario */
        int nuevaCapacidad = jugador->mano.capacidad == 0 ? 10 : jugador->mano.capacidad * 2;
        Carta *nuevasCartas = realloc(jugador->mano.cartas, nuevaCapacidad * sizeof(Carta));
        
        if (nuevasCartas == NULL) {
            /* No se pudo ampliar, devolver la carta a la banca */
            banca->numCartas++;
            return false;
        }
        
        jugador->mano.cartas = nuevasCartas;
        jugador->mano.capacidad = nuevaCapacidad;
    }
    
    /* Añadir la carta al mazo del jugador */
    jugador->mano.cartas[jugador->mano.numCartas] = carta;
    jugador->mano.numCartas++;
    
    obtenerNombreCarta(carta, cartaStr);
    printf("Jugador %d comió una ficha: %s\n", jugador->id, cartaStr);
    
    return true;
}

/* Actualizar el estado del jugador */
void actualizarEstadoJugador(Jugador *jugador, EstadoJugador nuevoEstado) {
    const char *estados[] = {"LISTO", "EJECUCION", "ESPERA_ES", "BLOQUEADO"};
    
    jugador->estado = nuevoEstado;
    
    /* Actualizar el BCP */
    actualizarBCPJugador(jugador);
    
    /* Actualizar en la tabla de procesos */
    pthread_mutex_lock(&mutexTabla);
    actualizarProcesoEnTabla(jugador->id, nuevoEstado);
    pthread_mutex_unlock(&mutexTabla);
    
    /* Mostrar cambio de estado */
    printf("Jugador %d cambió a estado: %s\n", jugador->id, estados[nuevoEstado]);
}

/* Pasar el turno del jugador */
void pasarTurno(Jugador *jugador) {
    jugador->turnoActual = false;
    
    /* Cambiar a estado LISTO o ESPERA_ES según corresponda */
    if (jugador->estado != ESPERA_ES) {
        actualizarEstadoJugador(jugador, LISTO);
    }
}

/* Entrar en estado de espera E/S */
void entrarEsperaES(Jugador *jugador) {
    /* Tiempo aleatorio entre 2 y 5 segundos */
    jugador->tiempoES = (rand() % 3 + 2) * 1000;
    actualizarEstadoJugador(jugador, ESPERA_ES);
    
    colorMagenta();
    printf("Jugador %d entró en E/S por %d ms\n", jugador->id, jugador->tiempoES);
    colorReset();
    
    /* Registrar evento de E/S */
    registrarEvento("Jugador %d entró en E/S por %d ms", jugador->id, jugador->tiempoES);
    
    /* Actualizar BCP */
    if (jugador->bcp != NULL) {
        jugador->bcp->tiempoES = jugador->tiempoES;
        actualizarBCPJugador(jugador);
    }
}

/* Salir del estado de espera E/S */
void salirEsperaES(Jugador *jugador) {
    jugador->tiempoES = 0;
    actualizarEstadoJugador(jugador, LISTO);
    
    colorMagenta();
    printf("Jugador %d salió de E/S\n", jugador->id);
    colorReset();
    
    /* Registrar evento */
    registrarEvento("Jugador %d salió de E/S", jugador->id);
    
    /* Actualizar BCP */
    if (jugador->bcp != NULL) {
        jugador->bcp->tiempoES = 0;
        actualizarBCPJugador(jugador);
    }
}

/* Actualizar el BCP del jugador */
void actualizarBCPJugador(Jugador *jugador) {
    if (jugador->bcp != NULL) {
        /* Actualizar las variables del BCP */
        jugador->bcp->estado = jugador->estado;
        jugador->bcp->tiempoEspera = jugador->tiempoES;
        jugador->bcp->tiempoRestante = jugador->tiempoRestante;
        jugador->bcp->prioridad = jugador->id;  /* Por ahora usamos el ID como prioridad */
        jugador->bcp->numCartas = jugador->mano.numCartas;
        jugador->bcp->turnoActual = jugador->turnoActual ? 1 : 0;
        
        /* Guardar el BCP en archivo */
        guardarBCP(jugador->bcp);
    }
}

/* Liberar recursos del jugador */
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