#include "procesos.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

// Variables globales
static TablaProcesos tabla_procesos;
static ColaES cola_es;
static int next_pid = 1;

// Nombres de archivos para BCP y tabla de procesos
static const char* ARCHIVO_BCP_BASE = "bcp_jugador_";
static const char* ARCHIVO_TABLA_PROCESOS = "tabla_procesos.txt";

// Inicializar el sistema de procesos
void inicializar_sistema_procesos() {
    // Inicializar la tabla de procesos
    tabla_procesos.num_procesos = 0;
    tabla_procesos.proceso_actual = -1;
    pthread_mutex_init(&tabla_procesos.mutex, NULL);
    tabla_procesos.algoritmo = FCFS; // Por defecto, usar FCFS
    tabla_procesos.quantum_global = 3; // 3 segundos por defecto para RR
    tabla_procesos.cambios_contexto = 0;
    tabla_procesos.procesos_bloqueados = 0;
    tabla_procesos.procesos_listos = 0;
    tabla_procesos.planificador_activo = true;
    tabla_procesos.tiempo_inicio_juego = time(NULL);
    tabla_procesos.turnos_completados = 0;
    tabla_procesos.max_tiempo_ejecucion = 10; // 10 segundos máximo por turno

    // Inicializar la cola de E/S
    cola_es.num_procesos = 0;
    pthread_mutex_init(&cola_es.mutex, NULL);
    cola_es.activo = true;

    // Crear hilos del planificador y gestor de E/S
    pthread_create(&tabla_procesos.hilo_planificador, NULL, hilo_planificador, NULL);
    pthread_create(&cola_es.hilo_es, NULL, hilo_gestor_es, NULL);

    // Crear los procesos para cada jugador
    for (int i = 0; i < NUM_JUGADORES; i++) {
        crear_proceso(i);
    }

    printf("Sistema de procesos inicializado correctamente.\n");
    guardar_estado_tabla_procesos();
}

// Crear un proceso para un jugador
void crear_proceso(int jugador_id) {
    if (tabla_procesos.num_procesos >= NUM_JUGADORES) {
        printf("Error: No se pueden crear más procesos, límite alcanzado.\n");
        return;
    }

    BCP *nuevo_bcp = (BCP*)malloc(sizeof(BCP));
    if (!nuevo_bcp) {
        printf("Error: No se pudo asignar memoria para el nuevo BCP.\n");
        return;
    }

    // Inicializar valores del BCP
    nuevo_bcp->pid = next_pid++;
    nuevo_bcp->jugador_id = jugador_id;
    nuevo_bcp->estado = PROCESO_NUEVO;
    nuevo_bcp->tiempo_creacion = time(NULL);
    nuevo_bcp->tiempo_inicio = 0;
    nuevo_bcp->tiempo_fin = 0;
    nuevo_bcp->prioridad = jugador_id + 1; // Prioridad basada en el ID
    nuevo_bcp->tiempo_cpu = 0;
    nuevo_bcp->tiempo_espera = 0;
    nuevo_bcp->tiempo_restante = tabla_procesos.max_tiempo_ejecucion;
    nuevo_bcp->quantum = tabla_procesos.quantum_global;
    nuevo_bcp->turnos_perdidos = 0;
    nuevo_bcp->puntos_acumulados = 0;
    nuevo_bcp->fichas_colocadas = 0;
    nuevo_bcp->primera_apeada = false;
    nuevo_bcp->tiempo_bloqueado = 0;
    nuevo_bcp->intentos_apeada = 0;

    // Agregar a la tabla de procesos
    pthread_mutex_lock(&tabla_procesos.mutex);
    tabla_procesos.procesos[tabla_procesos.num_procesos] = nuevo_bcp;
    tabla_procesos.num_procesos++;
    tabla_procesos.procesos_listos++;
    pthread_mutex_unlock(&tabla_procesos.mutex);

    // Cambiar el estado a LISTO
    actualizar_bcp(nuevo_bcp->pid, PROCESO_LISTO);

    printf("Proceso creado para el jugador %d con PID %d\n", jugador_id, nuevo_bcp->pid);
    guardar_estado_bcp(nuevo_bcp->pid);
}

