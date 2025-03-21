#ifndef JUGADORES_H
#define JUGADORES_H
#define MAX_PROCESOS 10

#include <pthread.h>
#include <stdbool.h>
#include "procesos_stub.h"  // En vez de procesos.h

// Estructura de una carta/ficha
typedef struct {
    int valor;           // Valor de la carta (1-13)
    char palo;           // Palo: C (Corazones), D (Diamantes), T (Tréboles), E (Espadas)
    bool esComodin;      // Indica si la carta es un comodín
} Carta;

// Estructura para una terna o cuaterna (grupo)
typedef struct {
    Carta cartas[4];     // Máximo 4 cartas (cuaterna)
    int numCartas;       // Número de cartas en el grupo
} Grupo;

// Estructura para una escalera
typedef struct {
    Carta *cartas;       // Arreglo dinámico de cartas
    int numCartas;       // Número de cartas en la escalera
    char palo;           // El palo de la escalera
} Escalera;

// Estructura para una jugada (apeada)
typedef struct {
    union {
        Grupo grupo;
        Escalera escalera;
    } jugada;
    bool esGrupo;        // true si es grupo, false si es escalera
    int puntos;          // Puntos totales de la jugada
    int idJugador;       // ID del jugador que realizó la jugada
} Apeada;

// Estructura para el mazo de cartas de un jugador
typedef struct {
    Carta *cartas;       // Arreglo dinámico de cartas
    int numCartas;       // Número actual de cartas
    int capacidad;       // Capacidad actual del arreglo dinámico
} Mazo;

// Declaraciones de funciones externas
bool juegoTerminado(void);
Apeada* obtenerApeadas(void);
int obtenerNumApeadas(void);
Mazo* obtenerBanca(void);
void finalizarJuego(int idJugador);

// Definición de estados de los jugadores
typedef enum {
    LISTO,              // El jugador está listo para jugar su turno
    EJECUCION,          // El jugador está realizando su turno
    ESPERA_ES,          // El jugador está en espera de E/S (comiendo fichas)
    BLOQUEADO           // El jugador está bloqueado (esperando su turno)
} EstadoJugador;

// Estados de procesos
typedef enum {
    PROC_NUEVO,
    PROC_LISTO,      
    PROC_EJECUTANDO, 
    PROC_BLOQUEADO,  
    PROC_TERMINADO   
} EstadoProceso;

struct BCP {
    int id;                    // ID único del proceso/jugador
    EstadoProceso estado;      // Estado actual del proceso
    int prioridad;             // Prioridad del proceso
    int tiempoCreacion;        // Tiempo en que se creó el proceso
    int tiempoEjecucion;       // Tiempo que ha estado ejecutándose
    int tiempoEspera;          // Tiempo que ha estado en espera
    int tiempoBloqueo;         // Tiempo que ha estado bloqueado
    int tiempoES;              // Tiempo de operación de E/S restante
    int tiempoQuantum;         // Tiempo de quantum asignado (para Round Robin)
    int tiempoRestante;        // Tiempo restante del quantum actual
    int numCartas;             // Número de cartas en mano
    int puntosAcumulados;      // Puntos acumulados en juego
    int turnoActual;           // Turno actual del juego
    int vecesApeo;             // Número de veces que se ha apeado
    int cartasComidas;         // Número de cartas tomadas de la banca
    int intentosFallidos;      // Intentos fallidos de apearse
    int turnosPerdidos;        // Turnos perdidos por timeouts
};

// Estructura principal del jugador
typedef struct {
    int id;                  // ID único del jugador
    Mazo mano;              // Cartas en la mano del jugador
    EstadoJugador estado;   // Estado actual del jugador
    pthread_t hilo;          // Identificador del hilo del jugador
    bool primeraApeada;      // Indica si ya realizó su primera apeada (30 puntos)
    int tiempoTurno;         // Tiempo asignado para su turno
    int tiempoRestante;      // Tiempo restante de su turno
    int tiempoES;            // Tiempo en E/S cuando come una ficha
    bool turnoActual;        // Indica si es su turno actual
    struct BCP *bcp;                // Bloque de Control de Proceso asociado
    int puntosTotal;         // Puntos totales acumulados
    bool terminado;          // Indica si el jugador ha terminado sus cartas
} Jugador;

// Funciones para manejo de jugadores
void inicializarJugador(Jugador *jugador, int id);
void *funcionHiloJugador(void *arg);
bool realizarTurno(Jugador *jugador, Apeada *apeadas, int numApeadas, Mazo *banca);
bool verificarApeada(Jugador *jugador, Apeada *apeada);
bool puedeApearse(Jugador *jugador);
bool comerFicha(Jugador *jugador, Mazo *banca);
void actualizarEstadoJugador(Jugador *jugador, EstadoJugador nuevoEstado);
void pasarTurno(Jugador *jugador);
void entrarEsperaES(Jugador *jugador);
void salirEsperaES(Jugador *jugador);
void liberarJugador(Jugador *jugador);
void actualizarBCP(Jugador *jugador);

#endif // JUGADORES_H