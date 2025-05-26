#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "procesos.h"
#include "utilidades.h"

static TablaProc tablaProc;

#define RUTA_BCP "bcp/"

//==============================================================================
// FUNCIONES DE MANEJO DEL BCP
//==============================================================================

BCP* crearBCP(int id) {
    BCP* nuevoBCP = (BCP*)malloc(sizeof(BCP));
    if (nuevoBCP == NULL) {
        printf("Error: No se pudo asignar memoria para el BCP\n");
        return NULL;
    }
    
    nuevoBCP->id = id;
    nuevoBCP->estado = PROC_NUEVO;
    nuevoBCP->prioridad = 0;
    nuevoBCP->tiempoCreacion = time(NULL);
    nuevoBCP->tiempoEjecucion = 0;
    nuevoBCP->tiempoEspera = 0;
    nuevoBCP->tiempoBloqueo = 0;
    nuevoBCP->tiempoES = 0;
    nuevoBCP->tiempoQuantum = 0;
    nuevoBCP->tiempoRestante = 0;
    nuevoBCP->numCartas = 0;
    nuevoBCP->puntosAcumulados = 0;
    nuevoBCP->turnoActual = 0;
    nuevoBCP->vecesApeo = 0;
    nuevoBCP->cartasComidas = 0;
    nuevoBCP->intentosFallidos = 0;
    nuevoBCP->turnosPerdidos = 0;
    nuevoBCP->cambiosEstado = 0;
    nuevoBCP->tiempoUltimoEstado = 0;
    nuevoBCP->tiempoUltimoBloqueo = 0;
    
    printf("BCP creado para el proceso %d\n", id);
    return nuevoBCP;
}

void actualizarBCP(BCP *bcp, int estado, int tiempoEjec, int tiempoEsp, int cartas) {
    if (bcp == NULL) {
        return;
    }
    
    if (bcp->estado != estado) {
        bcp->cambiosEstado++;
        bcp->tiempoUltimoEstado = 0;
        
        if (estado == PROC_BLOQUEADO) {
            bcp->tiempoUltimoBloqueo = 0;
        }
    } else {
        bcp->tiempoUltimoEstado += tiempoEjec + tiempoEsp;
    }
    
    bcp->estado = estado;
    bcp->tiempoEjecucion += tiempoEjec;
    bcp->tiempoEspera += tiempoEsp;
    bcp->numCartas = cartas;
    
    guardarBCP(bcp);
}

void guardarBCP(BCP *bcp) {
    if (bcp == NULL) {
        return;
    }
    
    char ruta[100];
    sprintf(ruta, "%s/bcp_%d.txt", RUTA_BCP, bcp->id);
    
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    char timestamp[25];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    FILE *archivo = fopen(ruta, "a");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir/crear el archivo BCP para el proceso %d\n", bcp->id);
        return;
    }
    
    fprintf(archivo, "\n=== ACTUALIZACIÓN BCP [%s] - RONDA %d ===\n", 
            timestamp, rondaActual > 0 ? rondaActual : 0);
    
    fprintf(archivo, "ID: %d\n", bcp->id);
    
    const char *estadoTexto[] = {"NUEVO", "LISTO", "EJECUTANDO", "BLOQUEADO", "TERMINADO"};
    fprintf(archivo, "Estado: %s\n", estadoTexto[bcp->estado]);
    
    fprintf(archivo, "Prioridad: %d\n", bcp->prioridad);
    fprintf(archivo, "Tiempo de ejecución: %d ms\n", bcp->tiempoEjecucion);
    fprintf(archivo, "Tiempo de espera: %d ms\n", bcp->tiempoEspera);
    fprintf(archivo, "Tiempo de bloqueo: %d ms\n", bcp->tiempoBloqueo);
    fprintf(archivo, "Tiempo de E/S restante: %d ms\n", bcp->tiempoES);
    fprintf(archivo, "Tiempo de quantum: %d ms\n", bcp->tiempoQuantum);
    fprintf(archivo, "Tiempo restante: %d ms\n", bcp->tiempoRestante);
    fprintf(archivo, "Número de cartas: %d\n", bcp->numCartas);
    fprintf(archivo, "Puntos acumulados: %d\n", bcp->puntosAcumulados);
    fprintf(archivo, "Turno actual: %s\n", bcp->turnoActual ? "SÍ" : "NO");
    fprintf(archivo, "Veces que se ha apeado: %d\n", bcp->vecesApeo);
    fprintf(archivo, "Cartas comidas: %d\n", bcp->cartasComidas);
    fprintf(archivo, "Intentos fallidos: %d\n", bcp->intentosFallidos);
    fprintf(archivo, "Turnos perdidos: %d\n", bcp->turnosPerdidos);
    fprintf(archivo, "Cambios de estado: %d\n", bcp->cambiosEstado);
    fprintf(archivo, "Tiempo en el estado actual: %d ms\n", bcp->tiempoUltimoEstado);
    fprintf(archivo, "Tiempo desde el último bloqueo: %d ms\n", bcp->tiempoUltimoBloqueo);
    
    fclose(archivo);
}

