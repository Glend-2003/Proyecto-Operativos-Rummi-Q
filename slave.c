#include <stdio.h>
#include <stdlib.h>
#include <pvm3.h>
#include <string.h>
#include <time.h>
#include "procesos.h"
#include "juego.h"

#define MSGTAG_BCP 1
#define MSGTAG_CONFIRM 2
#define MSGTAG_CAMBIO_ALG 3
#define MSGTAG_CONFIRM_ALG 4

// Variables globales
int tid_master;
FILE *bitacoraSlave;
TablaProc tablaProcesos;  // Tabla local del slave

// Condiciones para cambio de algoritmo
typedef struct {
    int numProcesosBloqueados;
    int tiempoPromedioEspera;
    int cambiosContexto;
} CondicionesAlgoritmo;

// Función para obtener timestamp
char* obtenerTimestamp() {
    static char timestamp[25];
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    return timestamp;
}

// Función para recibir y procesar BCP
void procesarBCP() {
    int bufid;
    BCP bcpRecibido;
    
    // Recibir BCP
    bufid = pvm_recv(tid_master, MSGTAG_BCP);
    if (bufid < 0) {
        return;
    }
    
    // Desempaquetar BCP
    pvm_upkint(&bcpRecibido.id, 1, 1);
    pvm_upkint(&bcpRecibido.estado, 1, 1);
    pvm_upkint(&bcpRecibido.prioridad, 1, 1);
    pvm_upkint(&bcpRecibido.tiempoEjecucion, 1, 1);
    pvm_upkint(&bcpRecibido.tiempoEspera, 1, 1);
    pvm_upkint(&bcpRecibido.tiempoBloqueo, 1, 1);
    pvm_upkint(&bcpRecibido.tiempoES, 1, 1);
    pvm_upkint(&bcpRecibido.tiempoQuantum, 1, 1);
    pvm_upkint(&bcpRecibido.tiempoRestante, 1, 1);
    pvm_upkint(&bcpRecibido.numCartas, 1, 1);
    pvm_upkint(&bcpRecibido.puntosAcumulados, 1, 1);
    pvm_upkint(&bcpRecibido.turnoActual, 1, 1);
    pvm_upkint(&bcpRecibido.vecesApeo, 1, 1);
    pvm_upkint(&bcpRecibido.cartasComidas, 1, 1);
    pvm_upkint(&bcpRecibido.intentosFallidos, 1, 1);
    pvm_upkint(&bcpRecibido.turnosPerdidos, 1, 1);
    pvm_upkint(&bcpRecibido.cambiosEstado, 1, 1);
    pvm_upkint(&bcpRecibido.tiempoUltimoEstado, 1, 1);
    pvm_upkint(&bcpRecibido.tiempoUltimoBloqueo, 1, 1);
    
    // Actualizar tabla de procesos
    actualizarTablaConBCP(&bcpRecibido);
    
    // Enviar confirmación al master
    char mensaje[256];
    sprintf(mensaje, "BCP del proceso %d recibido y procesado", bcpRecibido.id);
    
    pvm_initsend(PvmDataDefault);
    pvm_pkstr(mensaje);
    pvm_send(tid_master, MSGTAG_CONFIRM);
    
    // Log en bitácora
    fprintf(bitacoraSlave, "[%s] BCP recibido: Proceso %d, Estado %d, Cartas %d\n",
            obtenerTimestamp(), bcpRecibido.id, bcpRecibido.estado, bcpRecibido.numCartas);
    fflush(bitacoraSlave);
}

// Función para actualizar la tabla de procesos con el BCP recibido
void actualizarTablaConBCP(BCP *bcp) {
    int encontrado = 0;
    
    // Buscar si el proceso ya existe en la tabla
    for (int i = 0; i < tablaProcesos.numProcesos; i++) {
        if (tablaProcesos.procesos[i]->id == bcp->id) {
            // Actualizar el BCP existente
            *tablaProcesos.procesos[i] = *bcp;
            encontrado = 1;
            break;
        }
    }
    
    // Si no existe, agregarlo
    if (!encontrado && tablaProcesos.numProcesos < 10) {
        tablaProcesos.procesos[tablaProcesos.numProcesos] = malloc(sizeof(BCP));
        *tablaProcesos.procesos[tablaProcesos.numProcesos] = *bcp;
        tablaProcesos.numProcesos++;
    }
    
    // Actualizar estadísticas de la tabla
    actualizarEstadisticasTabla();
}

