#ifndef JUGADORES_H
#define JUGADORES_H

#include <pthread.h>
#include <stdbool.h>
#include <limits.h>
#include "procesos.h"

// ============== ESTRUCTURAS DE DATOS ==============
typedef struct {
    int valor;
    char palo;
    bool esComodin;
} Carta;

typedef struct {
    Carta cartas[4];
    int numCartas;
} Grupo;

typedef struct {
    Carta *cartas;
    int numCartas;
    int capacidad;
    char palo;
} Escalera;

typedef struct {
    union {
        Grupo grupo;
        Escalera escalera;
    } jugada;
    bool esGrupo;
    int puntos;
    int idJugador;
} Apeada;

typedef struct {
    Carta *cartas;
    int numCartas;
    int capacidad;
} Mazo;

typedef enum {
    LISTO,
    EJECUCION,
    ESPERA_ES,
    BLOQUEADO
} EstadoJugador;

struct BCP;

typedef struct {
    int id;
    Mazo mano;
    EstadoJugador estado;
    pthread_t hilo;
    bool primeraApeada;
    int tiempoTurno;
    int tiempoRestante;
    int tiempoES;
    bool turnoActual;
    struct BCP *bcp;
    int puntosTotal;
    bool terminado;
} Jugador;

// ============== FUNCIONES EXTERNAS ==============
bool juegoTerminado(void);
Apeada* obtenerApeadas(void);
int obtenerNumApeadas(void);
Mazo* obtenerBanca(void);
void finalizarJuego(int idJugador);

// ============== INICIALIZACIÓN ==============
void inicializarJugador(Jugador *jugador, int id);
void *funcionHiloJugador(void *arg);

// ============== LÓGICA DE TURNO ==============
bool realizarTurno(Jugador *jugador, Apeada *apeadas, int numApeadas, Mazo *banca);
bool intentarPrimeraApeada(Jugador *jugador, bool *hizoJugada);
bool intentarJugadaExistente(Jugador *jugador, Apeada *apeadas, int numApeadas, bool *hizoJugada);
bool intentarComerFicha(Jugador *jugador, Mazo *banca);

// ============== VALIDACIÓN Y CREACIÓN ==============
bool puedeApearse(Jugador *jugador);
bool verificarApeada(Jugador *jugador, Apeada *apeada);
bool realizarJugadaApeada(Jugador *jugador, Apeada *apeada);
Apeada* crearApeada(Jugador *jugador);

// ============== OPERACIONES BÁSICAS ==============
bool comerFicha(Jugador *jugador, Mazo *banca);
void actualizarEstadoJugador(Jugador *jugador, EstadoJugador nuevoEstado);
void pasarTurno(Jugador *jugador);

// ============== ENTRADA/SALIDA ==============
void entrarEsperaES(Jugador *jugador);
void salirEsperaES(Jugador *jugador);

// ============== BCP Y LIMPIEZA ==============
void actualizarBCPJugador(Jugador *jugador);
void liberarJugador(Jugador *jugador);

// ============== MUTEX EXTERNOS ==============
extern pthread_mutex_t mutexApeadas;
extern pthread_mutex_t mutexBanca;
extern pthread_mutex_t mutexTabla;

#endif /* JUGADORES_H */