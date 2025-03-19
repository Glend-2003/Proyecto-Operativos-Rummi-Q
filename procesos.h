#ifndef PROCESOS_H
#define PROCESOS_H

#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include "jugadores.h"

// Definición de estados de proceso
typedef enum {
    PROCESO_NUEVO,
    PROCESO_LISTO,
    PROCESO_EJECUTANDO,
    PROCESO_BLOQUEADO,
    PROCESO_TERMINADO
} EstadoProceso;

// Tipos de algoritmos de planificación
typedef enum {
    FCFS,   // First Come First Served
    RR      // Round Robin
} TipoAlgoritmo;

// Estructura para el Bloque de Control de Proceso (BCP)
typedef struct {
    int pid;                    // ID del proceso
    int jugador_id;             // ID del jugador asociado
    EstadoProceso estado;       // Estado actual del proceso
    time_t tiempo_creacion;     // Tiempo en que se creó el proceso
    time_t tiempo_inicio;       // Tiempo en que comenzó a ejecutarse
    time_t tiempo_fin;          // Tiempo en que terminó de ejecutarse
    int prioridad;              // Prioridad del proceso
    int tiempo_cpu;             // Tiempo total de CPU utilizado
    int tiempo_espera;          // Tiempo en estado de espera
    int tiempo_restante;        // Tiempo restante de ejecución (para RR)
    int quantum;                // Quantum asignado (para RR)
    int turnos_perdidos;        // Conteo de turnos perdidos
    int puntos_acumulados;      // Puntos acumulados por el jugador
    int fichas_colocadas;       // Fichas colocadas en la mesa
    bool primera_apeada;        // Indica si ya realizó la primera apeada
    time_t tiempo_bloqueado;    // Tiempo que estará bloqueado en E/S
    int intentos_apeada;        // Número de intentos de apeada
} BCP;

// Estructura para la tabla de procesos
typedef struct {
    BCP *procesos[NUM_JUGADORES];     // Arreglo de punteros a BCP
    int num_procesos;                 // Número de procesos en la tabla
    int proceso_actual;               // Índice del proceso actual
    pthread_mutex_t mutex;            // Mutex para acceso concurrente
    TipoAlgoritmo algoritmo;          // Algoritmo de planificación actual
    int quantum_global;               // Quantum para RR (en segundos)
    time_t ultimo_cambio_contexto;    // Tiempo del último cambio de contexto
    int cambios_contexto;             // Número de cambios de contexto
    int procesos_bloqueados;          // Número de procesos bloqueados
    int procesos_listos;              // Número de procesos listos
    bool planificador_activo;         // Estado del planificador
    time_t tiempo_inicio_juego;       // Tiempo en que inició el juego
    pthread_t hilo_planificador;      // Hilo del planificador
    int turnos_completados;           // Número total de turnos completados
    int max_tiempo_ejecucion;         // Tiempo máximo de ejecución por turno
} TablaProcesos;

// Estructura para la cola de E/S
typedef struct {
    int procesos_ids[NUM_JUGADORES];  // IDs de procesos en E/S
    int tiempos_espera[NUM_JUGADORES]; // Tiempos de espera correspondientes
    int num_procesos;                  // Número de procesos en la cola
    pthread_mutex_t mutex;             // Mutex para acceso concurrente
    pthread_t hilo_es;                 // Hilo de manejo de E/S
    bool activo;                       // Estado del gestor de E/S
} ColaES;

// Funciones de inicialización
void inicializar_sistema_procesos();
void crear_proceso(int jugador_id);
void destruir_proceso(int pid);

// Funciones de planificación
void cambiar_algoritmo(TipoAlgoritmo nuevo_algoritmo);
void planificar_siguiente_proceso();
void* hilo_planificador(void *arg);

// Funciones de E/S
void bloquear_proceso(int pid, int tiempo_espera);
void* hilo_gestor_es(void *arg);
void verificar_procesos_es();

// Funciones de actualización de datos
void actualizar_bcp(int pid, EstadoProceso nuevo_estado);
void registrar_turno_completado(int pid);
void registrar_turno_perdido(int pid);
void actualizar_puntos(int pid, int puntos);

// Funciones de acceso a datos
BCP* obtener_bcp(int pid);
TablaProcesos* obtener_tabla_procesos();
void guardar_estado_bcp(int pid);
void guardar_estado_tabla_procesos();

// Funciones de utilidad
void imprimir_estado_procesos();
int calcular_tiempo_aleatorio_es();

#endif