#ifndef PROCESOS_H
#define PROCESOS_H

#include <stdbool.h>
#include <time.h>

//==============================================================================
// ENUMERACIONES Y CONSTANTES
//==============================================================================

typedef enum {
    PROC_NUEVO,
    PROC_LISTO,
    PROC_EJECUTANDO,
    PROC_BLOQUEADO,
    PROC_TERMINADO
} EstadoProceso;

//==============================================================================
// ESTRUCTURAS
//==============================================================================

typedef struct BCP {
    int id;
    EstadoProceso estado;
    int prioridad;
    time_t tiempoCreacion;
    int tiempoEjecucion;
    int tiempoEspera;
    int tiempoBloqueo;
    int tiempoES;
    int tiempoQuantum;
    int tiempoRestante;
    int numCartas;
    int puntosAcumulados;
    int turnoActual;
    int vecesApeo;
    int cartasComidas;
    int intentosFallidos;
    int turnosPerdidos;
    int cambiosEstado;
    int tiempoUltimoEstado;
    int tiempoUltimoBloqueo;
} BCP;

typedef struct {
    BCP *procesos[10];
    int numProcesos;
    int procesoActual;
    int algoritmoActual;
    int numProcesosBloqueados;
    int numProcesosListos;
    int numProcesosSuspendidos;
    int numProcesosTerminados;
    int cambiosContexto;
    int tiempoTotalEjecucion;
    int tiempoTotalEspera;
    int tiempoTotalBloqueo;
    int tiempoTotalES;
    int turnosAsignados;
    int turnosCompletados;
    int turnosInterrumpidos;
    time_t tiempoInicio;
    time_t tiempoActual;
    float usoCPU;
    int ultimoCambioAlgoritmo;
} TablaProc;

//==============================================================================
// FUNCIONES DE MANEJO DEL BCP
//==============================================================================

BCP* crearBCP(int id);
void actualizarBCP(BCP *bcp, int estado, int tiempoEjec, int tiempoEsp, int cartas);
void guardarBCP(BCP *bcp);
void liberarBCP(BCP *bcp);

//==============================================================================
// FUNCIONES DE INICIALIZACIÓN Y LIBERACIÓN DE TABLA
//==============================================================================

void inicializarTabla(void);
void liberarTabla(void);

//==============================================================================
// FUNCIONES DE REGISTRO Y GESTIÓN DE PROCESOS
//==============================================================================

void registrarProcesoEnTabla(int id, int estado);
void actualizarProcesoEnTabla(int id, int estado);
void cambiarEstadoProceso(int id, int nuevoEstado);
void marcarProcesoTerminado(int id);

//==============================================================================
// FUNCIONES DE ASIGNACIÓN Y CONTROL DE TIEMPO
//==============================================================================

void asignarQuantum(int id, int quantum);
void aumentarTiempoEjecucion(int id, int tiempo);
void aumentarTiempoEspera(int id, int tiempo);
void aumentarTiempoBloqueo(int id, int tiempo);

//==============================================================================
// FUNCIONES DE CONTROL DE CONTEXTO Y ACCESO
//==============================================================================

void registrarCambioContexto(void);
BCP* obtenerBCPActual(void);

//==============================================================================
// FUNCIONES DE VISUALIZACIÓN
//==============================================================================

void imprimirEstadisticasTabla(void);

#endif /* PROCESOS_H */