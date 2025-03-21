#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "jugadores.h"
#include "mesa.h"
#include "procesos.h"
#include "utilidades.h"

// Mutex para acceso a recursos compartidos
pthread_mutex_t mutexApeadas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexBanca = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexTabla = PTHREAD_MUTEX_INITIALIZER;

// Inicializa un jugador con sus valores por defecto
void inicializarJugador(Jugador *jugador, int id) {
    jugador->id = id;
    jugador->estado = LISTO;
    jugador->primeraApeada = false;
    jugador->tiempoTurno = 0;
    jugador->tiempoRestante = 0;
    jugador->tiempoES = 0;
    jugador->turnoActual = false;
    jugador->puntosTotal = 0;
    jugador->terminado = false;
    
    // Inicializar el mazo del jugador
    jugador->mano.cartas = NULL;
    jugador->mano.numCartas = 0;
    jugador->mano.capacidad = 0;
    
    // Crear y asociar el BCP
    jugador->bcp = crearBCP(id);
    
    // Actualizar el estado inicial en el BCP
    actualizarBCP(jugador);
}

// Función principal que ejecutará cada hilo de jugador
void *funcionHiloJugador(void *arg) {
    Jugador *jugador = (Jugador *)arg;
    
    // Registrar en tabla de procesos que el hilo ha iniciado
    pthread_mutex_lock(&mutexTabla);
    registrarProcesoEnTabla(jugador->id, PROC_BLOQUEADO);
    pthread_mutex_unlock(&mutexTabla);
    
    // Bucle principal del jugador
    while (!jugador->terminado && !juegoTerminado()) {
        // Esperar a que sea su turno
        while (!jugador->turnoActual && !jugador->terminado && !juegoTerminado()) {
            // El jugador está en espera
            if (jugador->estado == ESPERA_ES) {
                // Simular tiempo en E/S
                if (jugador->tiempoES > 0) {
                    usleep(50000);  // 50ms
                    jugador->tiempoES -= 50;
                    
                    // Actualizar el BCP
                    actualizarBCP(jugador);
                    
                    // Si ya terminó su tiempo en E/S
                    if (jugador->tiempoES <= 0) {
                        salirEsperaES(jugador);
                    }
                }
            } else {
                // Esperar a que le asignen su turno
                usleep(100000);  // 100ms
            }
        }
        
        // Si el juego terminó o el jugador terminó, salir del bucle
        if (jugador->terminado || juegoTerminado()) {
            break;
        }
        
        // Cambiar estado a EJECUCION
        actualizarEstadoJugador(jugador, EJECUCION);
        
        // Obtener referencia a las apeadas y banca
        Apeada *apeadas = obtenerApeadas();
        int numApeadas = obtenerNumApeadas();
        Mazo *banca = obtenerBanca();
        
        // Realizar el turno
        bool turnoCompletado = realizarTurno(jugador, apeadas, numApeadas, banca);
        
        // Si el jugador no pudo completar su turno en el tiempo asignado
        if (!turnoCompletado) {
            printf("Jugador %d se quedó sin tiempo en su turno\n", jugador->id);
        }
        
        // Verificar si el jugador ha terminado sus cartas
        if (jugador->mano.numCartas == 0 && banca->numCartas == 0) {
            printf("¡Jugador %d ha ganado!\n", jugador->id);
            jugador->terminado = true;
            finalizarJuego(jugador->id);
        }
        
        // Pasar el turno
        pasarTurno(jugador);
    }
    
    // Registrar en tabla de procesos que el hilo ha terminado
    pthread_mutex_lock(&mutexTabla);
    registrarProcesoEnTabla(jugador->id, PROC_BLOQUEADO);
    pthread_mutex_unlock(&mutexTabla);
    
    return NULL;
}

// Realizar el turno del jugador
bool realizarTurno(Jugador *jugador, Apeada *apeadas, int numApeadas, Mazo *banca) {
    printf("Jugador %d está ejecutando su turno\n", jugador->id);
    
    // Verificar si tiene tiempo suficiente
    if (jugador->tiempoRestante <= 0) {
        return false;
    }
    
    // Tiempo de inicio del turno
    clock_t inicio = clock();
    bool turnoCompletado = false;
    bool hizoJugada = false;
    
    // Intentar realizar jugadas mientras tenga tiempo
    while ((clock() - inicio) * 1000 / CLOCKS_PER_SEC < jugador->tiempoRestante) {
        // Si es la primera vez que se apea
        if (!jugador->primeraApeada) {
            // Verificar si puede apearse con 30 puntos o más
            if (puedeApearse(jugador)) {
                // Implementar lógica para realizar primera apeada
                // Esta función debe crear las apeadas iniciales y añadirlas a la mesa
                // realizarPrimeraApeada(jugador, apeadas, &numApeadas);
                jugador->primeraApeada = true;
                hizoJugada = true;
            }
        } else {
            // Ya se apeó anteriormente, buscar jugadas posibles
            
            // Mutex para acceder a las apeadas
            pthread_mutex_lock(&mutexApeadas);
            
            // Buscar en cada apeada si puede hacer embones o modificar
            for (int i = 0; i < numApeadas; i++) {
                // Verificar si puede hacer jugada en esta apeada
                if (verificarApeada(jugador, &apeadas[i])) {
                    // Realizar jugada en esta apeada
                    // Implementar lógica para añadir cartas a esta apeada
                    hizoJugada = true;
                    break;
                }
            }
            
            pthread_mutex_unlock(&mutexTabla);
        }
        
        // Si no pudo hacer ninguna jugada, comer ficha si hay disponibles
        if (!hizoJugada) {
            pthread_mutex_lock(&mutexBanca);
            
            if (banca->numCartas > 0) {
                bool comio = comerFicha(jugador, banca);
                if (comio) {
                    // Entrar en estado de E/S después de comer
                    entrarEsperaES(jugador);
                    turnoCompletado = true;
                    pthread_mutex_unlock(&mutexTabla);
                    break;
                }
            } else {
                // No hay fichas para comer y no puede hacer jugada
                printf("Jugador %d no puede hacer jugada y no hay fichas para comer\n", jugador->id);
            }
            
            pthread_mutex_unlock(&mutexTabla);
        }
        
        // Si hizo alguna jugada, actualizar su estado
        if (hizoJugada) {
            // Verificar si terminó sus cartas
            if (jugador->mano.numCartas == 0) {
                turnoCompletado = true;
                break;
            }
        }
        
        // Pequeña pausa para no consumir demasiado CPU
        usleep(10000);  // 10ms
    }
    
    // Actualizar tiempo restante
    jugador->tiempoRestante -= (clock() - inicio) * 1000 / CLOCKS_PER_SEC;
    
    // Si el tiempo llegó a 0, el turno se considera completado
    if (jugador->tiempoRestante <= 0) {
        jugador->tiempoRestante = 0;
        turnoCompletado = true;
    }
    
    return turnoCompletado;
}

