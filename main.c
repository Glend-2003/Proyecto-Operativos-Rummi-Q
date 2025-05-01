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
#include <unistd.h>

/* Función para leer una tecla sin bloqueo */
int leerTecla(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;
    
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

/* Hilo para monitorear cambios de algoritmo */
void *monitorTeclas(void *arg) {
    while (!juegoTerminado()) {
        int tecla = leerTecla();
        
        if (tecla == '1') {
            cambiarAlgoritmo(ALG_FCFS);
        } else if (tecla == '2') {
            cambiarAlgoritmo(ALG_RR);
        } else if (tecla == 'q' || tecla == 'Q') {
            /* Opción para salir del juego con 'q' */
            printf("\nSaliendo del juego por solicitud del usuario...\n");
            
            // Forzar la finalización del juego sin ganador
            finalizarJuego(-1); /* -1 indica que no hay ganador */
            
            // Es importante salir del bucle después de finalizar
            break;
        }
        
        usleep(100000);  /* 100ms */
    }
    
    printf("Hilo de monitoreo de teclas finalizado\n");
    return NULL;
}

/* Mostrar información del juego y reglas */
void mostrarInformacion(void) {
    colorCian();
    printf("\n================================\n");
    printf("       JUEGO RUMMY - SISTEMAS OPERATIVOS      \n");
    printf("================================\n\n");
    colorReset();
    
    printf("Este programa simula el juego de Rummy para demostrar\n");
    printf("conceptos de planificación de procesos en sistemas operativos.\n\n");
    
    printf("REGLAS DEL JUEGO:\n");
    printf("- Se juega con 2 barajas completas (104 cartas + 4 comodines)\n");
    printf("- El objetivo es quedarse sin cartas\n");
    printf("- Se pueden formar 'apeadas' de dos tipos:\n");
    printf("  * Grupos: 3 o 4 cartas del mismo valor y diferentes palos\n");
    printf("  * Escaleras: 3+ cartas consecutivas del mismo palo\n");
    printf("- Para la primera apeada se necesitan 30+ puntos\n");
    printf("- Si no puede hacer jugada, se come una carta de la banca\n\n");
    
    printf("ALGORITMOS DE PLANIFICACIÓN:\n");
    printf("- FCFS (First-Come, First-Served): Primer jugador listo, primero en ser atendido\n");
    printf("- Round Robin: Asigna un quantum de tiempo a cada jugador en turnos\n\n");
    
    colorVerde();
    printf("CONTROLES:\n");
    printf("- Presione '1' para cambiar a algoritmo FCFS\n");
    printf("- Presione '2' para cambiar a algoritmo Round Robin\n");
    printf("- Presione 'q' para salir del juego\n\n");
    colorReset();
}

/* Función principal */
int main(int argc, char *argv[]) {
    int numJugadores = 4; /* Por defecto 4 jugadores según el enunciado */
    int opcion;
    
    /* Procesar argumentos de línea de comandos */
    if (argc > 1) {
        numJugadores = atoi(argv[1]);
        if (numJugadores <= 0 || numJugadores > MAX_JUGADORES) {
            printf("Número de jugadores inválido. Debe ser entre 1 y %d\n", MAX_JUGADORES);
            return EXIT_FAILURE;
        }
    }
    
    /* Inicializar semilla para números aleatorios */
    srand(time(NULL));
    
    /* Mostrar información del juego */
    mostrarInformacion();
    
    /* Confirmar inicio del juego */
    printf("¿Desea iniciar el juego con %d jugadores? (1: Sí, 0: No): ", numJugadores);
    scanf("%d", &opcion);
    if (opcion != 1) {
        printf("Juego cancelado por el usuario.\n");
        return EXIT_SUCCESS;
    }
    
    /* Limpiar el buffer de entrada */
    while (getchar() != '\n');
    
    /* Crear directorio para los BCP si no existe */
    system("mkdir -p bcp");
    
    /* Inicializar el juego */
    colorVerde();
    printf("\nInicializando juego con %d jugadores...\n", numJugadores);
    colorReset();
    
    if (!inicializarJuego(numJugadores)) {
        colorRojo();
        printf("Error al inicializar el juego\n");
        colorReset();
        return EXIT_FAILURE;
    }
    
    /* Crear hilo para monitorear teclas */
    pthread_t hiloMonitor;
    if (pthread_create(&hiloMonitor, NULL, monitorTeclas, NULL) != 0) {
        colorRojo();
        printf("Error al crear el hilo monitor\n");
        colorReset();
        return EXIT_FAILURE;
    }
    
    /* Iniciar el juego */
    printf("\nPresione cualquier tecla para iniciar el juego...\n");
    getchar();
    
    colorVerde();
    printf("\n¡Que comience el juego!\n");
    colorReset();
    
    /* Esta función ejecutará el bucle principal */
    iniciarJuego();
    
    /* Esperar a que termine el hilo monitor con un timeout manual */
    printf("Esperando a que el hilo monitor finalice...\n");
    
    /* Intento de join con tiempo límite */
    time_t startTime = time(NULL);
    const int MAX_WAIT_TIME = 3; // 3 segundos máximo
    
    /* Intentar hacer join normal con el hilo monitor */
    void *status;
    int joinResult = pthread_join(hiloMonitor, &status);
    
    if (joinResult != 0) {
        printf("No se pudo esperar correctamente al hilo monitor, código: %d\n", joinResult);
        printf("Continuando con la finalización del programa...\n");
    } else {
        printf("Hilo monitor finalizado correctamente\n");
    }
    
    /* Liberar recursos */
    liberarJuego();
    
    colorCian();
    printf("\n¡Gracias por jugar! El programa ha finalizado correctamente.\n");
    colorReset();
    
    return EXIT_SUCCESS;
}