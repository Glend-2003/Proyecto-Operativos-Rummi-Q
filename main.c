#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h> // Necesario para strcmp, strlen, etc. (aunque no se usa directamente aquí)

#include "juego.h"      // Contiene MAX_JUGADORES, ALG_FCFS, ALG_RR
#include "jugadores.h"
#include "mesa.h"       // Asegúrate que este archivo existe y está bien definido
#include "procesos.h"
#include "utilidades.h"
#include "memoria.h"    // Contiene ALG_AJUSTE_OPTIMO, ALG_LRU, ALG_MAPA_BITS

#define _DEFAULT_SOURCE // Para usleep, etc.

/* Función para leer una tecla sin bloqueo (tu implementación actual) */
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

/* Hilo para monitorear cambios de algoritmo y otras teclas */
void *monitorTeclas(void *arg) {
    printf("Hilo monitor de teclas iniciado. Presiona 'h' para ver los controles.\n");
    while (!juegoTerminado()) { // Usa la función de juego.h para saber si el juego terminó
        int tecla = leerTecla();

        if (tecla != EOF) { // Solo procesa si se leyó una tecla
            if (tecla == '1') {
                colorAmarillo();
                printf("\n[TECLA] Cambiando algoritmo de CPU a FCFS...\n");
                colorReset();
                cambiarAlgoritmo(ALG_FCFS); // De juego.c para planificación de CPU
            } else if (tecla == '2') {
                colorAmarillo();
                printf("\n[TECLA] Cambiando algoritmo de CPU a Round Robin...\n");
                colorReset();
                cambiarAlgoritmo(ALG_RR);   // De juego.c para planificación de CPU
            } else if (tecla == '3') {
                colorAmarillo();
                printf("\n[TECLA] Cambiando algoritmo de MEMORIA DE PARTICIÓN a AJUSTE ÓPTIMO...\n");
                colorReset();
                // Esta función es de memoria.c. Asegúrate que maneja ALG_AJUSTE_OPTIMO
                cambiarAlgoritmoMemoria(ALG_AJUSTE_OPTIMO);
            } else if (tecla == '4') {
                colorAmarillo();
                printf("\n[TECLA] Activando/Visualizando MEMORIA VIRTUAL con LRU...\n");
                colorReset();
                // Esta función es de memoria.c. Tu función cambiarAlgoritmoMemoria
                // ya tiene una opción para ALG_LRU. El efecto exacto de esto
                // (si solo cambia una bandera interna o tiene más implicaciones)
                // depende de tu implementación en memoria.c.
                // El enunciado pide implementar LRU, no necesariamente "cambiar a LRU" como
                // si fuera un modo exclusivo contra particiones. La memoria virtual con LRU
                // debería estar activa para las páginas de los procesos.
                // Esta tecla podría simplemente forzar la impresión del estado de la memoria virtual.
                cambiarAlgoritmoMemoria(ALG_LRU); // O simplemente llamar a imprimirEstadoMemoriaVirtual()
                                                 // si cambiarAlgoritmoMemoria(ALG_LRU) no tiene el efecto deseado.
                // imprimirEstadoMemoriaVirtual(); // Podrías llamarlo aquí directamente también
            } else if (tecla == '5') {
                colorAmarillo();
                printf("\n[TECLA] Cambiando algoritmo de MEMORIA DE PARTICIÓN a MAPA DE BITS...\n");
                colorReset();
                // Esta función es de memoria.c. Asegúrate que maneja ALG_MAPA_BITS
                // y que esta constante esté definida en memoria.h y usada en memoria.c
                cambiarAlgoritmoMemoria(ALG_MAPA_BITS);
            } else if (tecla == 'm' || tecla == 'M') {
                colorAmarillo();
                printf("\n[TECLA] Mostrando estado actual de la memoria...\n");
                colorReset();
                imprimirEstadoMemoria();        // Muestra estado de particiones (Ajuste Óptimo o Mapa de Bits)
                imprimirEstadoMemoriaVirtual(); // Muestra estado de la memoria virtual (LRU)
            } else if (tecla == 'q' || tecla == 'Q') {
                colorRojo();
                printf("\n[TECLA] Solicitud para salir del juego...\n");
                colorReset();
                finalizarJuego(-1); // -1 indica que no hay un jugador ganador específico, salida por usuario
                // El break aquí saldrá del bucle while en monitorTeclas.
                // juegoTerminado() será true después de llamar a finalizarJuego().
                break;
            } else if (tecla == 'h' || tecla == 'H') {
                // Llamar a mostrarInformacion() puede ser mucho texto aquí.
                // Mejor imprimir solo los controles.
                printf("\nCONTROLES DISPONIBLES:\n");
                printf("  '1': CPU FCFS         '2': CPU Round Robin\n");
                printf("  '3': Mem Part. AJUSTE ÓPTIMO\n");
                printf("  '4': Mem Virt. LRU (visualizar/activar)\n");
                printf("  '5': Mem Part. MAPA DE BITS\n");
                printf("  'm': Mostrar estado de TODA la memoria\n");
                printf("  'q': Salir del juego\n");
                printf("  'h': Mostrar esta ayuda\n");
            }
        }
        usleep(100000); // 0.1 segundos de espera para no consumir demasiado CPU
    }
    printf("Hilo de monitoreo de teclas finalizado.\n");
    return NULL;
}

