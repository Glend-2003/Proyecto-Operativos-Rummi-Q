#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include "juego.h"
#include "jugadores.h"
#include "mesa.h"
#include "procesos.h"
#include "utilidades.h"

// Función para leer una tecla sin bloqueo
int leerTecla() {
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

// Hilo para monitorear cambios de algoritmo
void *monitorTeclas(void *arg) {
    while (!juegoTerminado()) {
        int tecla = leerTecla();
        
        if (tecla == '1') {
            cambiarAlgoritmo(ALG_FCFS);
        } else if (tecla == '2') {
            cambiarAlgoritmo(ALG_RR);
        }
        
        usleep(100000);  // 100ms
    }
    
    return NULL;
}

int main() {
    // Inicializar semilla para números aleatorios
    srand(time(NULL));
    
    // Número de jugadores (4 por defecto según el enunciado)
    int numJugadores = 4;
    
    // Imprimir instrucciones
    printf("=== JUEGO RUMMY ===\n");
    printf("Presione '1' para cambiar a algoritmo FCFS\n");
    printf("Presione '2' para cambiar a algoritmo Round Robin\n");
    printf("Iniciando juego con %d jugadores...\n\n", numJugadores);
    
    // Inicializar el juego
    if (!inicializarJuego(numJugadores)) {
        printf("Error al inicializar el juego\n");
        return EXIT_FAILURE;
    }
    
    // Crear hilo para monitorear teclas
    pthread_t hiloMonitor;
    if (pthread_create(&hiloMonitor, NULL, monitorTeclas, NULL) != 0) {
        printf("Error al crear el hilo monitor\n");
        return EXIT_FAILURE;
    }
    
    // Iniciar el juego
    iniciarJuego();
    
    // Esperar a que termine el hilo monitor
    pthread_join(hiloMonitor, NULL);
    
    // Liberar recursos
    liberarJuego();
    
    printf("Fin del programa\n");
    
    return EXIT_SUCCESS;
}