// Destruir un proceso
void destruir_proceso(int pid) {
    pthread_mutex_lock(&tabla_procesos.mutex);
    for (int i = 0; i < tabla_procesos.num_procesos; i++) {
        if (tabla_procesos.procesos[i]->pid == pid) {
            BCP *bcp = tabla_procesos.procesos[i];
            
            // Actualizar estadísticas finales
            bcp->estado = PROCESO_TERMINADO;
            bcp->tiempo_fin = time(NULL);
            
            // Guardar el estado final
            guardar_estado_bcp(pid);
            
            // Si es el proceso actual, limpiar
            if (tabla_procesos.proceso_actual == i) {
                tabla_procesos.proceso_actual = -1;
            }
            
            // Liberar memoria y reorganizar la tabla
            free(bcp);
            for (int j = i; j < tabla_procesos.num_procesos - 1; j++) {
                tabla_procesos.procesos[j] = tabla_procesos.procesos[j + 1];
            }
            tabla_procesos.num_procesos--;
            
            break;
        }
    }
    pthread_mutex_unlock(&tabla_procesos.mutex);
    
    guardar_estado_tabla_procesos();
    printf("Proceso %d destruido\n", pid);
}

// Cambiar el algoritmo de planificación
void cambiar_algoritmo(TipoAlgoritmo nuevo_algoritmo) {
    pthread_mutex_lock(&tabla_procesos.mutex);
    tabla_procesos.algoritmo = nuevo_algoritmo;
    pthread_mutex_unlock(&tabla_procesos.mutex);
    
    const char* nombre_algoritmo = (nuevo_algoritmo == FCFS) ? "FCFS" : "Round Robin";
    printf("Algoritmo de planificación cambiado a: %s\n", nombre_algoritmo);
    
    guardar_estado_tabla_procesos();
}

// Planificar el siguiente proceso según el algoritmo actual
void planificar_siguiente_proceso() {
    pthread_mutex_lock(&tabla_procesos.mutex);
    
    int proceso_anterior = tabla_procesos.proceso_actual;
    
    // Si hay un proceso actual ejecutándose, marcarlo como listo
    if (proceso_anterior >= 0 && proceso_anterior < tabla_procesos.num_procesos) {
        BCP *bcp_actual = tabla_procesos.procesos[proceso_anterior];
        if (bcp_actual->estado == PROCESO_EJECUTANDO) {
            bcp_actual->estado = PROCESO_LISTO;
            guardar_estado_bcp(bcp_actual->pid);
        }
    }
    
    // Seleccionar el siguiente proceso según el algoritmo
    if (tabla_procesos.algoritmo == FCFS) {
        // First Come First Served: simplemente tomar el siguiente en la lista
        if (tabla_procesos.procesos_listos > 0) {
            for (int i = 0; i < tabla_procesos.num_procesos; i++) {
                if (tabla_procesos.procesos[i]->estado == PROCESO_LISTO) {
                    tabla_procesos.proceso_actual = i;
                    break;
                }
            }
        } else {
            tabla_procesos.proceso_actual = -1; // No hay procesos listos
        }
    } else if (tabla_procesos.algoritmo == RR) {
        // Round Robin: rotar entre procesos listos
        if (tabla_procesos.procesos_listos > 0) {
            int siguiente = (tabla_procesos.proceso_actual + 1) % tabla_procesos.num_procesos;
            int iteraciones = 0;
            
            // Buscar el siguiente proceso listo
            while (iteraciones < tabla_procesos.num_procesos) {
                if (tabla_procesos.procesos[siguiente]->estado == PROCESO_LISTO) {
                    tabla_procesos.proceso_actual = siguiente;
                    break;
                }
                siguiente = (siguiente + 1) % tabla_procesos.num_procesos;
                iteraciones++;
            }
            
            // Si no encontramos un proceso listo después de recorrer todos
            if (iteraciones >= tabla_procesos.num_procesos) {
                tabla_procesos.proceso_actual = -1;
            }
        } else {
            tabla_procesos.proceso_actual = -1; // No hay procesos listos
        }
    }
    
    // Si se ha seleccionado un nuevo proceso, marcarlo como EJECUTANDO
    if (tabla_procesos.proceso_actual >= 0) {
        BCP *nuevo_proceso = tabla_procesos.procesos[tabla_procesos.proceso_actual];
        nuevo_proceso->estado = PROCESO_EJECUTANDO;
        
        // Si es la primera vez que se ejecuta este proceso
        if (nuevo_proceso->tiempo_inicio == 0) {
            nuevo_proceso->tiempo_inicio = time(NULL);
        }
        
        // Registrar el cambio de contexto
        tabla_procesos.ultimo_cambio_contexto = time(NULL);
        tabla_procesos.cambios_contexto++;
        
        guardar_estado_bcp(nuevo_proceso->pid);
    }
    
    pthread_mutex_unlock(&tabla_procesos.mutex);
    guardar_estado_tabla_procesos();
}