void liberarBCP(BCP *bcp) {
    if (bcp != NULL) {
        free(bcp);
    }
}

//==============================================================================
// FUNCIONES DE INICIALIZACIÓN Y LIBERACIÓN DE TABLA
//==============================================================================

void inicializarTabla(void) {
    tablaProc.numProcesos = 0;
    tablaProc.procesoActual = -1;
    tablaProc.algoritmoActual = 0;
    tablaProc.numProcesosBloqueados = 0;
    tablaProc.numProcesosListos = 0;
    tablaProc.numProcesosSuspendidos = 0;
    tablaProc.numProcesosTerminados = 0;
    tablaProc.cambiosContexto = 0;
    tablaProc.tiempoTotalEjecucion = 0;
    tablaProc.tiempoTotalEspera = 0;
    tablaProc.tiempoTotalBloqueo = 0;
    tablaProc.tiempoTotalES = 0;
    tablaProc.turnosAsignados = 0;
    tablaProc.turnosCompletados = 0;
    tablaProc.turnosInterrumpidos = 0;
    tablaProc.tiempoInicio = time(NULL);
    tablaProc.tiempoActual = time(NULL);
    tablaProc.usoCPU = 0.0;
    tablaProc.ultimoCambioAlgoritmo = 0;
    
    for (int i = 0; i < 10; i++) {
        tablaProc.procesos[i] = NULL;
    }
    
    printf("Tabla de procesos inicializada\n");
    system("mkdir -p " RUTA_BCP);
}

void liberarTabla(void) {
    for (int i = 0; i < tablaProc.numProcesos; i++) {
        if (tablaProc.procesos[i] != NULL) {
            liberarBCP(tablaProc.procesos[i]);
            tablaProc.procesos[i] = NULL;
        }
    }
    
    tablaProc.numProcesos = 0;
    printf("Tabla de procesos liberada\n");
}

//==============================================================================
// FUNCIONES DE REGISTRO Y GESTIÓN DE PROCESOS
//==============================================================================

void registrarProcesoEnTabla(int id, int estado) {
    if (tablaProc.numProcesos >= 10) {
        printf("Error: La tabla de procesos está llena\n");
        return;
    }
    
    BCP *nuevoBCP = crearBCP(id);
    if (nuevoBCP == NULL) {
        return;
    }
    
    nuevoBCP->estado = estado;
    
    tablaProc.procesos[tablaProc.numProcesos] = nuevoBCP;
    tablaProc.numProcesos++;
    
    switch (estado) {
        case PROC_LISTO:
            tablaProc.numProcesosListos++;
            break;
        case PROC_BLOQUEADO:
            tablaProc.numProcesosBloqueados++;
            break;
        case PROC_TERMINADO:
            tablaProc.numProcesosTerminados++;
            break;
    }
    
    printf("Proceso %d registrado en la tabla (estado: %d)\n", id, estado);
    guardarBCP(nuevoBCP);
}

void actualizarProcesoEnTabla(int id, int nuevoEstado) {
    BCP *bcp = NULL;
    int indice = -1;
    
    for (int i = 0; i < tablaProc.numProcesos; i++) {
        if (tablaProc.procesos[i]->id == id) {
            bcp = tablaProc.procesos[i];
            indice = i;
            break;
        }
    }
    
    if (bcp == NULL) {
        printf("Error: No se encontró el proceso %d en la tabla\n", id);
        return;
    }
    
    // Decrementar contador del estado anterior
    switch (bcp->estado) {
        case PROC_LISTO:
            tablaProc.numProcesosListos--;
            break;
        case PROC_BLOQUEADO:
            tablaProc.numProcesosBloqueados--;
            break;
        default:
            break;
    }
    
    int estadoAnterior = bcp->estado;
    bcp->estado = nuevoEstado;
    
    // Incrementar contador del nuevo estado
    switch (nuevoEstado) {
        case PROC_LISTO:
            tablaProc.numProcesosListos++;
            break;
        case PROC_EJECUTANDO:
            if (tablaProc.procesoActual != -1 && tablaProc.procesoActual != indice) {
                BCP *procesoAnterior = tablaProc.procesos[tablaProc.procesoActual];
                if (procesoAnterior->estado == PROC_EJECUTANDO) {
                    procesoAnterior->estado = PROC_LISTO;
                    tablaProc.numProcesosListos++;
                    tablaProc.cambiosContexto++;
                }
            }
            tablaProc.procesoActual = indice;
            break;
        case PROC_BLOQUEADO:
            tablaProc.numProcesosBloqueados++;
            break;
        case PROC_TERMINADO:
            tablaProc.numProcesosTerminados++;
            break;
        default:
            break;
    }
    
    printf("Estado del proceso %d actualizado: %d -> %d\n", id, estadoAnterior, nuevoEstado);
    
    bcp->cambiosEstado++;
    bcp->tiempoUltimoEstado = 0;
    
    if (nuevoEstado == PROC_BLOQUEADO) {
        bcp->tiempoUltimoBloqueo = 0;
    }
    
    guardarBCP(bcp);
    
    const char *estadosTexto[] = {"NUEVO", "LISTO", "EJECUTANDO", "BLOQUEADO", "TERMINADO"};
    registrarEvento("Proceso %d cambió de estado: %s -> %s", 
                   id, 
                   (estadoAnterior >= 0 && estadoAnterior <= 4) ? estadosTexto[estadoAnterior] : "DESCONOCIDO",
                   (nuevoEstado >= 0 && nuevoEstado <= 4) ? estadosTexto[nuevoEstado] : "DESCONOCIDO");
}

