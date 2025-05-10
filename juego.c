#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#include "juego.h"
#include "jugadores.h"
#include "mesa.h"
#include "procesos.h"
#include "utilidades.h"
#include "memoria.h"
#define _DEFAULT_SOURCE


// Variables globales para el manejo del juego
static bool juegoEnCurso = false;
static bool hayGanador = false;
static int idGanador = -1;
static int algoritmoActual = ALG_FCFS;  // FCFS por defecto

// Mutex y variables de condición para sincronización
pthread_mutex_t mutexJuego = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condTurno = PTHREAD_COND_INITIALIZER;

// Jugadores
static Jugador jugadores[MAX_JUGADORES];
static int numJugadores = 0;
static int jugadorActual = 0;

// Quantum para Round Robin
static int quantum = 3000;  // 3 segundos

// Inicializar el juego
bool inicializarJuego(int cantidadJugadores) {
    if (cantidadJugadores <= 0 || cantidadJugadores > MAX_JUGADORES) {
        printf("Número de jugadores inválido. Debe ser entre 1 y %d\n", MAX_JUGADORES);
        return false;
    }
    
    // Inicializar la semilla aleatoria
    srand((unsigned int)time(NULL));
    
    numJugadores = cantidadJugadores;
    juegoEnCurso = true;
    hayGanador = false;
    idGanador = -1;
    
    // Inicializar la mesa
    if (!inicializarMesa()) {
        printf("Error al inicializar la mesa\n");
        return false;
    }
    
    // Inicializar la tabla de procesos
    inicializarTabla();
    
    // Inicializar los jugadores
    for (int i = 0; i < numJugadores; i++) {
        inicializarJugador(&jugadores[i], i);
    }
    
    // Repartir fichas a los jugadores
    repartirFichas();
    
    return true;
}

// Repartir fichas a los jugadores
void repartirFichas() {
    // Crear el mazo completo (2 barajas + 4 comodines)
    Mazo mazoCompleto;

    // Inicializar a valores seguros
    mazoCompleto.cartas = NULL;
    mazoCompleto.numCartas = 0;
    mazoCompleto.capacidad = 0;

    crearMazoCompleto(&mazoCompleto);
    
    // Verificar si se creó correctamente
    if (mazoCompleto.cartas == NULL) {
        printf("Error: No se pudo crear el mazo completo\n");
        return;
    }

    // Mezclar el mazo
    mezclarMazo(&mazoCompleto);
    
    // Calcular cuántas cartas repartir a cada jugador (2/3 del total)
    int cartasTotales = mazoCompleto.numCartas;
    int cartasPorJugador = (cartasTotales * 2) / (3 * numJugadores);
    
    // Repartir a cada jugador
    for (int i = 0; i < numJugadores; i++) {
        for (int j = 0; j < cartasPorJugador && mazoCompleto.numCartas > 0; j++) {
            // Tomar la última carta del mazo
            Carta carta = mazoCompleto.cartas[mazoCompleto.numCartas - 1];
            mazoCompleto.numCartas--;
            
            // Añadir a la mano del jugador
            if (jugadores[i].mano.numCartas >= jugadores[i].mano.capacidad) {
                int nuevaCapacidad = jugadores[i].mano.capacidad == 0 ? cartasPorJugador : jugadores[i].mano.capacidad * 2;
                Carta *nuevasCartas = realloc(jugadores[i].mano.cartas, nuevaCapacidad * sizeof(Carta));
                
                // Verificar si realloc tuvo éxito
                if (nuevasCartas == NULL) {
                    printf("Error: No se pudo reasignar memoria para la mano del jugador %d\n", i);
                    continue; // Saltamos esta carta
                }
                
                jugadores[i].mano.cartas = nuevasCartas;
                jugadores[i].mano.capacidad = nuevaCapacidad;
            }
            
            jugadores[i].mano.cartas[jugadores[i].mano.numCartas] = carta;
            jugadores[i].mano.numCartas++;
        }
    }
    
    // El resto de cartas van a la banca
    Mazo *banca = obtenerBanca();
    if (banca == NULL) {
        printf("Error: No se pudo obtener la banca\n");
        // Liberar memoria antes de salir
        if (mazoCompleto.cartas != NULL) {
            free(mazoCompleto.cartas);
        }
        return;
    }

    for (int i = 0; i < mazoCompleto.numCartas; i++) {
        if (banca->numCartas >= banca->capacidad) {
            int nuevaCapacidad = banca->capacidad == 0 ? mazoCompleto.numCartas : banca->capacidad * 2;
            Carta *nuevasCartas = realloc(banca->cartas, nuevaCapacidad * sizeof(Carta));
            
            // Verificar si realloc tuvo éxito
            if (nuevasCartas == NULL) {
                printf("Error: No se pudo reasignar memoria para la banca\n");
                break; // Salimos del bucle
            }
            
            banca->cartas = nuevasCartas;
            banca->capacidad = nuevaCapacidad;
        }
        
        banca->cartas[banca->numCartas] = mazoCompleto.cartas[i];
        banca->numCartas++;
    }
    
    // Liberar el mazo completo
    if (mazoCompleto.cartas != NULL) {
        free(mazoCompleto.cartas);
        mazoCompleto.cartas = NULL;
    }
}