// Hilo del planificador
void* hilo_planificador(void *arg) {
    while (tabla_procesos.planificador_activo) {
        // Verificar tiempos de ejecución y quantum
        pthread_mutex_lock(&tabla_procesos.mutex);
        if (tabla_procesos.proceso_actual >= 0) {
            BCP *proceso_actual = tabla_procesos.procesos[tabla_procesos.proceso_actual];
            time_t tiempo_actual = time(NULL);
            int tiempo_transcurrido = (int)(tiempo_actual - tabla_procesos.ultimo_cambio_contexto);
            
            // Actualizar tiempo de CPU
            proceso_actual->tiempo_cpu += tiempo_transcurrido;
            
            // Verificar si se agotó el tiempo máximo de ejecución
            if (proceso_actual->tiempo_cpu >= tabla_procesos.max_tiempo_ejecucion) {
                printf("Proceso %d (Jugador %d) agotó su tiempo máximo de ejecución\n", 
                       proceso_actual->pid, proceso_actual->jugador_id);
                proceso_actual->turnos_perdidos++;
                registrar_turno_perdido(proceso_actual->pid);
                proceso_actual->estado = PROCESO_LISTO;
                guardar_estado_bcp(proceso_actual->pid);
            }
            
            // En caso de Round Robin, verificar el quantum
            if (tabla_procesos.algoritmo == RR) {
                proceso_actual->tiempo_restante -= tiempo_transcurrido;
                if (proceso_actual->tiempo_restante <= 0) {
                    printf("Proceso %d (Jugador %d) agotó su quantum\n", 
                           proceso_actual->pid, proceso_actual->jugador_id);
                    proceso_actual->estado = PROCESO_LISTO;
                    proceso_actual->tiempo_restante = tabla_procesos.quantum_global;
                    guardar_estado_bcp(proceso_actual->pid);
                }
            }
        }
        pthread_mutex_unlock(&tabla_procesos.mutex);
        
        // Planificar el siguiente proceso
        planificar_siguiente_proceso();
        
        // Esperar antes de la próxima planificación
        sleep(1);
    }
    
    return NULL;
}

