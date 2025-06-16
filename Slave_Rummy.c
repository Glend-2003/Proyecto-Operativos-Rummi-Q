#define _GNU_SOURCE
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pvm3.h"
#include <time.h>

// Definir _GNU_SOURCE para funciones como strdup y usleep


// Estructuras del juego Rummy (mismas que en Master)
typedef struct {
    int id;
    int estado;
    int prioridad;
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
    int algoritmoActual;
} BCP;

typedef struct {
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
} TablaProc;

// Variables globales del Slave
TablaProc tablaProc;
BCP ultimosBCP[4]; // Mantener historial de los últimos BCP recibidos
int contadorRondas = 0;

void obtenerFechaActual(char *buffer, size_t size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void escribirBitacoraSlave(const char *evento, const char *detalles) {
    FILE *archivo = fopen("bitacoraSlave.dat", "a");
    if (archivo == NULL) {
        perror("Error al abrir bitacoraSlave.dat");
        return;
    }
    
    char timestamp[25];
    obtenerFechaActual(timestamp, sizeof(timestamp));
    
    fprintf(archivo, "[%s] %s: %s\n", timestamp, evento, detalles);
    fclose(archivo);
}

BCP parsearBCP(const char *contenidoBCP) {
    BCP bcp;
    memset(&bcp, 0, sizeof(BCP));
    
    // Parser simple para el formato del BCP
    char linea[256];
    char *contenido = strdup(contenidoBCP);
    char *token = strtok(contenido, "\n");
    
    while (token != NULL) {
        if (strncmp(token, "ID: ", 4) == 0) {
            bcp.id = atoi(token + 4);
        } else if (strncmp(token, "Estado: ", 8) == 0) {
            bcp.estado = atoi(token + 8);
        } else if (strncmp(token, "Prioridad: ", 11) == 0) {
            bcp.prioridad = atoi(token + 11);
        } else if (strncmp(token, "TiempoEjecucion: ", 17) == 0) {
            bcp.tiempoEjecucion = atoi(token + 17);
        } else if (strncmp(token, "TiempoEspera: ", 14) == 0) {
            bcp.tiempoEspera = atoi(token + 14);
        } else if (strncmp(token, "TiempoBloqueo: ", 15) == 0) {
            bcp.tiempoBloqueo = atoi(token + 15);
        } else if (strncmp(token, "TiempoES: ", 10) == 0) {
            bcp.tiempoES = atoi(token + 10);
        } else if (strncmp(token, "TiempoQuantum: ", 15) == 0) {
            bcp.tiempoQuantum = atoi(token + 15);
        } else if (strncmp(token, "TiempoRestante: ", 16) == 0) {
            bcp.tiempoRestante = atoi(token + 16);
        } else if (strncmp(token, "NumCartas: ", 11) == 0) {
            bcp.numCartas = atoi(token + 11);
        } else if (strncmp(token, "PuntosAcumulados: ", 18) == 0) {
            bcp.puntosAcumulados = atoi(token + 18);
        } else if (strncmp(token, "TurnoActual: ", 13) == 0) {
            bcp.turnoActual = atoi(token + 13);
        } else if (strncmp(token, "VecesApeo: ", 11) == 0) {
            bcp.vecesApeo = atoi(token + 11);
        } else if (strncmp(token, "CartasComidas: ", 15) == 0) {
            bcp.cartasComidas = atoi(token + 15);
        } else if (strncmp(token, "IntentosFallidos: ", 18) == 0) {
            bcp.intentosFallidos = atoi(token + 18);
        } else if (strncmp(token, "TurnosPerdidos: ", 16) == 0) {
            bcp.turnosPerdidos = atoi(token + 16);
        } else if (strncmp(token, "CambiosEstado: ", 15) == 0) {
            bcp.cambiosEstado = atoi(token + 15);
        } else if (strncmp(token, "TiempoUltimoEstado: ", 20) == 0) {
            bcp.tiempoUltimoEstado = atoi(token + 20);
        } else if (strncmp(token, "TiempoUltimoBloqueo: ", 21) == 0) {
            bcp.tiempoUltimoBloqueo = atoi(token + 21);
        } else if (strncmp(token, "AlgoritmoActual: ", 17) == 0) {
            bcp.algoritmoActual = atoi(token + 17);
        }
        
        token = strtok(NULL, "\n");
    }
    
    free(contenido);
    return bcp;
}

void actualizarTablaProc(BCP *bcp, int ronda) {
    // Actualizar datos generales
    tablaProc.tiempoActual = time(NULL);
    
    // Almacenar el BCP recibido
    if (bcp->id >= 0 && bcp->id < 4) {
        ultimosBCP[bcp->id] = *bcp;
    }
    
    // Recalcular estadísticas basándose en todos los BCP conocidos
    tablaProc.numProcesosListos = 0;
    tablaProc.numProcesosBloqueados = 0;
    tablaProc.numProcesosTerminados = 0;
    tablaProc.tiempoTotalEjecucion = 0;
    tablaProc.tiempoTotalEspera = 0;
    tablaProc.tiempoTotalBloqueo = 0;
    tablaProc.tiempoTotalES = 0;
    
    for (int i = 0; i < 4; i++) {
        switch(ultimosBCP[i].estado) {
            case 1: tablaProc.numProcesosListos++; break;
            case 3: tablaProc.numProcesosBloqueados++; break;
            case 4: tablaProc.numProcesosTerminados++; break;
        }
        
        tablaProc.tiempoTotalEjecucion += ultimosBCP[i].tiempoEjecucion;
        tablaProc.tiempoTotalEspera += ultimosBCP[i].tiempoEspera;
        tablaProc.tiempoTotalBloqueo += ultimosBCP[i].tiempoBloqueo;
        tablaProc.tiempoTotalES += ultimosBCP[i].tiempoES;
    }
    
    // Actualizar otros campos
    tablaProc.procesoActual = bcp->id;
    tablaProc.algoritmoActual = bcp->algoritmoActual;
    tablaProc.cambiosContexto++;
    
    // Calcular uso de CPU
    time_t tiempoTotal = tablaProc.tiempoActual - tablaProc.tiempoInicio;
    if (tiempoTotal > 0) {
        tablaProc.usoCPU = (float)tablaProc.tiempoTotalEjecucion / (tiempoTotal * 1000) * 100;
    }
}

void escribirTablaProc() {
    FILE *archivo = fopen("TablaProcSlave.dat", "w");
    if (archivo == NULL) {
        perror("Error al abrir TablaProcSlave.dat");
        return;
    }
    
    char timestamp[25];
    obtenerFechaActual(timestamp, sizeof(timestamp));
    
    fprintf(archivo, "=== TABLA DE PROCESOS SLAVE - RONDA %d ===\n", contadorRondas);
    fprintf(archivo, "Timestamp: %s\n", timestamp);
    fprintf(archivo, "NumProcesos: %d\n", tablaProc.numProcesos);
    fprintf(archivo, "ProcesoActual: %d\n", tablaProc.procesoActual);
    fprintf(archivo, "AlgoritmoActual: %d\n", tablaProc.algoritmoActual);
    fprintf(archivo, "NumProcesosBloqueados: %d\n", tablaProc.numProcesosBloqueados);
    fprintf(archivo, "NumProcesosListos: %d\n", tablaProc.numProcesosListos);
    fprintf(archivo, "NumProcesosSuspendidos: %d\n", tablaProc.numProcesosSuspendidos);
    fprintf(archivo, "NumProcesosTerminados: %d\n", tablaProc.numProcesosTerminados);
    fprintf(archivo, "CambiosContexto: %d\n", tablaProc.cambiosContexto);
    fprintf(archivo, "TiempoTotalEjecucion: %d\n", tablaProc.tiempoTotalEjecucion);
    fprintf(archivo, "TiempoTotalEspera: %d\n", tablaProc.tiempoTotalEspera);
    fprintf(archivo, "TiempoTotalBloqueo: %d\n", tablaProc.tiempoTotalBloqueo);
    fprintf(archivo, "TiempoTotalES: %d\n", tablaProc.tiempoTotalES);
    fprintf(archivo, "TurnosAsignados: %d\n", tablaProc.turnosAsignados);
    fprintf(archivo, "TurnosCompletados: %d\n", tablaProc.turnosCompletados);
    fprintf(archivo, "TurnosInterrumpidos: %d\n", tablaProc.turnosInterrumpidos);
    fprintf(archivo, "TiempoInicio: %ld\n", tablaProc.tiempoInicio);
    fprintf(archivo, "TiempoActual: %ld\n", tablaProc.tiempoActual);
    fprintf(archivo, "UsoCPU: %.2f\n", tablaProc.usoCPU);
    
    // Agregar detalles de cada proceso
    fprintf(archivo, "\n=== DETALLES DE PROCESOS ===\n");
    for (int i = 0; i < 4; i++) {
        fprintf(archivo, "Proceso %d:\n", i);
        fprintf(archivo, "  Estado: %d\n", ultimosBCP[i].estado);
        fprintf(archivo, "  NumCartas: %d\n", ultimosBCP[i].numCartas);
        fprintf(archivo, "  TiempoEjecucion: %d\n", ultimosBCP[i].tiempoEjecucion);
        fprintf(archivo, "  VecesApeo: %d\n", ultimosBCP[i].vecesApeo);
        fprintf(archivo, "  CartasComidas: %d\n", ultimosBCP[i].cartasComidas);
        fprintf(archivo, "  TurnosPerdidos: %d\n", ultimosBCP[i].turnosPerdidos);
    }
    
    fclose(archivo);
}

int analizarCondicionesCambio() {
    // Condición 1: Más del 60% de procesos bloqueados
    if (tablaProc.numProcesosBloqueados > (tablaProc.numProcesos * 0.6)) {
        escribirBitacoraSlave("CONDICION_CAMBIO", "Mas del 60% de procesos bloqueados");
        return 1;
    }
    
    // Condición 2: Promedio alto de turnos perdidos
    int totalTurnosPerdidos = 0;
    for (int i = 0; i < 4; i++) {
        totalTurnosPerdidos += ultimosBCP[i].turnosPerdidos;
    }
    if (totalTurnosPerdidos > 8) {
        escribirBitacoraSlave("CONDICION_CAMBIO", "Demasiados turnos perdidos en total");
        return 1;
    }
    
    // Condición 3: Uso de CPU muy bajo o muy alto
    if ((tablaProc.usoCPU < 25.0 || tablaProc.usoCPU > 85.0) && contadorRondas > 3) {
        char detalle[100];
        sprintf(detalle, "Uso de CPU anormal: %.2f%%", tablaProc.usoCPU);
        escribirBitacoraSlave("CONDICION_CAMBIO", detalle);
        return 1;
    }
    
    return 0;
}

void inicializarTablaProc() {
    tablaProc.numProcesos = 4;
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
    
    // Inicializar array de BCP
    for (int i = 0; i < 4; i++) {
        memset(&ultimosBCP[i], 0, sizeof(BCP));
        ultimosBCP[i].id = i;
        ultimosBCP[i].estado = 1; // LISTO por defecto
    }
}

int main() {
    int ptid;
    char buf[100000];
    char nombreArchivo[100];
    int idJugador, ronda;
    
    printf("=== SLAVE RUMMY - SISTEMA DISTRIBUIDO ===\n");
    
    // Inicializar tabla de procesos
    inicializarTablaProc();
    
    // Obtener TID del padre (Master)
    ptid = pvm_parent();
    if (ptid < 0) {
        fprintf(stderr, "Error: No se pudo obtener el TID del Master\n");
        exit(1);
    }
    
    printf("Slave iniciado. Master TID: %d\n", ptid);
    printf("Slave TID: %d\n", pvm_mytid());
    
    // Escribir inicio en bitácora
    escribirBitacoraSlave("INICIO_SLAVE", "Slave iniciado correctamente");
    
    // Bucle principal del Slave
    while (1) {
        int cc = pvm_recv(ptid, -1);
        if (cc <= 0) {
            fprintf(stderr, "Error al recibir mensaje del Master\n");
            break;
        }
        
        // Desempaquetar datos
        pvm_upkstr(buf);           // Contenido del BCP
        pvm_upkstr(nombreArchivo); // Nombre del archivo
        pvm_upkint(&idJugador, 1, 1);  // ID del jugador
        pvm_upkint(&ronda, 1, 1);      // Número de ronda
        
        contadorRondas = ronda;
        
        // Verificar si es señal de terminación
        if (strcmp(buf, "FIN_JUEGO") == 0) {
            printf("Recibida señal de fin de juego\n");
            escribirBitacoraSlave("FIN_JUEGO", "Slave terminando por orden del Master");
            break;
        }
        
        printf("Ronda %d: Recibido BCP del Jugador %d\n", ronda, idJugador);
        
        // Parsear el BCP recibido
        BCP bcpRecibido = parsearBCP(buf);
        
        // Actualizar tabla de procesos
        actualizarTablaProc(&bcpRecibido, ronda);
        
        // Escribir tabla de procesos actualizada
        escribirTablaProc();
        
        // Analizar condiciones para cambio de algoritmo
        int debeCambiar = analizarCondicionesCambio();
        
        // Preparar respuesta
        char respuesta[200];
        if (debeCambiar) {
            sprintf(respuesta, "BCP_Jugador_%d_RECIBIDO_OK_CAMBIAR_ALGORITMO", idJugador);
            
            char detalleCambio[100];
            sprintf(detalleCambio, "Sugerido cambio de algoritmo en ronda %d", ronda);
            escribirBitacoraSlave("CAMBIO_ALGORITMO_SUGERIDO", detalleCambio);
        } else {
            sprintf(respuesta, "BCP_Jugador_%d_RECIBIDO_OK", idJugador);
        }
        
        // Enviar confirmación al Master
        pvm_initsend(PvmDataDefault);
        pvm_pkstr(respuesta);
        pvm_send(ptid, 1);
        
        // Registrar en bitácora
        char evento[100];
        sprintf(evento, "BCP_PROCESADO_JUGADOR_%d", idJugador);
        char detalle[200];
        sprintf(detalle, "Ronda %d - Estado: %d, Cartas: %d, Algoritmo: %s", 
                ronda, bcpRecibido.estado, bcpRecibido.numCartas,
                bcpRecibido.algoritmoActual == 0 ? "FCFS" : "RR");
        escribirBitacoraSlave(evento, detalle);
        
        // Si se sugirió cambio, esperar confirmación del Master
        if (debeCambiar) {
            cc = pvm_recv(ptid, 2); // Mensaje tipo 2 para confirmación de cambio
            if (cc > 0) {
                char confirmacion[100];
                pvm_upkstr(confirmacion);
                
                printf("Confirmación de cambio recibida: %s\n", confirmacion);
                
                // Actualizar algoritmo local
                if (strstr(confirmacion, "FCFS") != NULL) {
                    tablaProc.algoritmoActual = 0;
                } else if (strstr(confirmacion, "RR") != NULL) {
                    tablaProc.algoritmoActual = 1;
                }
                
                escribirBitacoraSlave("CAMBIO_ALGORITMO_CONFIRMADO", confirmacion);
            }
        }
        
        // Mostrar estadísticas actuales
        printf("Estadísticas Slave - Ronda %d:\n", ronda);
        printf("  Algoritmo actual: %s\n", tablaProc.algoritmoActual == 0 ? "FCFS" : "RR");
        printf("  Procesos listos: %d\n", tablaProc.numProcesosListos);
        printf("  Procesos bloqueados: %d\n", tablaProc.numProcesosBloqueados);
        printf("  Procesos terminados: %d\n", tablaProc.numProcesosTerminados);
        printf("  Uso CPU: %.2f%%\n", tablaProc.usoCPU);
        printf("  Cambios de contexto: %d\n", tablaProc.cambiosContexto);
        
        // Pequeña pausa para no saturar
        usleep(100000); // 100ms
    }
    
    // Escribir estadísticas finales
    escribirTablaProc();
    escribirBitacoraSlave("SLAVE_TERMINADO", "Estadísticas finales guardadas");
    
    printf("Slave terminado correctamente\n");
    pvm_exit();
    return 0;
}