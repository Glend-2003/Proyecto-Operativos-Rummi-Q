#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "procesos.h"

// Variable global para la tabla de procesos
static TablaProc tablaProc;

// Ruta para guardar los archivos BCP
#define RUTA_BCP "bcp/"

// Funciones para el manejo del BCP

// Crear un nuevo BCP
void guardarBCP(BCP *bcp) {
    if (bcp == NULL) {
        return;
    }
    
    // Crear la ruta del archivo
    char ruta[100];
    sprintf(ruta, "%s/bcp_%d.txt", RUTA_BCP, bcp->id);
    
    // Abrir el archivo (o crearlo si no existe)
    FILE *archivo = fopen(ruta, "w");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir/crear el archivo BCP para el proceso %d\n", bcp->id);
        return;
    }
    
    // Guardar los datos del BCP en el archivo
    fprintf(archivo, "=== BLOQUE DE CONTROL DE PROCESO ===\n");
    fprintf(archivo, "ID: %d\n", bcp->id);
    
    // Estado como texto
    const char *estadoTexto[] = {"NUEVO", "LISTO", "EJECUTANDO", "BLOQUEADO", "TERMINADO"};
    fprintf(archivo, "Estado: %s\n", estadoTexto[bcp->estado]);
    
    fprintf(archivo, "Prioridad: %d\n", bcp->prioridad);
    
    // Tiempo de creación como texto legible
    char tiempoCreacion[50];
    strftime(tiempoCreacion, sizeof(tiempoCreacion), "%Y-%m-%d %H:%M:%S", localtime(&bcp->tiempoCreacion));
    fprintf(archivo, "Tiempo de creación: %s\n", tiempoCreacion);
    
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
    
    // Cerrar el archivo y forzar la escritura al disco
    fclose(archivo);
}

