#include <stdio.h>
#include <stdlib.h>
#include <pvm3.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "procesos.h"
#include "juego.h"

#define MSGTAG_BCP 1
#define MSGTAG_CONFIRM 2
#define MSGTAG_CAMBIO_ALG 3
#define MSGTAG_CONFIRM_ALG 4

// Variables globales
int tid_slaves[2];  // TIDs de los slaves
int algoritmoActualMaster = ALG_FCFS;
FILE *bitacoraMaster;

// Función para enviar BCP a un slave
void enviarBCP(int tid_slave, BCP *bcp) {
    // Empaquetar el BCP
    pvm_initsend(PvmDataDefault);
    
    // Empaquetar todos los campos del BCP
    pvm_pkint(&bcp->id, 1, 1);
    pvm_pkint(&bcp->estado, 1, 1);
    pvm_pkint(&bcp->prioridad, 1, 1);
    pvm_pkint(&bcp->tiempoEjecucion, 1, 1);
    pvm_pkint(&bcp->tiempoEspera, 1, 1);
    pvm_pkint(&bcp->tiempoBloqueo, 1, 1);
    pvm_pkint(&bcp->tiempoES, 1, 1);
    pvm_pkint(&bcp->tiempoQuantum, 1, 1);
    pvm_pkint(&bcp->tiempoRestante, 1, 1);
    pvm_pkint(&bcp->numCartas, 1, 1);
    pvm_pkint(&bcp->puntosAcumulados, 1, 1);
    pvm_pkint(&bcp->turnoActual, 1, 1);
    pvm_pkint(&bcp->vecesApeo, 1, 1);
    pvm_pkint(&bcp->cartasComidas, 1, 1);
    pvm_pkint(&bcp->intentosFallidos, 1, 1);
    pvm_pkint(&bcp->turnosPerdidos, 1, 1);
    pvm_pkint(&bcp->cambiosEstado, 1, 1);
    pvm_pkint(&bcp->tiempoUltimoEstado, 1, 1);
    pvm_pkint(&bcp->tiempoUltimoBloqueo, 1, 1);
    
    // Enviar al slave
    pvm_send(tid_slave, MSGTAG_BCP);
    
    // Log en bitácora
    fprintf(bitacoraMaster, "[%s] BCP enviado al Slave (tid=%d) - Proceso %d\n", 
            obtenerTimestamp(), tid_slave, bcp->id);
    fflush(bitacoraMaster);
}

// Función para recibir confirmación del slave
void recibirConfirmacion(int tid_slave) {
    int bufid;
    char mensaje[256];
    
    // Recibir mensaje de confirmación
    bufid = pvm_recv(tid_slave, MSGTAG_CONFIRM);
    if (bufid < 0) {
        printf("Error al recibir confirmación del slave\n");
        return;
    }
    
    // Desempaquetar mensaje
    pvm_upkstr(mensaje);
    
    // Escribir en bitácora
    fprintf(bitacoraMaster, "[%s] Confirmación del Slave (tid=%d): %s\n", 
            obtenerTimestamp(), tid_slave, mensaje);
    fflush(bitacoraMaster);
}

// Función para manejar cambio de algoritmo
void manejarCambioAlgoritmo() {
    int bufid;
    int nuevoAlgoritmo;
    char razon[256];
    int tid_solicitante;
    
    // Verificar si hay solicitud de cambio
    bufid = pvm_nrecv(-1, MSGTAG_CAMBIO_ALG);
    if (bufid > 0) {
        // Obtener quién envió el mensaje
        pvm_bufinfo(bufid, NULL, NULL, &tid_solicitante);
        
        // Desempaquetar datos
        pvm_upkint(&nuevoAlgoritmo, 1, 1);
        pvm_upkstr(razon);
        
        // Cambiar algoritmo
        algoritmoActualMaster = nuevoAlgoritmo;
        cambiarAlgoritmo(nuevoAlgoritmo);
        
        // Log en bitácora
        fprintf(bitacoraMaster, "[%s] Cambio de algoritmo solicitado por Slave (tid=%d): %s -> %s. Razón: %s\n",
                obtenerTimestamp(), tid_solicitante,
                algoritmoActualMaster == ALG_FCFS ? "Round Robin" : "FCFS",
                nuevoAlgoritmo == ALG_FCFS ? "FCFS" : "Round Robin",
                razon);
        fflush(bitacoraMaster);
        
        // Confirmar al slave
        pvm_initsend(PvmDataDefault);
        pvm_pkint(&nuevoAlgoritmo, 1, 1);
        pvm_send(tid_solicitante, MSGTAG_CONFIRM_ALG);
    }
}