// Función para actualizar estadísticas de la tabla
void actualizarEstadisticasTabla() {
    tablaProcesos.numProcesosBloqueados = 0;
    tablaProcesos.numProcesosListos = 0;
    tablaProcesos.numProcesosTerminados = 0;
    tablaProcesos.tiempoTotalEspera = 0;
    tablaProcesos.tiempoTotalEjecucion = 0;
    
    for (int i = 0; i < tablaProcesos.numProcesos; i++) {
        BCP *bcp = tablaProcesos.procesos[i];
        
        switch (bcp->estado) {
            case PROC_BLOQUEADO:
                tablaProcesos.numProcesosBloqueados++;
                break;
            case PROC_LISTO:
                tablaProcesos.numProcesosListos++;
                break;
            case PROC_TERMINADO:
                tablaProcesos.numProcesosTerminados++;
                break;
        }
        
        tablaProcesos.tiempoTotalEspera += bcp->tiempoEspera;
        tablaProcesos.tiempoTotalEjecucion += bcp->tiempoEjecucion;
    }
}

// Función para verificar condiciones de cambio de algoritmo
void verificarCondicionesCambio() {
    int deberíaCambiar = 0;
    int algoritmoActual = tablaProcesos.algoritmoActual;
    int nuevoAlgoritmo = algoritmoActual;
    char razon[256] = "";
    
    // Condición 1: Muchos procesos bloqueados (más del 50%)
    if (tablaProcesos.numProcesos > 0) {
        float porcentajeBloqueados = (float)tablaProcesos.numProcesosBloqueados / tablaProcesos.numProcesos;
        if (porcentajeBloqueados > 0.5) {
            deberíaCambiar = 1;
            sprintf(razon, "Alto porcentaje de procesos bloqueados: %.1f%%", porcentajeBloqueados * 100);
            nuevoAlgoritmo = (algoritmoActual == ALG_FCFS) ? ALG_RR : ALG_FCFS;
        }
    }
    
    // Condición 2: Tiempo de espera promedio alto (más de 5 segundos)
    if (!deberíaCambiar && tablaProcesos.numProcesos > 0) {
        int tiempoPromedioEspera = tablaProcesos.tiempoTotalEspera / tablaProcesos.numProcesos;
        if (tiempoPromedioEspera > 5000) {  // 5000 ms = 5 segundos
            deberíaCambiar = 1;
            sprintf(razon, "Tiempo de espera promedio alto: %d ms", tiempoPromedioEspera);
            nuevoAlgoritmo = ALG_RR;  // Round Robin es mejor para reducir tiempo de espera
        }
    }
    
    // Condición 3: Muchos cambios de contexto en Round Robin
    if (!deberíaCambiar && algoritmoActual == ALG_RR) {
        if (tablaProcesos.cambiosContexto > 100) {
            deberíaCambiar = 1;
            sprintf(razon, "Demasiados cambios de contexto: %d", tablaProcesos.cambiosContexto);
            nuevoAlgoritmo = ALG_FCFS;  // FCFS reduce cambios de contexto
        }
    }
    
    // Si debe cambiar, notificar al master
    if (deberíaCambiar) {
        pvm_initsend(PvmDataDefault);
        pvm_pkint(&nuevoAlgoritmo, 1, 1);
        pvm_pkstr(razon);
        pvm_send(tid_master, MSGTAG_CAMBIO_ALG);
        
        // Log en bitácora
        fprintf(bitacoraSlave, "[%s] Solicitando cambio de algoritmo: %s\n",
                obtenerTimestamp(), razon);
        fflush(bitacoraSlave);
        
        // Esperar confirmación
        int bufid = pvm_recv(tid_master, MSGTAG_CONFIRM_ALG);
        if (bufid > 0) {
            int algoritmoConfirmado;
            pvm_upkint(&algoritmoConfirmado, 1, 1);
            
            fprintf(bitacoraSlave, "[%s] Cambio de algoritmo confirmado: %s\n",
                    obtenerTimestamp(), 
                    algoritmoConfirmado == ALG_FCFS ? "FCFS" : "Round Robin");
            fflush(bitacoraSlave);
            
            tablaProcesos.algoritmoActual = algoritmoConfirmado;
            tablaProcesos.cambiosContexto = 0;  // Resetear contador
        }
    }
}

