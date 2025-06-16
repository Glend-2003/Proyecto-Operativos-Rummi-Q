#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include "pvm3.h"
#include <time.h>
#include <unistd.h>

// Estructuras del juego Rummy
typedef struct {
    int id;
    int estado;              // 0=NUEVO, 1=LISTO, 2=EJECUTANDO, 3=BLOQUEADO, 4=TERMINADO
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
    int algoritmoActual;     // 0=FCFS, 1=RR
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

// Variables globales
BCP jugadores[4];
TablaProc tablaProc;
int algoritmoActual = 0; // 0=FCFS, 1=RR
int rondaActual = 1;

void obtenerFechaActual(char *buffer, size_t size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void inicializarBCP(BCP *bcp, int id) {
    bcp->id = id;
    bcp->estado = 1; // LISTO
    bcp->prioridad = id;
    bcp->tiempoEjecucion = 0;
    bcp->tiempoEspera = 0;
    bcp->tiempoBloqueo = 0;
    bcp->tiempoES = 0;
    bcp->tiempoQuantum = algoritmoActual == 1 ? 3000 : 10000; // 3s para RR, 10s para FCFS
    bcp->tiempoRestante = bcp->tiempoQuantum;
    bcp->numCartas = 15 + (rand() % 10); // 15-24 cartas iniciales
    bcp->puntosAcumulados = 0;
    bcp->turnoActual = (id == 0) ? 1 : 0;
    bcp->vecesApeo = 0;
    bcp->cartasComidas = 0;
    bcp->intentosFallidos = 0;
    bcp->turnosPerdidos = 0;
    bcp->cambiosEstado = 0;
    bcp->tiempoUltimoEstado = 0;
    bcp->tiempoUltimoBloqueo = 0;
    bcp->algoritmoActual = algoritmoActual;
}

void inicializarTablaProc() {
    tablaProc.numProcesos = 4;
    tablaProc.procesoActual = 0;
    tablaProc.algoritmoActual = algoritmoActual;
    tablaProc.numProcesosBloqueados = 0;
    tablaProc.numProcesosListos = 4;
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
}

void escribirBCP(const char *nombreArchivo, BCP *bcp) {
    FILE *archivo = fopen(nombreArchivo, "w");
    if (archivo == NULL) {
        perror("Error al abrir el archivo BCP");
        return;
    }
    
    char timestamp[25];
    obtenerFechaActual(timestamp, sizeof(timestamp));
    
    fprintf(archivo, "=== BCP JUGADOR %d - RONDA %d ===\n", bcp->id, rondaActual);
    fprintf(archivo, "Timestamp: %s\n", timestamp);
    fprintf(archivo, "ID: %d\n", bcp->id);
    fprintf(archivo, "Estado: %d\n", bcp->estado);
    fprintf(archivo, "Prioridad: %d\n", bcp->prioridad);
    fprintf(archivo, "TiempoEjecucion: %d\n", bcp->tiempoEjecucion);
    fprintf(archivo, "TiempoEspera: %d\n", bcp->tiempoEspera);
    fprintf(archivo, "TiempoBloqueo: %d\n", bcp->tiempoBloqueo);
    fprintf(archivo, "TiempoES: %d\n", bcp->tiempoES);
    fprintf(archivo, "TiempoQuantum: %d\n", bcp->tiempoQuantum);
    fprintf(archivo, "TiempoRestante: %d\n", bcp->tiempoRestante);
    fprintf(archivo, "NumCartas: %d\n", bcp->numCartas);
    fprintf(archivo, "PuntosAcumulados: %d\n", bcp->puntosAcumulados);
    fprintf(archivo, "TurnoActual: %d\n", bcp->turnoActual);
    fprintf(archivo, "VecesApeo: %d\n", bcp->vecesApeo);
    fprintf(archivo, "CartasComidas: %d\n", bcp->cartasComidas);
    fprintf(archivo, "IntentosFallidos: %d\n", bcp->intentosFallidos);
    fprintf(archivo, "TurnosPerdidos: %d\n", bcp->turnosPerdidos);
    fprintf(archivo, "CambiosEstado: %d\n", bcp->cambiosEstado);
    fprintf(archivo, "TiempoUltimoEstado: %d\n", bcp->tiempoUltimoEstado);
    fprintf(archivo, "TiempoUltimoBloqueo: %d\n", bcp->tiempoUltimoBloqueo);
    fprintf(archivo, "AlgoritmoActual: %d\n", bcp->algoritmoActual);
    
    fclose(archivo);
}

void escribirTablaProc(const char *nombreArchivo) {
    FILE *archivo = fopen(nombreArchivo, "w");
    if (archivo == NULL) {
        perror("Error al abrir el archivo Tabla de Procesos");
        return;
    }
    
    char timestamp[25];
    obtenerFechaActual(timestamp, sizeof(timestamp));
    
    fprintf(archivo, "=== TABLA DE PROCESOS - RONDA %d ===\n", rondaActual);
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
    
    fclose(archivo);
}

void escribirBitacoraMaster(const char *mensaje, const char *respuestaSlave) {
    FILE *archivo = fopen("bitacoraMaster.dat", "a");
    if (archivo == NULL) {
        perror("Error al abrir bitacoraMaster.dat");
        return;
    }
    
    char timestamp[25];
    obtenerFechaActual(timestamp, sizeof(timestamp));
    
    fprintf(archivo, "[%s] Ronda %d: %s - Respuesta Slave: %s\n", 
            timestamp, rondaActual, mensaje, respuestaSlave);
    fclose(archivo);
}

void simularJugada(BCP *bcp) {
    // Simular diferentes acciones del juego
    int accion = rand() % 4;
    
    switch(accion) {
        case 0: // Jugada exitosa
            bcp->tiempoEjecucion += rand() % 1000 + 500; // 500-1500ms
            if (bcp->numCartas > 3) {
                bcp->numCartas -= rand() % 3 + 1;
                bcp->vecesApeo++;
                bcp->puntosAcumulados += rand() % 30 + 10;
            }
            bcp->estado = 1; // LISTO
            break;
            
        case 1: // Comer carta (E/S)
            bcp->estado = 3; // BLOQUEADO
            bcp->tiempoES = rand() % 2000 + 1000; // 1-3 segundos
            bcp->cartasComidas++;
            bcp->numCartas++;
            break;
            
        case 2: // Intento fallido
            bcp->tiempoEjecucion += rand() % 500 + 200;
            bcp->intentosFallidos++;
            bcp->estado = 1; // LISTO
            break;
            
        case 3: // Timeout
            bcp->turnosPerdidos++;
            bcp->estado = 1; // LISTO
            break;
    }
    
    // Actualizar otros campos
    bcp->cambiosEstado++;
    bcp->tiempoUltimoEstado = rand() % 1000;
    
    // Verificar condición de victoria
    if (bcp->numCartas <= 0) {
        bcp->estado = 4; // TERMINADO
        printf("¡JUGADOR %d HA GANADO!\n", bcp->id);
    }
}

void actualizarTablaProc() {
    tablaProc.tiempoActual = time(NULL);
    tablaProc.numProcesosListos = 0;
    tablaProc.numProcesosBloqueados = 0;
    tablaProc.numProcesosTerminados = 0;
    
    for (int i = 0; i < 4; i++) {
        switch(jugadores[i].estado) {
            case 1: tablaProc.numProcesosListos++; break;
            case 3: tablaProc.numProcesosBloqueados++; break;
            case 4: tablaProc.numProcesosTerminados++; break;
        }
        
        tablaProc.tiempoTotalEjecucion += jugadores[i].tiempoEjecucion;
        tablaProc.tiempoTotalEspera += jugadores[i].tiempoEspera;
        tablaProc.tiempoTotalBloqueo += jugadores[i].tiempoBloqueo;
        tablaProc.tiempoTotalES += jugadores[i].tiempoES;
    }
    
    tablaProc.turnosAsignados++;
    
    // Calcular uso de CPU
    time_t tiempoTotal = tablaProc.tiempoActual - tablaProc.tiempoInicio;
    if (tiempoTotal > 0) {
        tablaProc.usoCPU = (float)tablaProc.tiempoTotalEjecucion / (tiempoTotal * 1000) * 100;
    }
}

int debecambiarAlgoritmo() {
    // Condición 1: Más del 50% de procesos bloqueados
    if (tablaProc.numProcesosBloqueados > tablaProc.numProcesos / 2) {
        return 1;
    }
    
    // Condición 2: Demasiados turnos perdidos
    int totalTurnosPerdidos = 0;
    for (int i = 0; i < 4; i++) {
        totalTurnosPerdidos += jugadores[i].turnosPerdidos;
    }
    if (totalTurnosPerdidos > 5) {
        return 1;
    }
    
    // Condición 3: Uso de CPU muy bajo
    if (tablaProc.usoCPU < 30.0 && rondaActual > 3) {
        return 1;
    }
    
    return 0;
}

void cambiarAlgoritmo() {
    algoritmoActual = (algoritmoActual == 0) ? 1 : 0;
    tablaProc.algoritmoActual = algoritmoActual;
    
    // Actualizar quantum de todos los jugadores
    for (int i = 0; i < 4; i++) {
        jugadores[i].tiempoQuantum = algoritmoActual == 1 ? 3000 : 10000;
        jugadores[i].tiempoRestante = jugadores[i].tiempoQuantum;
        jugadores[i].algoritmoActual = algoritmoActual;
    }
    
    printf("Algoritmo cambiado a: %s\n", algoritmoActual == 0 ? "FCFS" : "Round Robin");
}

char* leer_archivo(const char* nombre_archivo) {
    FILE *fp;
    char *contenido;
    long tamano_archivo;
    
    fp = fopen(nombre_archivo, "rb");
    if (fp == NULL) {
        return NULL;
    }
    
    fseek(fp, 0, SEEK_END);
    tamano_archivo = ftell(fp);
    rewind(fp);
    
    contenido = (char*)malloc((tamano_archivo + 1) * sizeof(char));
    if (contenido == NULL) {
        perror("Error al asignar memoria");
        fclose(fp);
        return NULL;
    }

    fread(contenido, sizeof(char), tamano_archivo, fp);
    contenido[tamano_archivo] = '\0';

    fclose(fp);
    return contenido;
}

int main() {
    char buf[100000];
    int cc, tid[1];  // Solo 1 slave
    char fechaEnvio[20];
    char fechaRecibido[20];
    char nombreArchivoBCP[50];
    char nombreArchivoTabla[50] = "TablaProc.dat";
    
    printf("=== MASTER RUMMY - SISTEMA DISTRIBUIDO ===\n");
    
    // Cambiar al directorio de trabajo
    if (chdir("/home/usuario/pvm3/rummy") != 0) {
        printf("Advertencia: No se pudo cambiar al directorio esperado\n");
    }
    
    // Inicializar semilla aleatoria
    srand(time(NULL));
    
    // Inicializar estructuras del juego
    inicializarTablaProc();
    for (int i = 0; i < 4; i++) {
        inicializarBCP(&jugadores[i], i);
    }
    
    // Inicializar PVM y crear slaves
    int master_tid = pvm_mytid();
    if (master_tid < 0) {
        fprintf(stderr, "Error al inicializar PVM\n");
        exit(1);
    }
    
    cc = pvm_spawn("SLAVE_RUMMY", (char**)0, 0, "", 1, tid);
    if (cc != 1) {
        fprintf(stderr, "Error al crear slave: %d creados de 1 esperado\n", cc);
        exit(1);
    }
    
    printf("Master TID: %d\n", master_tid);
    printf("Slave creado: TID: %d\n", tid[0]);
    
    // Bucle principal del juego
    int juegoTerminado = 0;
    
    while (!juegoTerminado && rondaActual <= 20) {
        printf("\n=== RONDA %d ===\n", rondaActual);
        
        // Simular jugadas para cada jugador
        for (int i = 0; i < 4; i++) {
            if (jugadores[i].estado != 4) { // Si no ha terminado
                // Asignar turno
                for (int j = 0; j < 4; j++) {
                    jugadores[j].turnoActual = (j == i) ? 1 : 0;
                }
                
                simularJugada(&jugadores[i]);
                
                // Escribir BCP del jugador actual
                sprintf(nombreArchivoBCP, "BCP_Jugador_%d.dat", i);
                escribirBCP(nombreArchivoBCP, &jugadores[i]);
                
                // Leer archivo BCP
                char *contenidoBCP = leer_archivo(nombreArchivoBCP);
                if (contenidoBCP != NULL) {
                    obtenerFechaActual(fechaEnvio, sizeof(fechaEnvio));
                    
                    printf("Enviando BCP Jugador %d al Slave\n", i);
                    
                    // Enviar BCP al único slave
                    pvm_initsend(PvmDataDefault);
                    pvm_pkstr(contenidoBCP);
                    pvm_pkstr(nombreArchivoBCP);
                    pvm_pkint(&i, 1, 1); // ID del jugador
                    pvm_pkint(&rondaActual, 1, 1); // Número de ronda
                    pvm_send(tid[0], 1);
                    
                    free(contenidoBCP);
                    
                    // Recibir confirmación del slave
                    cc = pvm_recv(tid[0], -1);
                    if (cc > 0) {
                        pvm_upkstr(buf);
                        obtenerFechaActual(fechaRecibido, sizeof(fechaRecibido));
                        
                        printf("Respuesta Slave: %s\n", buf);
                        
                        // Escribir en bitácora
                        char mensaje[200];
                        sprintf(mensaje, "BCP Jugador %d enviado", i);
                        escribirBitacoraMaster(mensaje, buf);
                        
                        // Verificar si el slave sugiere cambio de algoritmo
                        if (strstr(buf, "CAMBIAR_ALGORITMO") != NULL) {
                            printf("Slave sugiere cambio de algoritmo\n");
                            cambiarAlgoritmo();
                            
                            // Confirmar cambio al slave
                            pvm_initsend(PvmDataDefault);
                            char confirmacion[100];
                            sprintf(confirmacion, "ALGORITMO_CAMBIADO_A_%s", 
                                   algoritmoActual == 0 ? "FCFS" : "RR");
                            pvm_pkstr(confirmacion);
                            pvm_send(tid[0], 2);
                        }
                    } else {
                        printf("Error al recibir del slave\n");
                    }
                }
                
                // Verificar si alguien ganó
                if (jugadores[i].estado == 4) {
                    juegoTerminado = 1;
                    break;
                }
            }
        }
        
        // Actualizar tabla de procesos
        actualizarTablaProc();
        escribirTablaProc(nombreArchivoTabla);
        
        // Verificar condiciones para cambio de algoritmo
        if (debecambiarAlgoritmo()) {
            printf("Condiciones detectadas para cambio de algoritmo\n");
            cambiarAlgoritmo();
        }
        
        // Mostrar estadísticas de la ronda
        printf("Estadísticas Ronda %d:\n", rondaActual);
        printf("  Algoritmo actual: %s\n", algoritmoActual == 0 ? "FCFS" : "Round Robin");
        printf("  Procesos listos: %d\n", tablaProc.numProcesosListos);
        printf("  Procesos bloqueados: %d\n", tablaProc.numProcesosBloqueados);
        printf("  Procesos terminados: %d\n", tablaProc.numProcesosTerminados);
        printf("  Uso CPU: %.2f%%\n", tablaProc.usoCPU);
        
        for (int i = 0; i < 4; i++) {
            printf("  Jugador %d: %d cartas, Estado %d\n", 
                   i, jugadores[i].numCartas, jugadores[i].estado);
        }
        
        rondaActual++;
        sleep(2); // Pausa entre rondas
    }
    
    // Enviar señal de terminación al slave
    pvm_initsend(PvmDataDefault);
    pvm_pkstr("FIN_JUEGO");
    pvm_pkstr("Juego terminado");
    pvm_send(tid[0], 999);
    
    printf("\n=== JUEGO TERMINADO ===\n");
    printf("Total de rondas: %d\n", rondaActual - 1);
    printf("Ganador: ");
    for (int i = 0; i < 4; i++) {
        if (jugadores[i].estado == 4) {
            printf("Jugador %d\n", i);
            break;
        }
    }
    
    pvm_exit();
    return 0;
}