// Función para obtener timestamp
char* obtenerTimestamp() {
    static char timestamp[25];
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    return timestamp;
}

// Thread para monitorear el juego y enviar BCPs
void* monitorJuego(void* arg) {
    int slaveActual = 0;
    
    while (!juegoTerminado()) {
        // Obtener BCPs de todos los jugadores
        for (int i = 0; i < 4; i++) {  // 4 jugadores
            BCP *bcp = obtenerBCPJugador(i);
            if (bcp != NULL) {
                // Enviar BCP al slave actual (alternando entre slaves)
                enviarBCP(tid_slaves[slaveActual], bcp);
                
                // Esperar confirmación
                recibirConfirmacion(tid_slaves[slaveActual]);
                
                // Alternar entre slaves
                slaveActual = (slaveActual + 1) % 2;
            }
        }
        
        // Manejar posibles cambios de algoritmo
        manejarCambioAlgoritmo();
        
        // Pequeña pausa
        usleep(500000);  // 500ms
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    int mytid;
    int numJugadores = 4;
    pthread_t hiloMonitor;
    
    // Inicializar PVM
    mytid = pvm_mytid();
    if (mytid < 0) {
        printf("Error al inicializar PVM\n");
        return 1;
    }
    
    printf("Master iniciado con TID: %d\n", mytid);
    
    // Abrir bitácora
    bitacoraMaster = fopen("bitacoraMaster.dat", "a");
    if (bitacoraMaster == NULL) {
        printf("Error al abrir bitácora\n");
        return 1;
    }
    
    fprintf(bitacoraMaster, "\n[%s] === MASTER INICIADO (TID=%d) ===\n", 
            obtenerTimestamp(), mytid);
    fflush(bitacoraMaster);
    
    // Spawn de los slaves
    char *slave_args[2] = {NULL};
    int numt = pvm_spawn("slave", slave_args, PvmTaskDefault, NULL, 2, tid_slaves);
    
    if (numt < 2) {
        printf("Error: Solo se pudieron crear %d slaves de 2\n", numt);
        fprintf(bitacoraMaster, "[%s] Error: Solo se crearon %d slaves\n", 
                obtenerTimestamp(), numt);
        pvm_exit();
        return 1;
    }
    
    printf("Slaves creados: TID1=%d, TID2=%d\n", tid_slaves[0], tid_slaves[1]);
    fprintf(bitacoraMaster, "[%s] Slaves creados: TID1=%d, TID2=%d\n", 
            obtenerTimestamp(), tid_slaves[0], tid_slaves[1]);
    fflush(bitacoraMaster);
    
    // Inicializar el juego
    if (!inicializarJuego(numJugadores)) {
        printf("Error al inicializar el juego\n");
        pvm_exit();
        return 1;
    }
    
    // Crear hilo para monitorear el juego
    if (pthread_create(&hiloMonitor, NULL, monitorJuego, NULL) != 0) {
        printf("Error al crear hilo monitor\n");
        pvm_exit();
        return 1;
    }
    
    // Iniciar el juego
    iniciarJuego();
    
    // Esperar a que termine el hilo monitor
    pthread_join(hiloMonitor, NULL);
    
    // Notificar a los slaves que terminen
    pvm_initsend(PvmDataDefault);
    pvm_pkint(&mytid, 1, 1);
    pvm_mcast(tid_slaves, 2, 0);  // Mensaje especial para terminar
    
    // Cerrar bitácora
    fprintf(bitacoraMaster, "[%s] === MASTER FINALIZADO ===\n", obtenerTimestamp());
    fclose(bitacoraMaster);
    
    // Liberar recursos
    liberarJuego();
    
    // Salir de PVM
    pvm_exit();
    
    return 0;
}

// Función auxiliar para obtener BCP de un jugador (agregar a procesos.c)
BCP* obtenerBCPJugador(int idJugador) {
    extern TablaProc tablaProc;
    
    for (int i = 0; i < tablaProc.numProcesos; i++) {
        if (tablaProc.procesos[i]->id == idJugador) {
            return tablaProc.procesos[i];
        }
    }
    return NULL;
}
