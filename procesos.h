#ifndef PROCESOS_H
#define PROCESOS_H

#include <stdbool.h>
#include <time.h>

/* Definición de estados de proceso */
typedef enum {
    PROC_NUEVO,        /* Proceso recién creado */
    PROC_LISTO,        /* Proceso listo para ejecutar */
    PROC_EJECUTANDO,   /* Proceso en ejecución */
    PROC_BLOQUEADO,    /* Proceso bloqueado (esperando E/S o recurso) */
    PROC_TERMINADO     /* Proceso terminado */
} EstadoProceso;

/* Estructura para el Bloque de Control de Proceso (BCP) */
typedef struct BCP {
    /* Variables requeridas (mínimo 15) */
    int id;                     /* ID único del proceso/jugador */
    EstadoProceso estado;       /* Estado actual del proceso */
    int prioridad;              /* Prioridad del proceso */
    time_t tiempoCreacion;      /* Tiempo en que se creó el proceso */
    int tiempoEjecucion;        /* Tiempo total que ha estado ejecutándose */
    int tiempoEspera;           /* Tiempo total que ha estado en espera */
    int tiempoBloqueo;          /* Tiempo total que ha estado bloqueado */
    int tiempoES;               /* Tiempo de operación de E/S restante */
    int tiempoQuantum;          /* Tiempo de quantum asignado (para Round Robin) */
    int tiempoRestante;         /* Tiempo restante del quantum actual */
    int numCartas;              /* Número de cartas en mano */
    int puntosAcumulados;       /* Puntos acumulados en juego */
    int turnoActual;            /* Indica si es su turno actual (1=sí, 0=no) */
    int vecesApeo;              /* Número de veces que se ha apeado */
    int cartasComidas;          /* Número de cartas tomadas de la banca */
    int intentosFallidos;       /* Intentos fallidos de apearse */
    int turnosPerdidos;         /* Turnos perdidos por timeouts */
    int cambiosEstado;          /* Número de cambios de estado */
    int tiempoUltimoEstado;     /* Tiempo en el estado actual */
    int tiempoUltimoBloqueo;    /* Tiempo desde el último bloqueo */
} BCP;

/* Estructura para la tabla de procesos */
typedef struct {
    /* Variables requeridas (mínimo 15) */
    BCP *procesos[10];                /* Array de punteros a BCPs (máximo 10 procesos) */
    int numProcesos;                  /* Número actual de procesos */
    int procesoActual;                /* Índice del proceso en ejecución */
    int algoritmoActual;              /* Algoritmo de planificación actual */
    int numProcesosBloqueados;        /* Número de procesos bloqueados */
    int numProcesosListos;            /* Número de procesos listos */
    int numProcesosSuspendidos;       /* Número de procesos suspendidos */
    int numProcesosTerminados;        /* Número de procesos terminados */
    int cambiosContexto;              /* Número de cambios de contexto */
    int tiempoTotalEjecucion;         /* Tiempo total de ejecución */
    int tiempoTotalEspera;            /* Tiempo total de espera */
    int tiempoTotalBloqueo;           /* Tiempo total de bloqueo */
    int tiempoTotalES;                /* Tiempo total en E/S */
    int turnosAsignados;              /* Número total de turnos asignados */
    int turnosCompletados;            /* Número de turnos completados */
    int turnosInterrumpidos;          /* Número de turnos interrumpidos */
    time_t tiempoInicio;              /* Tiempo de inicio de la simulación */
    time_t tiempoActual;              /* Tiempo actual */
    float usoCPU;                     /* Porcentaje de uso de CPU */
    int ultimoCambioAlgoritmo;        /* Tiempo del último cambio de algoritmo */
} TablaProc;

/* Funciones para el manejo del BCP */
BCP* crearBCP(int id);
void actualizarBCP(BCP *bcp, int estado, int tiempoEjec, int tiempoEsp, int cartas);
void guardarBCP(BCP *bcp);
void liberarBCP(BCP *bcp);

/* Funciones para el manejo de la tabla de procesos */
void inicializarTabla(void);
void registrarProcesoEnTabla(int id, int estado);
void actualizarProcesoEnTabla(int id, int estado);
void cambiarEstadoProceso(int id, int nuevoEstado);
void asignarQuantum(int id, int quantum);
void marcarProcesoTerminado(int id);
void aumentarTiempoEjecucion(int id, int tiempo);
void aumentarTiempoEspera(int id, int tiempo);
void aumentarTiempoBloqueo(int id, int tiempo);
void registrarCambioContexto(void);
BCP* obtenerBCPActual(void);
void imprimirEstadisticasTabla(void);
void liberarTabla(void);

#endif /* PROCESOS_H */