void cambiarEstadoProceso(int id, int nuevoEstado) {
    actualizarProcesoEnTabla(id, nuevoEstado);
}

void marcarProcesoTerminado(int id) {
    actualizarProcesoEnTabla(id, PROC_TERMINADO);
}

//==============================================================================
// FUNCIONES DE ASIGNACIÓN Y CONTROL DE TIEMPO
//==============================================================================

void asignarQuantum(int id, int quantum) {
    BCP *bcp = NULL;
    
    for (int i = 0; i < tablaProc.numProcesos; i++) {
        if (tablaProc.procesos[i]->id == id) {
            bcp = tablaProc.procesos[i];
            break;
        }
    }
    
    if (bcp == NULL) {
        printf("Error: No se encontró el proceso %d en la tabla\n", id);
        return;
    }
    
    bcp->tiempoQuantum = quantum;
    bcp->tiempoRestante = quantum;
    
    printf("Quantum asignado al proceso %d: %d ms\n", id, quantum);
    
    tablaProc.turnosAsignados++;
    guardarBCP(bcp);
}

void aumentarTiempoEjecucion(int id, int tiempo) {
    BCP *bcp = NULL;
    
    for (int i = 0; i < tablaProc.numProcesos; i++) {
        if (tablaProc.procesos[i]->id == id) {
            bcp = tablaProc.procesos[i];
            break;
        }
    }
    
    if (bcp == NULL) {
        printf("Error: No se encontró el proceso %d en la tabla\n", id);
        return;
    }
    
    bcp->tiempoEjecucion += tiempo;
    tablaProc.tiempoTotalEjecucion += tiempo;
    
    if (tablaProc.algoritmoActual == 1) {
        bcp->tiempoRestante -= tiempo;
        if (bcp->tiempoRestante < 0) {
            bcp->tiempoRestante = 0;
        }
    }
    
    guardarBCP(bcp);
}

void aumentarTiempoEspera(int id, int tiempo) {
    BCP *bcp = NULL;
    
    for (int i = 0; i < tablaProc.numProcesos; i++) {
        if (tablaProc.procesos[i]->id == id) {
            bcp = tablaProc.procesos[i];
            break;
        }
    }
    
    if (bcp == NULL) {
        printf("Error: No se encontró el proceso %d en la tabla\n", id);
        return;
    }
    
    bcp->tiempoEspera += tiempo;
    tablaProc.tiempoTotalEspera += tiempo;
    
    guardarBCP(bcp);
}

void aumentarTiempoBloqueo(int id, int tiempo) {
    BCP *bcp = NULL;
    
    for (int i = 0; i < tablaProc.numProcesos; i++) {
        if (tablaProc.procesos[i]->id == id) {
            bcp = tablaProc.procesos[i];
            break;
        }
    }
    
    if (bcp == NULL) {
        printf("Error: No se encontró el proceso %d en la tabla\n", id);
        return;
    }
    
    bcp->tiempoBloqueo += tiempo;
    tablaProc.tiempoTotalBloqueo += tiempo;
    bcp->tiempoUltimoBloqueo += tiempo;
    
    guardarBCP(bcp);
}

//==============================================================================
// FUNCIONES DE CONTROL DE CONTEXTO Y ACCESO
//==============================================================================

void registrarCambioContexto(void) {
    tablaProc.cambiosContexto++;
}

BCP* obtenerBCPActual(void) {
    if (tablaProc.procesoActual == -1) {
        return NULL;
    }
    
    return tablaProc.procesos[tablaProc.procesoActual];
}

//==============================================================================
// FUNCIONES DE VISUALIZACIÓN
//==============================================================================