// Iniciar el juego, creando los hilos de los jugadores
void iniciarJuego() {
    // Crear los hilos de los jugadores
    for (int i = 0; i < numJugadores; i++) {
        if (pthread_create(&jugadores[i].hilo, NULL, funcionHiloJugador, (void *)&jugadores[i]) != 0) {
            printf("Error al crear el hilo del jugador %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    
    // Iniciar el bucle principal del juego (hilo juego)
    bucleJuego();
}


// Bucle principal del juego
void bucleJuego() {
    printf("¡Iniciando el juego con %d jugadores!\n", numJugadores);
    
    // Variables para control de rondas y actualización
    int numRonda = 0;
    clock_t ultimaActualizacion = clock();
    const int INTERVALO_ACTUALIZACION = 5000; // 5 segundos en ms
    
    while (juegoEnCurso) {
        // Verificar primero si el juego debe terminar
        if (juegoTerminado()) {
            break;
        }
        
        // Incrementar el contador de rondas al inicio de cada iteración
        numRonda++;
        rondaActual = numRonda;  // Actualizar variable global
        
        // Seleccionar el próximo jugador según el algoritmo de planificación
        int siguienteJugador;
        
        if (algoritmoActual == ALG_FCFS) {
            // FCFS: Seleccionar el primer jugador en estado LISTO
            siguienteJugador = seleccionarJugadorFCFS();
        } else {
            // Round Robin: Seleccionar siguiente jugador y asignar quantum
            siguienteJugador = seleccionarJugadorRR();
        }
        
        // Si no hay jugadores disponibles, esperar un poco
        if (siguienteJugador == -1) {
            usleep(100000);  // 100ms
            continue;
        }
        
        // Asignar turno al jugador seleccionado
        asignarTurno(siguienteJugador);
        
        // Esperar a que el jugador termine su turno o se agote su tiempo
        esperarFinTurno(siguienteJugador);
        
        // Verificar si hay un ganador o si el juego debe terminar
        if (hayGanador || juegoTerminado()) {
            break;
        }
        
        // Verificar si es tiempo de actualizar las estadísticas y registrar la ronda
        clock_t ahora = clock();
        int tiempoTranscurrido = (ahora - ultimaActualizacion) * 1000 / CLOCKS_PER_SEC;
        
        if (tiempoTranscurrido >= INTERVALO_ACTUALIZACION) {
            // Registrar el historial de esta ronda
            registrarHistorial(numRonda, jugadores, numJugadores);
            
            // Actualizar estadísticas
            imprimirEstadisticasTabla();
            
            // Resetear el temporizador
            ultimaActualizacion = ahora;
        }
        
        // Pequeña pausa para no consumir demasiado CPU
        usleep(10000);  // 10ms
    }
    
    // Esperar a que todos los hilos de jugadores terminen
    printf("Esperando a que terminen los hilos de los jugadores...\n");
    
    // Intentar join con los hilos con un tiempo máximo de espera
    for (int i = 0; i < numJugadores; i++) {
        // Versión simplificada sin usar pthread_timedjoin_np
        // Intentamos hacer join, pero solo esperamos un tiempo limitado
        time_t startTime = time(NULL);
        bool joined = false;
        
        // Usamos pthread_tryjoin_np si está disponible
        #ifdef __USE_GNU
        int result = pthread_tryjoin_np(jugadores[i].hilo, NULL);
        if (result == 0) {
            joined = true;
        } else {
            // Si no podemos hacer join inmediatamente, esperamos un poco e intentamos de nuevo
            while (time(NULL) - startTime < 3) { // Máximo 3 segundos de espera
                usleep(100000); // 100ms
                result = pthread_tryjoin_np(jugadores[i].hilo, NULL);
                if (result == 0) {
                    joined = true;
                    break;
                }
            }
        }
        #else
        // Si no tenemos pthread_tryjoin_np, usamos el join normal
        // pero asegurándonos de que el hilo esté en estado terminado
        jugadores[i].terminado = true;
        pthread_join(jugadores[i].hilo, NULL);
        joined = true;
        #endif
        
        if (!joined) {
            printf("No se pudo esperar al hilo del Jugador %d, continuando...\n", i);
        }
    }
    
    // Mostrar resultados finales
    mostrarResultados();
    
    // Mensaje final
    printf("\nEl historial completo del juego se ha guardado en 'historial_juego.txt'\n");
    printf("El juego ha terminado. ¡Gracias por jugar!\n");
}

// Seleccionar el próximo jugador según FCFS
int seleccionarJugadorFCFS() {
    for (int i = 0; i < numJugadores; i++) {
        // Comenzamos desde el siguiente al último jugador que tuvo turno
        int idx = (jugadorActual + i) % numJugadores;
        
        // Si el jugador está listo y no ha terminado
        if (jugadores[idx].estado == LISTO && !jugadores[idx].terminado) {
            return idx;
        }
    }
    
    return -1;  // No hay jugadores listos
}

// Seleccionar el próximo jugador según Round Robin
int seleccionarJugadorRR() {
    // En Round Robin, simplemente tomamos el siguiente jugador que no haya terminado
    int inicio = (jugadorActual + 1) % numJugadores;
    
    for (int i = 0; i < numJugadores; i++) {
        int idx = (inicio + i) % numJugadores;
        
        // Si el jugador no ha terminado
        if (!jugadores[idx].terminado) {
            return idx;
        }
    }
    
    return -1;  // No hay jugadores disponibles
}

// Asignar turno a un jugador
// En juego.c - Necesitamos modificar la función asignarTurno
void asignarTurno(int idJugador) {
    if (idJugador < 0 || idJugador >= numJugadores) {
        return;
    }
    
    jugadorActual = idJugador;
    
    // Asignar tiempo según el algoritmo
    if (algoritmoActual == ALG_FCFS) {
        // En FCFS, tiempo ilimitado (o un valor alto)
        jugadores[idJugador].tiempoTurno = 10000;  // 10 segundos
    } else {
        // En Round Robin, asignar quantum dinámico basado en número de cartas
        // Base: 1000ms + 100ms por cada carta en la mano (mínimo 1500ms, máximo 5000ms)
        int quantumDinamico = 1000 + (jugadores[idJugador].mano.numCartas * 100);
        
        // Establecer límites mínimo y máximo
        if (quantumDinamico < 1500) quantumDinamico = 1500;
        if (quantumDinamico > 5000) quantumDinamico = 5000;
        
        jugadores[idJugador].tiempoTurno = quantumDinamico;
        quantum = quantumDinamico; // Actualizar el quantum global
    }
    
    // Establecer tiempo restante
    jugadores[idJugador].tiempoRestante = jugadores[idJugador].tiempoTurno;
    
    // Marcar como turno actual
    jugadores[idJugador].turnoActual = true;
    
    printf("Turno asignado al Jugador %d por %d ms\n", idJugador, jugadores[idJugador].tiempoTurno);
}
// Esperar a que un jugador termine su turno
void esperarFinTurno(int idJugador) {
    if (idJugador < 0 || idJugador >= numJugadores) {
        return;
    }
    
    // Esperar hasta que el jugador termine su turno o se agote su tiempo
    clock_t inicio = clock();
    
    while (jugadores[idJugador].turnoActual) {
        // Verificar si el juego ha terminado
        if (juegoTerminado()) {
            // Forzar fin de turno si el juego terminó
            jugadores[idJugador].turnoActual = false;
            break;
        }
        
        // Verificar si se ha agotado el tiempo
        int tiempoTranscurrido = (clock() - inicio) * 1000 / CLOCKS_PER_SEC;
        
        if (tiempoTranscurrido >= jugadores[idJugador].tiempoTurno) {
            // Forzar fin de turno por tiempo agotado
            jugadores[idJugador].tiempoRestante = 0;
            jugadores[idJugador].turnoActual = false;
            printf("Tiempo agotado para Jugador %d\n", idJugador);
            break;
        }
        
        // Pequeña pausa para no consumir demasiado CPU
        usleep(50000);  // 50ms
    }
}

// Cambiar el algoritmo de planificación
// Cambiar el algoritmo de planificación
void cambiarAlgoritmo(int nuevoAlgoritmo) {
    if (nuevoAlgoritmo != ALG_FCFS && nuevoAlgoritmo != ALG_RR) {
        printf("Algoritmo no válido\n");
        return;
    }
    
    algoritmoActual = nuevoAlgoritmo;
    
    const char *nombres[] = {"FCFS", "Round Robin"};
    printf("Algoritmo cambiado a: %s\n", nombres[algoritmoActual]);
    
    // Explicar el comportamiento del quantum dinámico si se cambió a Round Robin
    if (nuevoAlgoritmo == ALG_RR) {
        printf("Usando quantum dinámico basado en el número de cartas:\n");
        printf("  - Base: 1000ms + 100ms por carta\n");
        printf("  - Mínimo: 1500ms, Máximo: 5000ms\n");
    }
}

// Finalizar el juego con un ganador
void finalizarJuego(int idJugadorGanador) {
    // Establecer las variables que controlan el bucle principal
    hayGanador = true;
    idGanador = idJugadorGanador;
    juegoEnCurso = false;
    
    // Registrar evento importante
    if (idJugadorGanador >= 0) {
        registrarEvento("¡El Jugador %d ha ganado el juego!", idJugadorGanador);
    } else {
        registrarEvento("Juego finalizado por el usuario");
    }
    
    // Registrar historial final
    registrarHistorial(-1, jugadores, numJugadores);
    
    // Forzar una última actualización de estadísticas
    imprimirEstadisticasTabla();
    
    // NUEVO: Mostrar estadísticas de memoria
    imprimirEstadoMemoria();
    imprimirEstadoMemoriaVirtual();
    
    // Asegurarse de que todos los jugadores estén en estado BLOQUEADO
    // Esto ayuda a que los hilos de los jugadores terminen correctamente
    for (int i = 0; i < numJugadores; i++) {
        // Interrumpir cualquier espera activa de los jugadores
        jugadores[i].terminado = true;
        jugadores[i].turnoActual = false;
        
        // NUEVO: Liberar la memoria asignada a cada jugador
        liberarMemoria(i);
        
        // Actualizar estado a BLOQUEADO
        actualizarEstadoJugador(&jugadores[i], BLOQUEADO);
    }
    
    // Mensaje de confirmación
    printf("Se guardaron todas las estadísticas. El juego ha terminado.\n");
}

// Verificar si el juego ha terminado
bool juegoTerminado() {
    return !juegoEnCurso;
}

// Mostrar resultados finales (continuación)
void mostrarResultados() {
    FILE *archivo;
    
    // Abrir archivo en modo append
    archivo = fopen("historial_juego.txt", "a");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir/crear el archivo de historial para resultados finales\n");
    } else {
        // Escribir separador para los resultados finales
        fprintf(archivo, "----------------------------------------\n");
        fprintf(archivo, "========== RESULTADOS FINALES ==========\n");
        fprintf(archivo, "----------------------------------------\n\n");
        
        if (hayGanador) {
            fprintf(archivo, "¡El Jugador %d ha ganado!\n\n", idGanador);
        } else {
            fprintf(archivo, "Juego terminado sin ganador\n\n");
        }
        
        // Escribir puntuaciones finales
        fprintf(archivo, "Puntuaciones finales:\n");
        for (int i = 0; i < numJugadores; i++) {
            fprintf(archivo, "Jugador %d: %d puntos, %d cartas restantes\n", 
                   i, jugadores[i].puntosTotal, jugadores[i].mano.numCartas);
        }
        
        // Cerrar el archivo
        fclose(archivo);
    }
    
    // Continuar con la impresión normal en la consola
    printf("\n=== RESULTADOS FINALES ===\n");
    
    if (hayGanador) {
        printf("¡El Jugador %d ha ganado!\n", idGanador);
    } else {
        printf("Juego terminado sin ganador\n");
        
        // Mostrar puntuaciones
        printf("\nPuntuaciones finales:\n");
        for (int i = 0; i < numJugadores; i++) {
            printf("Jugador %d: %d puntos, %d cartas restantes\n", 
                   i, jugadores[i].puntosTotal, jugadores[i].mano.numCartas);
        }
        
        // Determinar quien tiene menos puntos si no hay un ganador claro
        int menorPuntos = INT_MAX;
        int jugadorMenorPuntos = -1;
        
        for (int i = 0; i < numJugadores; i++) {
            int puntosTotales = calcularPuntosMano(&jugadores[i].mano);
            if (puntosTotales < menorPuntos) {
                menorPuntos = puntosTotales;
                jugadorMenorPuntos = i;
            }
        }
        
        if (jugadorMenorPuntos != -1) {
            printf("El Jugador %d tiene la menor cantidad de puntos (%d)\n", 
                   jugadorMenorPuntos, menorPuntos);
        }
    }
    
    // Mostrar estadísticas de los procesos
    imprimirEstadisticasTabla();
}

// Liberar recursos del juego
void liberarJuego() {
    // Liberar recursos de los jugadores
    for (int i = 0; i < numJugadores; i++) {
        liberarJugador(&jugadores[i]);
        
        // NUEVO: Asegurarse de que toda la memoria esté liberada
        liberarMemoria(i);
    }
    
    // Liberar recursos de la mesa
    liberarMesa();
    
    // Liberar la tabla de procesos
    liberarTabla();
    
    // NUEVO: Mostrar estadísticas finales de memoria
    imprimirEstadoMemoria();
    imprimirEstadoMemoriaVirtual();
    
    printf("Todos los recursos liberados correctamente.\n");
}
// Calcular los puntos totales de una mano
int calcularPuntosMano(Mazo *mano) {
    int total = 0;
    
    for (int i = 0; i < mano->numCartas; i++) {
        Carta carta = mano->cartas[i];
        
        if (carta.esComodin) {
            total += 20;  // Los comodines valen 20 puntos
        } else {
            // Asignar valor según la carta
            if (carta.valor >= 11) {  // J, Q, K
                total += 10;
            } else if (carta.valor == 1) {  // As
                total += 15;
            } else {
                total += carta.valor;
            }
        }
    }
    
    return total;
}