// Bloquear un proceso y enviarlo a la cola de E/S
void bloquear_proceso(int pid, int tiempo_espera) {
    pthread_mutex_lock(&tabla_procesos.mutex);
    for (int i = 0; i < tabla_procesos.num_procesos; i++) {
        if (tabla_procesos.procesos[i]->pid == pid) {
            BCP *bcp = tabla_procesos.procesos[i];
            bcp->estado = PROCESO_BLOQUEADO;
            bcp->tiempo_bloqueado = time(NULL) + tiempo_espera;
            tabla_procesos.procesos_bloqueados++;
            tabla_procesos.procesos_listos--;
            
            // Si es el proceso actual, necesitamos planificar otro
            if (tabla_procesos.proceso_actual == i) {
                tabla_procesos.proceso_actual = -1;
            }
            
            guardar_estado_bcp(pid);
            break;
        }
    }
    pthread_mutex_unlock(&tabla_procesos.mutex);
    
    // Agregar a la cola de E/S
    pthread_mutex_lock(&cola_es.mutex);
    cola_es.procesos_ids[cola_es.num_procesos] = pid;
    cola_es.tiempos_espera[cola_es.num_procesos] = tiempo_espera;
    cola_es.num_procesos++;
    pthread_mutex_unlock(&cola_es.mutex);
    
    printf("Proceso %d bloqueado por %d segundos\n", pid, tiempo_espera);
    guardar_estado_tabla_procesos();
}

// Hilo gestor de E/S
void* hilo_gestor_es(void *arg) {
    while (cola_es.activo) {
        verificar_procesos_es();
        sleep(1);
    }
    return NULL;
}

// Verificar procesos en E/S
void verificar_procesos_es() {
    pthread_mutex_lock(&cola_es.mutex);
    time_t tiempo_actual = time(NULL);
    int i = 0;
    
    while (i < cola_es.num_procesos) {
        int pid = cola_es.procesos_ids[i];
        BCP *bcp = obtener_bcp(pid);
        
        if (bcp && bcp->tiempo_bloqueado <= tiempo_actual) {
            // El tiempo de bloqueo ha terminado, mover a estado LISTO
            pthread_mutex_lock(&tabla_procesos.mutex);
            bcp->estado = PROCESO_LISTO;
            tabla_procesos.procesos_bloqueados--;
            tabla_procesos.procesos_listos++;
            pthread_mutex_unlock(&tabla_procesos.mutex);
            
            printf("Proceso %d desbloqueado y listo para ejecutar\n", pid);
            guardar_estado_bcp(pid);
            
            // Eliminar de la cola de E/S
            for (int j = i; j < cola_es.num_procesos - 1; j++) {
                cola_es.procesos_ids[j] = cola_es.procesos_ids[j + 1];
                cola_es.tiempos_espera[j] = cola_es.tiempos_espera[j + 1];
            }
            cola_es.num_procesos--;
        } else {
            i++; // Avanzar al siguiente proceso solo si este no se eliminó
        }
    }
    
    pthread_mutex_unlock(&cola_es.mutex);
}

// Actualizar el estado de un BCP
void actualizar_bcp(int pid, EstadoProceso nuevo_estado) {
    pthread_mutex_lock(&tabla_procesos.mutex);
    for (int i = 0; i < tabla_procesos.num_procesos; i++) {
        if (tabla_procesos.procesos[i]->pid == pid) {
            BCP *bcp = tabla_procesos.procesos[i];
            
            // Actualizar contadores de procesos según cambio de estado
            if (bcp->estado == PROCESO_LISTO && nuevo_estado != PROCESO_LISTO) {
                tabla_procesos.procesos_listos--;
            } else if (bcp->estado != PROCESO_LISTO && nuevo_estado == PROCESO_LISTO) {
                tabla_procesos.procesos_listos++;
            }
            
            if (bcp->estado == PROCESO_BLOQUEADO && nuevo_estado != PROCESO_BLOQUEADO) {
                tabla_procesos.procesos_bloqueados--;
            } else if (bcp->estado != PROCESO_BLOQUEADO && nuevo_estado == PROCESO_BLOQUEADO) {
                tabla_procesos.procesos_bloqueados++;
            }
            
            bcp->estado = nuevo_estado;
            guardar_estado_bcp(pid);
            break;
        }
    }
    pthread_mutex_unlock(&tabla_procesos.mutex);
    
    guardar_estado_tabla_procesos();
}

// Registrar un turno completado
void registrar_turno_completado(int pid) {
    pthread_mutex_lock(&tabla_procesos.mutex);
    tabla_procesos.turnos_completados++;
    pthread_mutex_unlock(&tabla_procesos.mutex);
    
    guardar_estado_tabla_procesos();
}