void imprimirEstadisticasTabla(void) {
    FILE *archivo;
    const char *nombreArchivo = "estadisticas_procesos.txt";
    
    archivo = fopen(nombreArchivo, "w");
    if (archivo == NULL) {
        printf("Error: No se pudo crear el archivo %s\n", nombreArchivo);
        return;
    }
    
    printf("\n=== ESTADÍSTICAS DE LA TABLA DE PROCESOS ===\n");
    fprintf(archivo, "=== ESTADÍSTICAS DE LA TABLA DE PROCESOS ===\n");
    
    printf("Número de procesos: %d\n", tablaProc.numProcesos);
    fprintf(archivo, "Número de procesos: %d\n", tablaProc.numProcesos);
    
    printf("Procesos listos: %d\n", tablaProc.numProcesosListos);
    fprintf(archivo, "Procesos listos: %d\n", tablaProc.numProcesosListos);
    
    printf("Procesos bloqueados: %d\n", tablaProc.numProcesosBloqueados);
    fprintf(archivo, "Procesos bloqueados: %d\n", tablaProc.numProcesosBloqueados);
    
    printf("Procesos terminados: %d\n", tablaProc.numProcesosTerminados);
    fprintf(archivo, "Procesos terminados: %d\n", tablaProc.numProcesosTerminados);
    
    printf("Cambios de contexto: %d\n", tablaProc.cambiosContexto);
    fprintf(archivo, "Cambios de contexto: %d\n", tablaProc.cambiosContexto);
    
    printf("Tiempo total de ejecución: %d ms\n", tablaProc.tiempoTotalEjecucion);
    fprintf(archivo, "Tiempo total de ejecución: %d ms\n", tablaProc.tiempoTotalEjecucion);
    
    printf("Tiempo total de espera: %d ms\n", tablaProc.tiempoTotalEspera);
    fprintf(archivo, "Tiempo total de espera: %d ms\n", tablaProc.tiempoTotalEspera);
    
    printf("Tiempo total de bloqueo: %d ms\n", tablaProc.tiempoTotalBloqueo);
    fprintf(archivo, "Tiempo total de bloqueo: %d ms\n", tablaProc.tiempoTotalBloqueo);
    
    printf("Tiempo total en E/S: %d ms\n", tablaProc.tiempoTotalES);
    fprintf(archivo, "Tiempo total en E/S: %d ms\n", tablaProc.tiempoTotalES);
    
    printf("Turnos asignados: %d\n", tablaProc.turnosAsignados);
    fprintf(archivo, "Turnos asignados: %d\n", tablaProc.turnosAsignados);
    
    printf("Turnos completados: %d\n", tablaProc.turnosCompletados);
    fprintf(archivo, "Turnos completados: %d\n", tablaProc.turnosCompletados);
    
    printf("Turnos interrumpidos: %d\n", tablaProc.turnosInterrumpidos);
    fprintf(archivo, "Turnos interrumpidos: %d\n", tablaProc.turnosInterrumpidos);
    
    tablaProc.tiempoActual = time(NULL);
    double tiempoTotal = difftime(tablaProc.tiempoActual, tablaProc.tiempoInicio);
    
    printf("Tiempo total de simulación: %.2f segundos\n", tiempoTotal);
    fprintf(archivo, "Tiempo total de simulación: %.2f segundos\n", tiempoTotal);
    
    if (tiempoTotal > 0) {
        tablaProc.usoCPU = (float)tablaProc.tiempoTotalEjecucion / (tiempoTotal * 1000) * 100;
        printf("Uso de CPU: %.2f%%\n", tablaProc.usoCPU);
        fprintf(archivo, "Uso de CPU: %.2f%%\n", tablaProc.usoCPU);
    }
    
    printf("\nDETALLES DE PROCESOS:\n");
    fprintf(archivo, "\nDETALLES DE PROCESOS:\n");
    
    for (int i = 0; i < tablaProc.numProcesos; i++) {
        BCP *bcp = tablaProc.procesos[i];
        
        const char *estadoTexto[] = {"NUEVO", "LISTO", "EJECUTANDO", "BLOQUEADO", "TERMINADO"};
        
        printf("Proceso %d: %s, Ejecución: %d ms, Espera: %d ms, Cartas: %d\n",
               bcp->id, estadoTexto[bcp->estado], bcp->tiempoEjecucion, bcp->tiempoEspera, bcp->numCartas);
        fprintf(archivo, "Proceso %d: %s, Ejecución: %d ms, Espera: %d ms, Cartas: %d\n",
                bcp->id, estadoTexto[bcp->estado], bcp->tiempoEjecucion, bcp->tiempoEspera, bcp->numCartas);
    }
    
    printf("===========================================\n");
    fprintf(archivo, "===========================================\n");
    
    fclose(archivo);
    printf("Estadísticas de la tabla de procesos guardadas en '%s'\n", nombreArchivo);
}