void mostrarInformacion(void) {
    colorCian();
    printf("\n==============================================\n");
    printf("      JUEGO RUMMI - SIMULADOR SO (G5)      \n");
    printf("==============================================\n\n");
    colorReset();

    printf("Este programa simula el juego de Rummy para demostrar conceptos de\n");
    printf("planificación de CPU y gestión de memoria en sistemas operativos.\n\n");

    printf("REGLAS BÁSICAS DEL JUEGO (Simplificado):\n");
    printf("- Objetivo: Descartar todas las cartas formando jugadas.\n");
    printf("- Jugadas: Grupos (mismo valor, != palo) o Escaleras (consecutivas, mismo palo).\n");
    printf("- Primera apeada requiere 30+ puntos.\n\n");

    printf("ALGORITMOS DE PLANIFICACIÓN DE CPU:\n");
    printf("- FCFS (First-Come, First-Served): Atiende en orden de llegada a listos.\n");
    printf("- Round Robin: Asigna un quantum de tiempo por turno.\n\n");

    printf("GESTIÓN DE MEMORIA (PARTICIONES):\n");
    printf("  Se simula la asignación de memoria física a los procesos cuando entran a E/S.\n");
    printf("  Dos procesos elegidos aleatoriamente pueden solicitar crecer en memoria.\n");
    printf("- Ajuste Óptimo: Usa la partición libre más pequeña que pueda alojar al proceso.\n");
    printf("- Mapa de Bits: Divide la memoria en bloques fijos y usa un bitmap para rastrear ocupación.\n\n");

    printf("GESTIÓN DE MEMORIA (VIRTUAL CON INTERCAMBIO):\n");
    printf("  Cada 'ficha' (carta) de un jugador se considera una página lógica.\n");
    printf("  La memoria principal tiene un número limitado de marcos.\n");
    printf("  Cuando se accede a una página no presente, ocurre un fallo de página.\n");
    printf("- LRU (Least Recently Used): Si no hay marcos libres, se reemplaza la página\n");
    printf("    que no ha sido usada en más tiempo.\n\n");

    colorVerde();
    printf("CONTROLES DURANTE EL JUEGO:\n");
    printf("  '1': Cambiar algoritmo de CPU a FCFS.\n");
    printf("  '2': Cambiar algoritmo de CPU a Round Robin.\n");
    printf("  '3': Cambiar algoritmo de MEMORIA DE PARTICIÓN a AJUSTE ÓPTIMO.\n");
    printf("  '4': Activar/Visualizar MEMORIA VIRTUAL con LRU (el efecto depende de `cambiarAlgoritmoMemoria`).\n");
    printf("  '5': Cambiar algoritmo de MEMORIA DE PARTICIÓN a MAPA DE BITS.\n");
    printf("  'm': Mostrar estado actual de la memoria (particiones y virtual).\n");
    printf("  'q': Salir del juego.\n");
    printf("  'h': Mostrar esta ayuda de controles nuevamente.\n\n");
    colorReset();
}

int main(int argc, char *argv[]) {
    int numJugadores = 4; // Por defecto 4 jugadores según el enunciado
    int opcion;

    srand(time(NULL)); // Inicializar semilla para números aleatorios una vez

    mostrarInformacion();

    printf("¿Desea iniciar el juego con %d jugadores? (1: Sí, 0: No): ", numJugadores);
    if (scanf("%d", &opcion) != 1) { // Verificar que scanf leyó un número
        printf("Entrada inválida. Saliendo.\n");
        return EXIT_FAILURE;
    }
    while (getchar() != '\n'); // Limpiar buffer de entrada

    if (opcion != 1) {
        printf("Juego cancelado por el usuario.\n");
        return EXIT_SUCCESS;
    }

    system("mkdir -p bcp"); // Crear directorio para los BCP si no existe

    // Inicializar el sistema de memoria (incluye particiones y virtual)
    inicializarMemoria(); // De memoria.c

    colorVerde();
    printf("\nInicializando componentes del juego con %d jugadores...\n", numJugadores);
    colorReset();

    // Inicializar el juego (repartir cartas, crear jugadores, etc.)
    if (!inicializarJuego(numJugadores)) { // De juego.c
        colorRojo();
        printf("Error crítico al inicializar el juego.\n");
        colorReset();
        return EXIT_FAILURE;
    }

    pthread_t hiloMonitor;
    if (pthread_create(&hiloMonitor, NULL, monitorTeclas, NULL) != 0) {
        colorRojo();
        printf("Error crítico al crear el hilo monitor de teclas.\n");
        colorReset();
        // Considerar limpiar recursos si el juego ya se inicializó parcialmente
        return EXIT_FAILURE;
    }

    // iniciarJuego() debe contener el bucle principal del juego que hace avanzar los turnos
    // y llama a las funciones de los hilos de los jugadores.
    // Este es el hilo principal del juego.
    iniciarJuego(); // De juego.c - Esta función bloqueará hasta que el juego termine

    // Una vez que iniciarJuego() retorna, significa que el juego ha terminado.
    // El hilo monitorTeclas debería haber terminado también porque juegoTerminado() será true.
    printf("El bucle principal del juego ha finalizado. Esperando al hilo monitor...\n");
    pthread_join(hiloMonitor, NULL); // Esperar a que el hilo monitor termine limpiamente

    printf("\nJuego finalizado. Mostrando estadísticas finales y liberando recursos...\n");

    // Mostrar estadísticas finales de memoria (ya se llama en finalizarJuego, pero por si acaso)
    // imprimirEstadoMemoria();
    // imprimirEstadoMemoriaVirtual();

    // Liberar todos los recursos del juego (incluida la memoria simulada)
    liberarJuego(); // De juego.c

    colorCian();
    printf("\nSimulación del juego Rummy ha finalizado correctamente.\n");
    colorReset();

    return EXIT_SUCCESS;
}