// Registrar un turno perdido
void registrar_turno_perdido(int pid) {
    BCP *bcp = obtener_bcp(pid);
    if (bcp) {
        bcp->turnos_perdidos++;
        guardar_estado_bcp(pid);
    }
}

// Actualizar puntos de un jugador
void actualizar_puntos(int pid, int puntos) {
    BCP *bcp = obtener_bcp(pid);
    if (bcp) {
        bcp->puntos_acumulados += puntos;
        guardar_estado_bcp(pid);
    }
}

// Obtener el BCP de un proceso
BCP* obtener_bcp(int pid) {
    BCP *resultado = NULL;
    
    pthread_mutex_lock(&tabla_procesos.mutex);
    for (int i = 0; i < tabla_procesos.num_procesos; i++) {
        if (tabla_procesos.procesos[i]->pid == pid) {
            resultado = tabla_procesos.procesos[i];
            break;
        }
    }
    pthread_mutex_unlock(&tabla_procesos.mutex);
    
    return resultado;
}

// Obtener la tabla de procesos
TablaProcesos* obtener_tabla_procesos() {
    return &tabla_procesos;
}

// Guardar el estado de un BCP en un archivo
void guardar_estado_bcp(int pid) {
    BCP *bcp = obtener_bcp(pid);
    if (!bcp) return;
    
    char filename[100];
    sprintf(filename, "%s%d.txt", ARCHIVO_BCP_BASE, bcp->jugador_id);
    
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error: No se pudo abrir el archivo %s para escritura.\n", filename);
        return;
    }
    
    // Obtener timestamps legibles
    char tiempo_creacion[30];
    char tiempo_inicio[30];
    char tiempo_fin[30];
    char tiempo_bloqueado[30];
    
    struct tm *tm_info;
    
    tm_info = localtime(&bcp->tiempo_creacion);
    strftime(tiempo_creacion, sizeof(tiempo_creacion), "%Y-%m-%d %H:%M:%S", tm_info);
    
    if (bcp->tiempo_inicio > 0) {
        tm_info = localtime(&bcp->tiempo_inicio);
        strftime(tiempo_inicio, sizeof(tiempo_inicio), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strcpy(tiempo_inicio, "No iniciado");
    }
    
    if (bcp->tiempo_fin > 0) {
        tm_info = localtime(&bcp->tiempo_fin);
        strftime(tiempo_fin, sizeof(tiempo_fin), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strcpy(tiempo_fin, "No finalizado");
    }
    
    if (bcp->tiempo_bloqueado > 0) {
        tm_info = localtime(&bcp->tiempo_bloqueado);
        strftime(tiempo_bloqueado, sizeof(tiempo_bloqueado), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strcpy(tiempo_bloqueado, "No bloqueado");
    }
    
    // Convertir estado a string
    const char *estado_str;
    switch(bcp->estado) {
        case PROCESO_NUEVO: estado_str = "NUEVO"; break;
        case PROCESO_LISTO: estado_str = "LISTO"; break;
        case PROCESO_EJECUTANDO: estado_str = "EJECUTANDO"; break;
        case PROCESO_BLOQUEADO: estado_str = "BLOQUEADO"; break;
        case PROCESO_TERMINADO: estado_str = "TERMINADO"; break;
        default: estado_str = "DESCONOCIDO";
    }
    
    // Escribir información al archivo
    fprintf(file, "=== BLOQUE DE CONTROL DE PROCESO ===\n");
    fprintf(file, "PID: %d\n", bcp->pid);
    fprintf(file, "Jugador ID: %d\n", bcp->jugador_id);
    fprintf(file, "Estado: %s\n", estado_str);
    fprintf(file, "Tiempo de creación: %s\n", tiempo_creacion);
    fprintf(file, "Tiempo de inicio: %s\n", tiempo_inicio);
    fprintf(file, "Tiempo de fin: %s\n", tiempo_fin);
    fprintf(file, "Prioridad: %d\n", bcp->prioridad);
    fprintf(file, "Tiempo de CPU: %d segundos\n", bcp->tiempo_cpu);
    fprintf(file, "Tiempo de espera: %d segundos\n", bcp->tiempo_espera);
    fprintf(file, "Tiempo restante: %d segundos\n", bcp->tiempo_restante);
    fprintf(file, "Quantum: %d segundos\n", bcp->quantum);
    fprintf(file, "Turnos perdidos: %d\n", bcp->turnos_perdidos);
    fprintf(file, "Puntos acumulados: %d\n", bcp->puntos_acumulados);
    fprintf(file, "Fichas colocadas: %d\n", bcp->fichas_colocadas);
    fprintf(file, "Primera apeada realizada: %s\n", bcp->primera_apeada ? "Sí" : "No");
    fprintf(file, "Tiempo bloqueado hasta: %s\n", tiempo_bloqueado);
    fprintf(file, "Intentos de apeada: %d\n", bcp->intentos_apeada);
    
    fclose(file);
}

// Guardar el estado de la tabla de procesos en un archivo
void guardar_estado_tabla_procesos() {
    FILE *file = fopen(ARCHIVO_TABLA_PROCESOS, "w");
    if (!file) {
        printf("Error: No se pudo abrir el archivo %s para escritura.\n", ARCHIVO_TABLA_PROCESOS);
        return;
    }
    
    // Obtener timestamp legible
    char tiempo_inicio[30];
    struct tm *tm_info = localtime(&tabla_procesos.tiempo_inicio_juego);
    strftime(tiempo_inicio, sizeof(tiempo_inicio), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Convertir algoritmo a string
    const char *algoritmo_str = (tabla_procesos.algoritmo == FCFS) ? "FCFS" : "Round Robin";
    
    fprintf(file, "=== TABLA DE PROCESOS ===\n");
    fprintf(file, "Número de procesos: %d\n", tabla_procesos.num_procesos);
    fprintf(file, "Proceso actual: %d\n", 
            (tabla_procesos.proceso_actual >= 0) ? 
            tabla_procesos.procesos[tabla_procesos.proceso_actual]->pid : -1);
    fprintf(file, "Algoritmo de planificación: %s\n", algoritmo_str);
    fprintf(file, "Quantum global: %d segundos\n", tabla_procesos.quantum_global);
    fprintf(file, "Último cambio de contexto: %ld\n", tabla_procesos.ultimo_cambio_contexto);
    fprintf(file, "Cambios de contexto: %d\n", tabla_procesos.cambios_contexto);
    fprintf(file, "Procesos bloqueados: %d\n", tabla_procesos.procesos_bloqueados);
    fprintf(file, "Procesos listos: %d\n", tabla_procesos.procesos_listos);
    fprintf(file, "Planificador activo: %s\n", tabla_procesos.planificador_activo ? "Sí" : "No");
    fprintf(file, "Tiempo de inicio del juego: %s\n", tiempo_inicio);
    fprintf(file, "Turnos completados: %d\n", tabla_procesos.turnos_completados);
    fprintf(file, "Tiempo máximo de ejecución: %d segundos\n", tabla_procesos.max_tiempo_ejecucion);
    fprintf(file, "\nProcesos activos:\n");
    fprintf(file, "PID\tJugador\tEstado\tTiempo CPU\tPuntos\tTurnos Perdidos\n");
    
    for (int i = 0; i < tabla_procesos.num_procesos; i++) {
        BCP *bcp = tabla_procesos.procesos[i];
        const char *estado_str;
        switch(bcp->estado) {
            case PROCESO_NUEVO: estado_str = "NUEVO"; break;
            case PROCESO_LISTO: estado_str = "LISTO"; break;
            case PROCESO_EJECUTANDO: estado_str = "EJECUTANDO"; break;
            case PROCESO_BLOQUEADO: estado_str = "BLOQUEADO"; break;
            case PROCESO_TERMINADO: estado_str = "TERMINADO"; break;
            default: estado_str = "DESCONOCIDO";
        }
        
        fprintf(file, "%d\t%d\t%s\t%d\t\t%d\t%d\n", 
                bcp->pid, bcp->jugador_id, estado_str, 
                bcp->tiempo_cpu, bcp->puntos_acumulados, bcp->turnos_perdidos);
    }
    
    fclose(file);
}

// Imprimir el estado actual de los procesos
void imprimir_estado_procesos() {
    pthread_mutex_lock(&tabla_procesos.mutex);
    
    printf("\n=== ESTADO DE PROCESOS ===\n");
    printf("Algoritmo: %s, Procesos: %d, Listos: %d, Bloqueados: %d\n", 
           (tabla_procesos.algoritmo == FCFS) ? "FCFS" : "Round Robin",
           tabla_procesos.num_procesos, tabla_procesos.procesos_listos, 
           tabla_procesos.procesos_bloqueados);
    
    printf("\nPID\tJugador\tEstado\tTiempo CPU\tPuntos\tTurnos Perdidos\n");
    printf("---\t-------\t------\t---------\t------\t--------------\n");
    
    for (int i = 0; i < tabla_procesos.num_procesos; i++) {
        BCP *bcp = tabla_procesos.procesos[i];
        const char *estado_str;
        switch(bcp->estado) {
            case PROCESO_NUEVO: estado_str = "NUEVO"; break;
            case PROCESO_LISTO: estado_str = "LISTO"; break;
            case PROCESO_EJECUTANDO: estado_str = "EJECUTANDO"; break;
            case PROCESO_BLOQUEADO: estado_str = "BLOQUEADO"; break;
            case PROCESO_TERMINADO: estado_str = "TERMINADO"; break;
            default: estado_str = "DESCONOCIDO";
        }
        
        // Destacar el proceso actual
        char indicador = (i == tabla_procesos.proceso_actual) ? '*' : ' ';
        
        printf("%c %d\t%d\t%s\t%d\t\t%d\t%d\n", 
               indicador, bcp->pid, bcp->jugador_id, estado_str, 
               bcp->tiempo_cpu, bcp->puntos_acumulados, bcp->turnos_perdidos);
    }
    
    printf("\n* = Proceso en ejecución\n\n");
    
    pthread_mutex_unlock(&tabla_procesos.mutex);
}

// Calcular un tiempo aleatorio para el bloqueo de E/S
int calcular_tiempo_aleatorio_es() {
    // Generar un valor aleatorio entre 3 y 10 segundos
    return 3 + (rand() % 8);
}

// Función para finalizar el sistema de procesos
void finalizar_sistema_procesos() {
    // Marcar los hilos como inactivos
    tabla_procesos.planificador_activo = false;
    cola_es.activo = false;
    
    // Esperar a que terminen los hilos
    pthread_join(tabla_procesos.hilo_planificador, NULL);
    pthread_join(cola_es.hilo_es, NULL);
    
    // Destruir los mutexes
    pthread_mutex_destroy(&tabla_procesos.mutex);
    pthread_mutex_destroy(&cola_es.mutex);
    
    // Liberar los BCPs
    for (int i = 0; i < tabla_procesos.num_procesos; i++) {
        free(tabla_procesos.procesos[i]);
    }
    
    printf("Sistema de procesos finalizado.\n");
}

// Función para obtener el jugador asociado a un proceso
int obtener_jugador_id(int pid) {
    BCP *bcp = obtener_bcp(pid);
    return bcp ? bcp->jugador_id : -1;
}

// Función para obtener el proceso asociado a un jugador
int obtener_pid_jugador(int jugador_id) {
    pthread_mutex_lock(&tabla_procesos.mutex);
    for (int i = 0; i < tabla_procesos.num_procesos; i++) {
        if (tabla_procesos.procesos[i]->jugador_id == jugador_id) {
            int pid = tabla_procesos.procesos[i]->pid;
            pthread_mutex_unlock(&tabla_procesos.mutex);
            return pid;
        }
    }
    pthread_mutex_unlock(&tabla_procesos.mutex);
    return -1;
}

// Función para verificar si es el turno de un jugador específico
bool es_turno_jugador(int jugador_id) {
    bool resultado = false;
    pthread_mutex_lock(&tabla_procesos.mutex);
    
    if (tabla_procesos.proceso_actual >= 0) {
        resultado = (tabla_procesos.procesos[tabla_procesos.proceso_actual]->jugador_id == jugador_id);
    }
    
    pthread_mutex_unlock(&tabla_procesos.mutex);
    return resultado;
}

// Función para marcar que un jugador hizo su primera apeada
void marcar_primera_apeada(int pid) {
    BCP *bcp = obtener_bcp(pid);
    if (bcp) {
        bcp->primera_apeada = true;
        bcp->intentos_apeada++;
        guardar_estado_bcp(pid);
    }
}

// Función para incrementar contador de fichas colocadas
void incrementar_fichas_colocadas(int pid, int cantidad) {
    BCP *bcp = obtener_bcp(pid);
    if (bcp) {
        bcp->fichas_colocadas += cantidad;
        guardar_estado_bcp(pid);
    }
}

// Función para incrementar contador de intentos de apeada
void incrementar_intentos_apeada(int pid) {
    BCP *bcp = obtener_bcp(pid);
    if (bcp) {
        bcp->intentos_apeada++;
        guardar_estado_bcp(pid);
    }
}

// Función para verificar si un proceso ha hecho su primera apeada
bool ha_hecho_primera_apeada(int pid) {
    BCP *bcp = obtener_bcp(pid);
    return bcp ? bcp->primera_apeada : false;
}

// Función para ajustar el quantum global (para Round Robin)
void ajustar_quantum_global(int nuevo_quantum) {
    if (nuevo_quantum < 1) {
        printf("Error: El quantum debe ser al menos 1 segundo.\n");
        return;
    }
    
    pthread_mutex_lock(&tabla_procesos.mutex);
    tabla_procesos.quantum_global = nuevo_quantum;
    
    // Actualizar el quantum de todos los procesos
    for (int i = 0; i < tabla_procesos.num_procesos; i++) {
        tabla_procesos.procesos[i]->quantum = nuevo_quantum;
    }
    
    pthread_mutex_unlock(&tabla_procesos.mutex);
    printf("Quantum global ajustado a %d segundos.\n", nuevo_quantum);
    guardar_estado_tabla_procesos();
}

// Función para manejar la entrada de teclado para cambiar el algoritmo
void* hilo_entrada_teclado(void *arg) {
    char tecla;
    while (1) {
        tecla = getchar();
        
        // Limpiar el buffer de entrada
        while (getchar() != '\n');
        
        switch (tecla) {
            case 'f':
            case 'F':
                cambiar_algoritmo(FCFS);
                break;
                
            case 'r':
            case 'R':
                cambiar_algoritmo(RR);
                break;
                
            case 'q':
            case 'Q':
                printf("Saliendo del monitoreo de teclado...\n");
                return NULL;
                
            case 'p':
            case 'P':
                imprimir_estado_procesos();
                break;
                
            default:
                printf("Tecla no reconocida. Opciones:\n");
                printf("F: Cambiar a FCFS\n");
                printf("R: Cambiar a Round Robin\n");
                printf("P: Imprimir estado de procesos\n");
                printf("Q: Salir del monitoreo de teclado\n");
        }
    }
    
    return NULL;
}

// Función para iniciar el monitoreo de teclado
void iniciar_monitoreo_teclado() {
    pthread_t hilo_teclado;
    printf("Iniciando monitoreo de teclado para cambiar algoritmos.\n");
    printf("Opciones:\n");
    printf("F: Cambiar a FCFS\n");
    printf("R: Cambiar a Round Robin\n");
    printf("P: Imprimir estado de procesos\n");
    printf("Q: Salir del monitoreo de teclado\n");
    
    pthread_create(&hilo_teclado, NULL, hilo_entrada_teclado, NULL);
}