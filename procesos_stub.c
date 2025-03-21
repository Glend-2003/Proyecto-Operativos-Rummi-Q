#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "procesos_stub.h"
#include "jugadores.h"

// Variable global para la tabla de procesos
struct {
    struct BCP* procesos[MAX_PROCESOS];
    int numProcesos;
} tablaProcesosTemporal;

// Implementación temporal de crearBCP
struct BCP* crearBCP(int id) {
    struct BCP* nuevoBCP = (struct BCP*)malloc(sizeof(struct BCP));
    if (nuevoBCP != NULL) {
        nuevoBCP->id = id;
        nuevoBCP->estado = 0; // Valor por defecto
        nuevoBCP->tiempoEspera = 0;
        nuevoBCP->tiempoEjecucion = 0;
        nuevoBCP->prioridad = id;
        nuevoBCP->numCartas = 0;
        nuevoBCP->turnoActual = 0;
    }
    return nuevoBCP;
}

// Implementación temporal de guardarBCP
void guardarBCP(struct BCP* bcp) {
    if (bcp == NULL) return;
    
    printf("[STUB] Guardando BCP para jugador %d\n", bcp->id);
    // En esta implementación stub, solo imprimimos un mensaje
}

// Implementación temporal de liberarBCP
void liberarBCP(struct BCP* bcp) {
    if (bcp != NULL) {
        free(bcp);
    }
}

// Implementación temporal de inicializarTabla
void inicializarTabla(void) {
    printf("[STUB] Inicializando tabla de procesos\n");
    
    for (int i = 0; i < MAX_PROCESOS; i++) {
        tablaProcesosTemporal.procesos[i] = NULL;
    }
    tablaProcesosTemporal.numProcesos = 0;
}

// Implementación temporal de registrarProcesoEnTabla
void registrarProcesoEnTabla(int id, int estado) {
    printf("[STUB] Registrando proceso %d en estado %d\n", id, estado);
    
    if (tablaProcesosTemporal.numProcesos < MAX_PROCESOS) {
        int indice = tablaProcesosTemporal.numProcesos;
        tablaProcesosTemporal.procesos[indice] = crearBCP(id);
        tablaProcesosTemporal.procesos[indice]->estado = estado;
        tablaProcesosTemporal.numProcesos++;
    }
}

// Implementación temporal de actualizarProcesoEnTabla
void actualizarProcesoEnTabla(int id, int estado) {
    printf("[STUB] Actualizando proceso %d a estado %d\n", id, estado);
    
    // Buscar el proceso con el ID proporcionado
    for (int i = 0; i < tablaProcesosTemporal.numProcesos; i++) {
        if (tablaProcesosTemporal.procesos[i]->id == id) {
            tablaProcesosTemporal.procesos[i]->estado = estado;
            break;
        }
    }
}

// Implementación temporal de imprimirEstadisticasTabla
void imprimirEstadisticasTabla(void) {
    printf("[STUB] Imprimiendo estadísticas de la tabla de procesos\n");
    printf("Total de procesos registrados: %d\n", tablaProcesosTemporal.numProcesos);
    
    for (int i = 0; i < tablaProcesosTemporal.numProcesos; i++) {
        printf("Proceso %d - Estado: %d\n", 
               tablaProcesosTemporal.procesos[i]->id, 
               tablaProcesosTemporal.procesos[i]->estado);
    }
}

// Implementación temporal de liberarTabla
void liberarTabla(void) {
    printf("[STUB] Liberando tabla de procesos\n");
    
    for (int i = 0; i < tablaProcesosTemporal.numProcesos; i++) {
        if (tablaProcesosTemporal.procesos[i] != NULL) {
            liberarBCP(tablaProcesosTemporal.procesos[i]);
            tablaProcesosTemporal.procesos[i] = NULL;
        }
    }
    tablaProcesosTemporal.numProcesos = 0;
}