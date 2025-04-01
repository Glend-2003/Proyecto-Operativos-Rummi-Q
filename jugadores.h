#ifndef JUGADORES_H
#define JUGADORES_H

#include <pthread.h>
#include <stdbool.h>
#include <limits.h>
#include "procesos.h"

/* Estructura de una carta/ficha */
typedef struct {
    int valor;           /* Valor de la carta (1-13) */
    char palo;           /* Palo: C (Corazones), D (Diamantes), T (Tréboles), E (Espadas) */
    bool esComodin;      /* Indica si la carta es un comodín */
} Carta;

/* Estructura para una terna o cuaterna (grupo) */
typedef struct {
    Carta cartas[4];     /* Máximo 4 cartas (cuaterna) */
    int numCartas;       /* Número de cartas en el grupo */
} Grupo;

/* Estructura para una escalera */
typedef struct {
    Carta *cartas;       /* Arreglo dinámico de cartas */
    int numCartas;       /* Número de cartas en la escalera */
    int capacidad;       /* Capacidad actual del arreglo */
    char palo;           /* El palo de la escalera */
} Escalera;

/* Estructura para una jugada (apeada) */
typedef struct {
    union {
        Grupo grupo;
        Escalera escalera;
    } jugada;
    bool esGrupo;        /* true si es grupo, false si es escalera */
    int puntos;          /* Puntos totales de la jugada */
    int idJugador;       /* ID del jugador que realizó la jugada */
} Apeada;

/* Estructura para el mazo de cartas de un jugador */
typedef struct {
    Carta *cartas;       /* Arreglo dinámico de cartas */
    int numCartas;       /* Número actual de cartas */
    int capacidad;       /* Capacidad actual del arreglo dinámico */
} Mazo;

/* Definición de estados de los jugadores */
typedef enum {
    LISTO,              /* El jugador está listo para jugar su turno */
    EJECUCION,          /* El jugador está realizando su turno */
    ESPERA_ES,          /* El jugador está en espera de E/S (comiendo fichas) */
    BLOQUEADO           /* El jugador está bloqueado (esperando su turno) */
} EstadoJugador;

/* Declaración adelantada de BCP para evitar dependencias circulares */
struct BCP;

/* Estructura principal del jugador */
typedef struct {
    int id;                  /* ID único del jugador */
    Mazo mano;              /* Cartas en la mano del jugador */
    EstadoJugador estado;   /* Estado actual del jugador */
    pthread_t hilo;          /* Identificador del hilo del jugador */
    bool primeraApeada;      /* Indica si ya realizó su primera apeada (30 puntos) */
    int tiempoTurno;         /* Tiempo asignado para su turno */
    int tiempoRestante;      /* Tiempo restante de su turno */
    int tiempoES;            /* Tiempo en E/S cuando come una ficha */
    bool turnoActual;        /* Indica si es su turno actual */
    BCP *bcp;        /* Bloque de Control de Proceso asociado */
    int puntosTotal;         /* Puntos totales acumulados */
    bool terminado;          /* Indica si el jugador ha terminado sus cartas */
} Jugador;

/* Declaraciones de funciones externas */
bool juegoTerminado(void);
Apeada* obtenerApeadas(void);
int obtenerNumApeadas(void);
Mazo* obtenerBanca(void);
void finalizarJuego(int idJugador);

/* Funciones para manejo de jugadores */
void inicializarJugador(Jugador *jugador, int id);
void *funcionHiloJugador(void *arg);
bool realizarTurno(Jugador *jugador, Apeada *apeadas, int numApeadas, Mazo *banca);

/* Funciones para verificar y realizar jugadas */
bool verificarApeada(Jugador *jugador, Apeada *apeada);
bool puedeApearse(Jugador *jugador);
bool realizarJugadaApeada(Jugador *jugador, Apeada *apeada);
Apeada* crearApeada(Jugador *jugador);

/* Funciones para operaciones básicas */
bool comerFicha(Jugador *jugador, Mazo *banca);
void actualizarEstadoJugador(Jugador *jugador, EstadoJugador nuevoEstado);
void pasarTurno(Jugador *jugador);
void entrarEsperaES(Jugador *jugador);
void salirEsperaES(Jugador *jugador);
void liberarJugador(Jugador *jugador);

/* Funciones para manejo de BCP */
void actualizarBCPJugador(Jugador *jugador);

/* Mutex externos */
extern pthread_mutex_t mutexApeadas;
extern pthread_mutex_t mutexBanca;
extern pthread_mutex_t mutexTabla;

#endif /* JUGADORES_H */