// Verificar si una apeada puede ser modificada por un jugador
bool verificarApeada(Jugador *jugador, Apeada *apeada) {
    // Esta función debe implementar la lógica para verificar si el jugador
    // puede modificar la apeada (añadir cartas, hacer embones, etc.)
    
    // Por ahora devolvemos un valor fijo (implementación pendiente)
    return false;
}

// Verificar si el jugador puede hacer su primera apeada
bool puedeApearse(Jugador *jugador) {
    // Esta función debe implementar la lógica para verificar si el jugador
    // tiene suficientes puntos (30 o más) para hacer su primera apeada
    
    // Por ahora devolvemos un valor fijo (implementación pendiente)
    return false;
}

// Comer una ficha de la banca
bool comerFicha(Jugador *jugador, Mazo *banca) {
    if (banca->numCartas <= 0) {
        return false;
    }
    
    // Tomar una carta de la banca
    Carta carta = banca->cartas[banca->numCartas - 1];
    banca->numCartas--;
    
    // Añadir carta a la mano del jugador
    if (jugador->mano.numCartas >= jugador->mano.capacidad) {
        // Ampliar capacidad si es necesario
        int nuevaCapacidad = jugador->mano.capacidad == 0 ? 10 : jugador->mano.capacidad * 2;
        jugador->mano.cartas = realloc(jugador->mano.cartas, nuevaCapacidad * sizeof(Carta));
        jugador->mano.capacidad = nuevaCapacidad;
    }
    
    // Añadir la carta al mazo del jugador
    jugador->mano.cartas[jugador->mano.numCartas] = carta;
    jugador->mano.numCartas++;
    
    printf("Jugador %d comió una ficha: %d de %c\n", jugador->id, carta.valor, carta.palo);
    
    return true;
}

// Actualizar el estado del jugador
void actualizarEstadoJugador(Jugador *jugador, EstadoJugador nuevoEstado) {
    jugador->estado = nuevoEstado;
    
    // Actualizar el BCP
    actualizarBCP(jugador);
    
    // Actualizar en la tabla de procesos
    pthread_mutex_lock(&mutexTabla);
    actualizarProcesoEnTabla(jugador->id, nuevoEstado);
    pthread_mutex_unlock(&mutexTabla);
    
    // Mostrar cambio de estado
    const char *estados[] = {"LISTO", "EJECUCION", "ESPERA_ES", "BLOQUEADO"};
    printf("Jugador %d cambió a estado: %s\n", jugador->id, estados[nuevoEstado]);
}

// Pasar el turno del jugador
void pasarTurno(Jugador *jugador) {
    jugador->turnoActual = false;
    
    // Cambiar a estado LISTO o ESPERA_ES según corresponda
    if (jugador->estado != ESPERA_ES) {
        actualizarEstadoJugador(jugador, LISTO);
    }
}

// Entrar en estado de espera E/S
void entrarEsperaES(Jugador *jugador) {
    // Tiempo aleatorio entre 2 y 5 segundos
    jugador->tiempoES = (rand() % 3 + 2) * 1000;
    actualizarEstadoJugador(jugador, ESPERA_ES);
    printf("Jugador %d entró en E/S por %d ms\n", jugador->id, jugador->tiempoES);
}

// Salir del estado de espera E/S
void salirEsperaES(Jugador *jugador) {
    jugador->tiempoES = 0;
    actualizarEstadoJugador(jugador, LISTO);
    printf("Jugador %d salió de E/S\n", jugador->id);
}

// Actualizar el BCP del jugador
void actualizarBCP(Jugador *jugador) {
    if (jugador->bcp != NULL) {
        // Actualizar las variables del BCP
        jugador->bcp->estado = jugador->estado;
        jugador->bcp->tiempoEspera = jugador->tiempoES;
        jugador->bcp->tiempoEjecucion = jugador->tiempoRestante;
        jugador->bcp->prioridad = jugador->id;  // Por ahora usamos el ID como prioridad
        jugador->bcp->numCartas = jugador->mano.numCartas;
        jugador->bcp->turnoActual = jugador->turnoActual;
        
        // Guardar el BCP en archivo
        guardarBCP(jugador->bcp);
    }
}

// Liberar recursos del jugador
void liberarJugador(Jugador *jugador) {
    if (jugador->mano.cartas != NULL) {
        free(jugador->mano.cartas);
        jugador->mano.cartas = NULL;
    }
    
    if (jugador->bcp != NULL) {
        liberarBCP(jugador->bcp);
        jugador->bcp = NULL;
    }
}