// Actualizar los datos de un BCP
void actualizarBCP(BCP *bcp, int estado, int tiempoEjec, int tiempoEsp, int cartas) {
    if (bcp == NULL) {
        return;
    }
    
    // Si el estado cambió, registrar el cambio
    if (bcp->estado != estado) {
        bcp->cambiosEstado++;
        bcp->tiempoUltimoEstado = 0;
        
        // Si el nuevo estado es PROC_BLOQUEADO, actualizar tiempoUltimoBloqueo
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
    
    // Guardar el BCP actualizado
    guardarBCP(bcp);
}

// Liberar la memoria de un BCP
void liberarBCP(BCP *bcp) {
    if (bcp != NULL) {
        free(bcp);
    }
}
BCP* crearBCP(int id) {
    BCP* nuevoBCP = (BCP*)malloc(sizeof(BCP));
    if (nuevoBCP == NULL) {
        printf("Error: No se pudo asignar memoria para el BCP\n");
        return NULL;
    }
    
    // Inicializar todas las variables del BCP
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
// Funciones para el manejo de la tabla de procesos

// Inicializar la tabla de procesos
void inicializarTabla(void) {
    // Inicializar todos los campos de la tabla
    tablaProc.numProcesos = 0;
    tablaProc.procesoActual = -1;
    tablaProc.algoritmoActual = 0; // FCFS por defecto
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
    
    // Inicializar el array de procesos
    for (int i = 0; i < 10; i++) {
        tablaProc.procesos[i] = NULL;
    }
    
    printf("Tabla de procesos inicializada\n");
    
    // Crear directorio para BCPs si no existe
    system("mkdir -p " RUTA_BCP);
}

// Registrar un nuevo proceso en la tabla
void registrarProcesoEnTabla(int id, int estado) {
    if (tablaProc.numProcesos >= 10) {
        printf("Error: La tabla de procesos está llena\n");
        return;
    }
    
    // Crear un nuevo BCP para el proceso
    BCP *nuevoBCP = crearBCP(id);
    if (nuevoBCP == NULL) {
        return;
    }
    
    // Actualizar el estado inicial
    nuevoBCP->estado = estado;
    
    // Añadir a la tabla
    tablaProc.procesos[tablaProc.numProcesos] = nuevoBCP;
    tablaProc.numProcesos++;
    
    // Actualizar contadores según el estado
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
    
    // Guardar el BCP
    guardarBCP(nuevoBCP);
}

// Actualizar el estado de un proceso en la tabla
void actualizarProcesoEnTabla(int id, int nuevoEstado) {
    // Buscar el proceso en la tabla
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
    
    // Decrementar el contador del estado anterior
    switch (bcp->estado) {
        case PROC_NUEVO:
            // El estado NUEVO no tiene contador específico
            break;
        case PROC_LISTO:
            tablaProc.numProcesosListos--;
            break;
        case PROC_EJECUTANDO:
            // Si tuvieras un contador para procesos en ejecución, lo decrementarías aquí
            break;
        case PROC_BLOQUEADO:
            tablaProc.numProcesosBloqueados--;
            break;
        case PROC_TERMINADO:
            // Normalmente no decrementamos contador de terminados
            break;
        default:
            printf("Error: Estado desconocido %d\n", bcp->estado);
            break;
    }
    
    // Actualizar el estado
    int estadoAnterior = bcp->estado;
    bcp->estado = nuevoEstado;
    
    // Incrementar el contador del nuevo estado
    switch (nuevoEstado) {
        case PROC_NUEVO:
            // El estado NUEVO no tiene contador específico
            break;
        case PROC_LISTO:
            tablaProc.numProcesosListos++;
            break;
        case PROC_EJECUTANDO:
            // Si hay un proceso en ejecución, cambiarlo a LISTO
            if (tablaProc.procesoActual != -1 && tablaProc.procesoActual != indice) {
                BCP *procesoAnterior = tablaProc.procesos[tablaProc.procesoActual];
                if (procesoAnterior->estado == PROC_EJECUTANDO) {
                    procesoAnterior->estado = PROC_LISTO;
                    tablaProc.numProcesosListos++;
                    
                    // Registrar cambio de contexto
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
            printf("Error: Estado desconocido %d\n", nuevoEstado);
            break;
    }
    
    printf("Estado del proceso %d actualizado: %d -> %d\n", id, estadoAnterior, nuevoEstado);
    
    // Actualizar el BCP para reflejar el cambio de estado
    bcp->cambiosEstado++;
    bcp->tiempoUltimoEstado = 0;
    
    // Si pasa a bloqueado, actualizar tiempo de último bloqueo
    if (nuevoEstado == PROC_BLOQUEADO) {
        bcp->tiempoUltimoBloqueo = 0;
    }
    
    // Guardar el BCP actualizado
    guardarBCP(bcp);
    
    // Registrar el evento en el log
    const char *estadosTexto[] = {"NUEVO", "LISTO", "EJECUTANDO", "BLOQUEADO", "TERMINADO"};
    registrarEvento("Proceso %d cambió de estado: %s -> %s", 
                   id, 
                   (estadoAnterior >= 0 && estadoAnterior <= 4) ? estadosTexto[estadoAnterior] : "DESCONOCIDO",
                   (nuevoEstado >= 0 && nuevoEstado <= 4) ? estadosTexto[nuevoEstado] : "DESCONOCIDO");
}

// Cambiar el estado de un proceso
void cambiarEstadoProceso(int id, int nuevoEstado) {
    actualizarProcesoEnTabla(id, nuevoEstado);
}

// Asignar un quantum a un proceso (para Round Robin)
void asignarQuantum(int id, int quantum) {
    // Buscar el proceso en la tabla
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
    
    // Asignar el quantum
    bcp->tiempoQuantum = quantum;
    bcp->tiempoRestante = quantum;
    
    printf("Quantum asignado al proceso %d: %d ms\n", id, quantum);
    
    // Incrementar contador de turnos asignados
    tablaProc.turnosAsignados++;
    
    // Guardar el BCP actualizado
    guardarBCP(bcp);
}

// Marcar un proceso como terminado
void marcarProcesoTerminado(int id) {
    actualizarProcesoEnTabla(id, PROC_TERMINADO);
}

// Aumentar el tiempo de ejecución de un proceso
void aumentarTiempoEjecucion(int id, int tiempo) {
    // Buscar el proceso en la tabla
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
    
    // Aumentar el tiempo de ejecución
    bcp->tiempoEjecucion += tiempo;
    
    // Actualizar tiempo total en la tabla
    tablaProc.tiempoTotalEjecucion += tiempo;
    
    // Actualizar tiempo restante si está en modo Round Robin
    if (tablaProc.algoritmoActual == 1) { // Round Robin
        bcp->tiempoRestante -= tiempo;
        if (bcp->tiempoRestante < 0) {
            bcp->tiempoRestante = 0;
        }
    }
    
    // Guardar el BCP actualizado
    guardarBCP(bcp);
}

// Aumentar el tiempo de espera de un proceso
void aumentarTiempoEspera(int id, int tiempo) {
    // Buscar el proceso en la tabla
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
    
    
    // Aumentar el tiempo de espera
    bcp->tiempoEspera += tiempo;
    
    // Actualizar tiempo total en la tabla
    tablaProc.tiempoTotalEspera += tiempo;
    
    // Guardar el BCP actualizado
    guardarBCP(bcp);
}

// Aumentar el tiempo de bloqueo de un proceso
void aumentarTiempoBloqueo(int id, int tiempo) {
    // Buscar el proceso en la tabla
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
    
    // Aumentar el tiempo de bloqueo
    bcp->tiempoBloqueo += tiempo;
    
    // Actualizar tiempo total en la tabla
    tablaProc.tiempoTotalBloqueo += tiempo;
    
    // Actualizar tiempoUltimoBloqueo
    bcp->tiempoUltimoBloqueo += tiempo;
    
    // Guardar el BCP actualizado
    guardarBCP(bcp);
}

// Registrar un cambio de contexto
void registrarCambioContexto(void) {
    tablaProc.cambiosContexto++;
}

// Obtener el BCP del proceso actual
BCP* obtenerBCPActual(void) {
    if (tablaProc.procesoActual == -1) {
        return NULL;
    }
    
    return tablaProc.procesos[tablaProc.procesoActual];
}

// Imprimir estadísticas de la tabla de procesos
void imprimirEstadisticasTabla(void) {
    FILE *archivo;
    const char *nombreArchivo = "estadisticas_procesos.txt";
    
    // Abrir archivo para escritura
    archivo = fopen(nombreArchivo, "w");
    if (archivo == NULL) {
        printf("Error: No se pudo crear el archivo %s\n", nombreArchivo);
        return;
    }
    
    // Imprimir y guardar en archivo las estadísticas
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
    
    // Calcular tiempo total de la simulación
    tablaProc.tiempoActual = time(NULL);
    double tiempoTotal = difftime(tablaProc.tiempoActual, tablaProc.tiempoInicio);
    
    printf("Tiempo total de simulación: %.2f segundos\n", tiempoTotal);
    fprintf(archivo, "Tiempo total de simulación: %.2f segundos\n", tiempoTotal);
    
    // Calcular uso de CPU
    if (tiempoTotal > 0) {
        tablaProc.usoCPU = (float)tablaProc.tiempoTotalEjecucion / (tiempoTotal * 1000) * 100;
        printf("Uso de CPU: %.2f%%\n", tablaProc.usoCPU);
        fprintf(archivo, "Uso de CPU: %.2f%%\n", tablaProc.usoCPU);
    }
    
    // Imprimir detalles de cada proceso
    printf("\nDETALLES DE PROCESOS:\n");
    fprintf(archivo, "\nDETALLES DE PROCESOS:\n");
    
    for (int i = 0; i < tablaProc.numProcesos; i++) {
        BCP *bcp = tablaProc.procesos[i];
        
        // Obtener nombre del estado
        const char *estadoTexto[] = {"NUEVO", "LISTO", "EJECUTANDO", "BLOQUEADO", "TERMINADO"};
        
        printf("Proceso %d: %s, Ejecución: %d ms, Espera: %d ms, Cartas: %d\n",
               bcp->id, estadoTexto[bcp->estado], bcp->tiempoEjecucion, bcp->tiempoEspera, bcp->numCartas);
        fprintf(archivo, "Proceso %d: %s, Ejecución: %d ms, Espera: %d ms, Cartas: %d\n",
                bcp->id, estadoTexto[bcp->estado], bcp->tiempoEjecucion, bcp->tiempoEspera, bcp->numCartas);
    }
    
    printf("===========================================\n");
    fprintf(archivo, "===========================================\n");
    
    // Cerrar el archivo
    fclose(archivo);
    printf("Estadísticas de la tabla de procesos guardadas en '%s'\n", nombreArchivo);
}

// Liberar la memoria de la tabla de procesos
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