// Función para guardar la tabla de procesos en archivo
void guardarTablaProcesos() {
    FILE *archivo = fopen("tabla_procesos.txt", "w");
    if (archivo == NULL) return;
    
    fprintf(archivo, "=== TABLA DE PROCESOS - SLAVE TID %d ===\n", pvm_mytid());
    fprintf(archivo, "Fecha: %s\n\n", obtenerTimestamp());
    
    fprintf(archivo, "Número de procesos: %d\n", tablaProcesos.numProcesos);
    fprintf(archivo, "Procesos listos: %d\n", tablaProcesos.numProcesosListos);
    fprintf(archivo, "Procesos bloqueados: %d\n", tablaProcesos.numProcesosBloqueados);
    fprintf(archivo, "Procesos terminados: %d\n", tablaProcesos.numProcesosTerminados);
    fprintf(archivo, "Tiempo total de espera: %d ms\n", tablaProcesos.tiempoTotalEspera);
    fprintf(archivo, "Tiempo total de ejecución: %d ms\n", tablaProcesos.tiempoTotalEjecucion);
    fprintf(archivo, "Cambios de contexto: %d\n", tablaProcesos.cambiosContexto);
    fprintf(archivo, "Algoritmo actual: %s\n", 
            tablaProcesos.algoritmoActual == ALG_FCFS ? "FCFS" : "Round Robin");
    
    fprintf(archivo, "\nDetalle de procesos:\n");
    for (int i = 0; i < tablaProcesos.numProcesos; i++) {
        BCP *bcp = tablaProcesos.procesos[i];
        fprintf(archivo, "  Proceso %d: Estado=%d, Cartas=%d, TiempoEjec=%d, TiempoEsp=%d\n",
                bcp->id, bcp->estado, bcp->numCartas, bcp->tiempoEjecucion, bcp->tiempoEspera);
    }
    
    fclose(archivo);
}

int main(int argc, char *argv[]) {
    int mytid;
    int running = 1;
    
    // Inicializar PVM
    mytid = pvm_mytid();
    if (mytid < 0) {
        printf("Error al inicializar PVM en slave\n");
        return 1;
    }
    
    // Obtener TID del master (parent)
    tid_master = pvm_parent();
    
    printf("Slave iniciado con TID: %d, Master TID: %d\n", mytid, tid_master);
    
    // Abrir bitácora
    char nombreBitacora[50];
    sprintf(nombreBitacora, "bitacoraSlave_%d.dat", mytid);
    bitacoraSlave = fopen(nombreBitacora, "a");
    if (bitacoraSlave == NULL) {
        printf("Error al abrir bitácora\n");
        return 1;
    }
    
    fprintf(bitacoraSlave, "\n[%s] === SLAVE INICIADO (TID=%d) ===\n", 
            obtenerTimestamp(), mytid);
    fflush(bitacoraSlave);
    
    // Inicializar tabla de procesos
    memset(&tablaProcesos, 0, sizeof(TablaProc));
    tablaProcesos.algoritmoActual = ALG_FCFS;
    
    // Bucle principal
    while (running) {
        // Verificar si hay mensajes
        int bufid = pvm_probe(-1, -1);
        
        if (bufid > 0) {
            int msgtag, src;
            pvm_bufinfo(bufid, NULL, &msgtag, &src);
            
            if (msgtag == MSGTAG_BCP) {
                procesarBCP();
                verificarCondicionesCambio();
                guardarTablaProcesos();
            } else if (msgtag == 0) {
                // Mensaje especial para terminar
                running = 0;
            }
        }
        
        usleep(100000);  // 100ms
    }
    
    // Cerrar bitácora
    fprintf(bitacoraSlave, "[%s] === SLAVE FINALIZADO ===\n", obtenerTimestamp());
    fclose(bitacoraSlave);
    
    // Liberar memoria
    for (int i = 0; i < tablaProcesos.numProcesos; i++) {
        if (tablaProcesos.procesos[i] != NULL) {
            free(tablaProcesos.procesos[i]);
        }
    }
    
    // Salir de PVM
    pvm_exit();
    
    return 0;
}
