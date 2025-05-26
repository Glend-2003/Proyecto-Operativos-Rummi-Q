#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include "juego.h"
#include "jugadores.h"
#include "mesa.h"
#include "procesos.h"
#include "utilidades.h"
#include "memoria.h"
#define _DEFAULT_SOURCE

// ============== ENTRADA SIN BLOQUEO ==============
int leerTecla(void) {
    struct termios oldt, newt;
    int ch, oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    return ch;
}

// ============== MONITOR DE TECLAS ==============
void *monitorTeclas(void *arg) {
    while (!juegoTerminado()) {
        int tecla = leerTecla();

        switch (tecla) {
            case '1':
                cambiarAlgoritmo(ALG_FCFS);
                break;
            case '2':
                cambiarAlgoritmo(ALG_RR);
                break;
            case '3':
                cambiarAlgoritmoMemoria(ALG_AJUSTE_OPTIMO);
                break;
            case '4':
                cambiarAlgoritmoMemoria(ALG_LRU);
                break;
            case '5':
                cambiarAlgoritmoMemoria(ALG_MAPA_BITS);
                break;
            case 'm':
            case 'M':
                imprimirEstadoMemoria();
                imprimirEstadoMemoriaVirtual();
                break;
            case 'q':
            case 'Q':
                printf("\nSaliendo del juego por solicitud del usuario...\n");
                finalizarJuego(-1);
                return NULL;
        }

        usleep(100000);
    }
    
    printf("Hilo de monitoreo de teclas finalizado\n");
    return NULL;
}

// ============== INFORMACIÓN DEL JUEGO ==============
void mostrarInformacion(void) {
    colorCian();
    printf("\n================================\n");
    printf("    JUEGO RUMMY - SISTEMAS OPERATIVOS\n");
    printf("================================\n\n");
    colorReset();
    
    printf("REGLAS DEL JUEGO:\n");
    printf("- 2 barajas completas (104 cartas + 4 comodines)\n");
    printf("- Objetivo: quedarse sin cartas\n");
    printf("- Apeadas: Grupos (3-4 cartas mismo valor) o Escaleras (3+ consecutivas)\n");
    printf("- Primera apeada requiere 30+ puntos\n");
    printf("- Sin jugada posible: comer carta de la banca\n\n");
    
    printf("ALGORITMOS DE PLANIFICACIÓN:\n");
    printf("- FCFS: Primer jugador listo, primero en ser atendido\n");
    printf("- Round Robin: Quantum de tiempo por jugador\n\n");
    
    printf("ALGORITMOS DE MEMORIA:\n");
    printf("- Ajuste Óptimo: Partición más pequeña disponible\n");
    printf("- LRU: Reemplaza página menos usada recientemente\n");
    printf("- Mapa de Bits: Gestión por bloques de bits\n\n");
    
    colorVerde();
    printf("CONTROLES:\n");
    printf("1 - FCFS CPU          | 2 - Round Robin CPU\n");
    printf("3 - Ajuste Óptimo     | 4 - LRU Virtual\n");
    printf("5 - Mapa de Bits      | m - Estado memoria\n");
    printf("q - Salir del juego\n\n");
    colorReset();
}

// ============== FUNCIÓN PRINCIPAL ==============
int main(int argc, char *argv[]) {
    int numJugadores = 4;
    int opcion;
    
    // Procesar argumentos
    if (argc > 1) {
        numJugadores = atoi(argv[1]);
        if (numJugadores <= 0 || numJugadores > MAX_JUGADORES) {
            printf("Número de jugadores inválido. Debe ser entre 1 y %d\n", MAX_JUGADORES);
            return EXIT_FAILURE;
        }
    }
    
    srand(time(NULL));
    mostrarInformacion();
    
    // Confirmar inicio
    printf("¿Iniciar juego con %d jugadores? (1: Sí, 0: No): ", numJugadores);
    scanf("%d", &opcion);
    if (opcion != 1) {
        printf("Juego cancelado por el usuario.\n");
        return EXIT_SUCCESS;
    }
    
    while (getchar() != '\n');
    
    // Preparar entorno
    system("mkdir -p bcp");
    inicializarMemoria();
    
    // Inicializar juego
    colorVerde();
    printf("\nInicializando juego con %d jugadores...\n", numJugadores);
    colorReset();
    
    if (!inicializarJuego(numJugadores)) {
        colorRojo();
        printf("Error al inicializar el juego\n");
        colorReset();
        return EXIT_FAILURE;
    }
    
    // Crear monitor de teclas
    pthread_t hiloMonitor;
    if (pthread_create(&hiloMonitor, NULL, monitorTeclas, NULL) != 0) {
        colorRojo();
        printf("Error al crear el hilo monitor\n");
        colorReset();
        return EXIT_FAILURE;
    }
    
    // Iniciar juego
    printf("\nPresione cualquier tecla para iniciar el juego...\n");
    getchar();
    
    colorVerde();
    printf("\n¡Que comience el juego!\n");
    colorReset();
    
    iniciarJuego();
    
    // Finalizar monitor
    printf("Esperando a que el hilo monitor finalice...\n");
    
    void *status;
    int joinResult = pthread_join(hiloMonitor, &status);
    
    if (joinResult != 0) {
        printf("No se pudo esperar correctamente al hilo monitor, código: %d\n", joinResult);
        printf("Continuando con la finalización del programa...\n");
    } else {
        printf("Hilo monitor finalizado correctamente\n");
    }
    
    // Estadísticas finales
    printf("\n=== ESTADÍSTICAS FINALES DE MEMORIA ===\n");
    imprimirEstadoMemoria();
    imprimirEstadoMemoriaVirtual();
    
    liberarJuego();
    
    colorCian();
    printf("\n¡Gracias por jugar! El programa ha finalizado correctamente.\n");
    colorReset();
    
    return EXIT_SUCCESS;
}