#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para memset (aunque no se usa para mapaBits ahora)
#include <limits.h> // Para INT_MAX
#include <time.h>   // Para rand() en inicializarMemoria

#include "memoria.h"
#include "jugadores.h"  // Para MAX_JUGADORES (si no está en otro sitio global)
#include "utilidades.h" // Para colores y registrarEvento
#include "juego.h"      // Para MAX_JUGADORES (si está definido aquí)

// Variable global para el gestor de memoria
GestorMemoria gestorMemoria;

// Declaración de funciones auxiliares privadas (si las necesitas fuera de su primer uso)
void consolidarParticiones(void); // Ya la tienes

// Inicializar el sistema de memoria
void inicializarMemoria(void) {
    // Inicializar las particiones para ajuste óptimo
    gestorMemoria.particiones[0].inicio = 0;
    gestorMemoria.particiones[0].tamano = MEM_TOTAL_SIZE;
    gestorMemoria.particiones[0].idProceso = -1; // Libre
    gestorMemoria.particiones[0].libre = true;
    gestorMemoria.numParticiones = 1;

    // Inicializar Mapa de Bits
    for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
        gestorMemoria.mapaBits[i] = -1; // -1 indica bloque libre
    }
    gestorMemoria.tamanoBloque = TAMANO_BLOQUE_BITMAP; // Establecer tamaño del bloque

    // Configuración general de memoria para particionamiento
    gestorMemoria.memoriaDisponible = MEM_TOTAL_SIZE;
    gestorMemoria.algoritmoActual = ALG_AJUSTE_OPTIMO; // Por defecto

    // Inicializar la memoria virtual (LRU)
    inicializarMemoriaVirtual();

    // Seleccionar aleatoriamente dos procesos que pueden crecer
    if (MAX_JUGADORES > 0) { // Evitar división por cero o módulo con cero si MAX_JUGADORES no está definido o es 0
        gestorMemoria.creceProc1 = rand() % MAX_JUGADORES;
        do {
            gestorMemoria.creceProc2 = rand() % MAX_JUGADORES;
        } while (MAX_JUGADORES > 1 && gestorMemoria.creceProc2 == gestorMemoria.creceProc1);
         if (MAX_JUGADORES == 1) gestorMemoria.creceProc2 = -1; // No hay segundo proceso si solo hay 1 jugador
    } else {
        gestorMemoria.creceProc1 = -1;
        gestorMemoria.creceProc2 = -1;
    }


    colorVerde();
    printf("Sistema de memoria inicializado: %d bytes disponibles.\n", MEM_TOTAL_SIZE);
    printf("Algoritmo de particionamiento por defecto: Ajuste Óptimo.\n");
    if (gestorMemoria.creceProc1 != -1) {
        printf("Proceso que puede crecer 1: ID %d.\n", gestorMemoria.creceProc1);
    }
    if (gestorMemoria.creceProc2 != -1) {
        printf("Proceso que puede crecer 2: ID %d.\n", gestorMemoria.creceProc2);
    }
    colorReset();

    registrarEvento("Sistema de memoria inicializado: %d bytes disponibles. Algoritmo particionamiento: Ajuste Óptimo.", MEM_TOTAL_SIZE);
    if (gestorMemoria.creceProc1 != -1) {
      registrarEvento("Proceso autorizado para crecer 1: ID %d.", gestorMemoria.creceProc1);
    }
     if (gestorMemoria.creceProc2 != -1) {
      registrarEvento("Proceso autorizado para crecer 2: ID %d.", gestorMemoria.creceProc2);
    }
}

// Asignar memoria cuando un proceso entra en E/S (tu versión, parece bien)
bool asignarMemoriaES(int idProceso) {
    int memoriaBase = 16;
    int memoriaPorCartaSimulada = 4;
    int numCartasSimuladas = 5 + (rand() % 8);
    int cantidadRequerida = memoriaBase + (numCartasSimuladas * memoriaPorCartaSimulada);

    colorCian();
    printf("Proceso %d entra a E/S, solicita %d bytes de memoria.\n", idProceso, cantidadRequerida);
    colorReset();
    registrarEvento("Proceso %d entra a E/S, solicita %d bytes.", idProceso, cantidadRequerida);

    bool asignado = asignarMemoria(idProceso, cantidadRequerida);

    if (asignado && (idProceso == gestorMemoria.creceProc1 || idProceso == gestorMemoria.creceProc2)) {
        int crecimiento = (rand() % 21) + 10; // Crecimiento entre 10 y 30 bytes
        colorMagenta();
        printf("Proceso %d (autorizado) intentará crecer en %d bytes adicionales.\n", idProceso, crecimiento);
        colorReset();
        registrarEvento("Proceso %d (autorizado) intenta crecer %d bytes.", idProceso, crecimiento);
        crecerProceso(idProceso, crecimiento); // crecerProceso ya imprime y registra
    }
    return asignado;
}

// Liberar memoria (tu versión, parece bien)
void liberarMemoria(int idProceso) {
    bool liberada = false;
    int bytesLiberadosTotal = 0; // Para registrar el total si un proceso tiene múltiples bloques/particiones

    if (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) {
        for (int i = 0; i < gestorMemoria.numParticiones; i++) {
            if (gestorMemoria.particiones[i].idProceso == idProceso) {
                int tamanoLiberado = gestorMemoria.particiones[i].tamano;
                gestorMemoria.particiones[i].libre = true;
                gestorMemoria.particiones[i].idProceso = -1;
                gestorMemoria.memoriaDisponible += tamanoLiberado;
                bytesLiberadosTotal += tamanoLiberado;
                liberada = true;

                colorAmarillo();
                printf("Memoria liberada (Ajuste Óptimo): Proceso %d, partición de %d bytes en inicio %d.\n", idProceso, tamanoLiberado, gestorMemoria.particiones[i].inicio);
                colorReset();
                
                consolidarParticiones();
                i = -1; // Re-evaluar desde el principio debido a la consolidación
            }
        }
    } else if (gestorMemoria.algoritmoActual == ALG_MAPA_BITS) {
        int bloquesLiberados = 0;
        for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
            if (gestorMemoria.mapaBits[i] == idProceso) {
                gestorMemoria.mapaBits[i] = -1; // Marcar como libre
                gestorMemoria.memoriaDisponible += gestorMemoria.tamanoBloque;
                bloquesLiberados++;
                liberada = true;
            }
        }
        if (liberada) {
            bytesLiberadosTotal = bloquesLiberados * gestorMemoria.tamanoBloque;
            colorAmarillo();
            printf("Memoria liberada (Mapa de Bits): Proceso %d, %d bloques (%d bytes).\n",
                   idProceso, bloquesLiberados, bytesLiberadosTotal);
            colorReset();
        }
    } else if (gestorMemoria.algoritmoActual == ALG_LRU) {
        // Este caso es ambiguo. ALG_LRU es para memoria virtual.
        // La memoria física de particiones no debería estar en modo "LRU".
        // Quizás esta función solo deba operar sobre particiones.
        // Si liberarMemoria también debe liberar páginas virtuales, eso ya se hace abajo.
        colorRojo();
        printf("Advertencia: liberarMemoria llamada con algoritmoActual=ALG_LRU. Operación no definida para particiones.\n");
        colorReset();
    } else {
        printf("Error: Algoritmo de liberación de memoria desconocido (%d).\n", gestorMemoria.algoritmoActual);
        return;
    }

    if (liberada) {
        registrarEvento("Memoria física liberada: Proceso %d, %d bytes (Algoritmo: %s).",
                        idProceso, bytesLiberadosTotal,
                        (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" :
                        (gestorMemoria.algoritmoActual == ALG_MAPA_BITS ? "Mapa de Bits" : "LRU/Desconocido")));
        imprimirEstadoMemoria();
    } else {
        // printf("No se encontró memoria física asignada al proceso %d para liberar con algoritmo %d.\n", idProceso, gestorMemoria.algoritmoActual);
    }

    // Liberar páginas virtuales del proceso (esto es independiente del algoritmo de particionamiento)
    liberarPaginasProceso(idProceso); // Esta función ya registra su propia acción.
}

// Consolidar particiones (tu versión, parece bien)
void consolidarParticiones(void) {
    int i = 0;
    bool consolidado;
    do {
        consolidado = false;
        i = 0;
        while (i < gestorMemoria.numParticiones - 1) {
            if (gestorMemoria.particiones[i].libre && gestorMemoria.particiones[i + 1].libre) {
                colorMagenta();
                printf("Consolidando particiones libres: [%d, %d bytes] con [%d, %d bytes]\n",
                       gestorMemoria.particiones[i].inicio, gestorMemoria.particiones[i].tamano,
                       gestorMemoria.particiones[i+1].inicio, gestorMemoria.particiones[i+1].tamano);
                colorReset();
                
                gestorMemoria.particiones[i].tamano += gestorMemoria.particiones[i + 1].tamano;
                for (int j = i + 1; j < gestorMemoria.numParticiones - 1; j++) {
                    gestorMemoria.particiones[j] = gestorMemoria.particiones[j + 1];
                }
                gestorMemoria.numParticiones--;
                consolidado = true; 
                break; // Romper para reiniciar el while externo y re-escanear
            }
            i++;
        }
    } while (consolidado && gestorMemoria.numParticiones > 1);
}

// Asignar memoria (tu versión, parece bien)
bool asignarMemoria(int idProceso, int cantidadRequerida) {
    if (cantidadRequerida <= 0) {
        colorRojo();
        printf("Error (asignarMemoria): La cantidad de memoria solicitada debe ser mayor que cero.\n");
        colorReset();
        return false;
    }

    // Verificar si el algoritmo actual es para particionamiento
    if (gestorMemoria.algoritmoActual != ALG_AJUSTE_OPTIMO && gestorMemoria.algoritmoActual != ALG_MAPA_BITS) {
        colorRojo();
        printf("Error (asignarMemoria): Algoritmo %d no es para particionamiento. No se puede asignar memoria física.\n", gestorMemoria.algoritmoActual);
        colorReset();
        // Si el modo es LRU (virtual), la asignación de memoria física no se hace con esta función directamente.
        // Se podría considerar un error o simplemente no hacer nada.
        return false;
    }
    
    if (cantidadRequerida > gestorMemoria.memoriaDisponible) {
        colorRojo();
        printf("Error (asignarMemoria): No hay suficiente memoria física disponible (%d solicitados, %d disponibles).\n",
               cantidadRequerida, gestorMemoria.memoriaDisponible);
        colorReset();
        registrarEvento("Fallo asignación memoria física: Proceso %d solicitó %d, disponible %d.", idProceso, cantidadRequerida, gestorMemoria.memoriaDisponible);
        return false;
    }

    bool asignado = false;
    int direccionAsignada = -1; // Para el log

    if (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) {
        int indiceOptimo = -1;
        int tamanoFragmentoMinimo = INT_MAX;

        for (int i = 0; i < gestorMemoria.numParticiones; i++) {
            if (gestorMemoria.particiones[i].libre && gestorMemoria.particiones[i].tamano >= cantidadRequerida) {
                if (gestorMemoria.particiones[i].tamano - cantidadRequerida < tamanoFragmentoMinimo) {
                    tamanoFragmentoMinimo = gestorMemoria.particiones[i].tamano - cantidadRequerida;
                    indiceOptimo = i;
                } else if (gestorMemoria.particiones[i].tamano - cantidadRequerida == tamanoFragmentoMinimo) {
                    // Si hay empate en fragmentación, se podría elegir la de menor dirección (ya implícito en el recorrido)
                    // o la más pequeña que aún quepa (ya implícito con < tamanoFragmentoMinimo)
                }
            }
        }

        if (indiceOptimo != -1) {
            Particion *particionSeleccionada = &gestorMemoria.particiones[indiceOptimo];
            direccionAsignada = particionSeleccionada->inicio;

            if (particionSeleccionada->tamano == cantidadRequerida) {
                particionSeleccionada->libre = false;
                particionSeleccionada->idProceso = idProceso;
            } else { // Dividir partición
                if (gestorMemoria.numParticiones >= MAX_PARTICIONES) {
                    colorRojo();
                    printf("Error (Ajuste Óptimo): Máximo de particiones alcanzado. No se puede dividir.\n");
                    colorReset();
                    return false;
                }
                // Mover particiones para hacer espacio para la nueva (fragmento restante)
                for (int j = gestorMemoria.numParticiones; j > indiceOptimo + 1; j--) {
                    gestorMemoria.particiones[j] = gestorMemoria.particiones[j - 1];
                }
                // Configurar el nuevo fragmento libre
                gestorMemoria.particiones[indiceOptimo + 1].inicio = particionSeleccionada->inicio + cantidadRequerida;
                gestorMemoria.particiones[indiceOptimo + 1].tamano = particionSeleccionada->tamano - cantidadRequerida;
                gestorMemoria.particiones[indiceOptimo + 1].idProceso = -1;
                gestorMemoria.particiones[indiceOptimo + 1].libre = true;

                // Ajustar la partición asignada
                particionSeleccionada->tamano = cantidadRequerida;
                particionSeleccionada->libre = false;
                particionSeleccionada->idProceso = idProceso;
                gestorMemoria.numParticiones++;
            }
            gestorMemoria.memoriaDisponible -= cantidadRequerida;
            asignado = true;
        } else {
            colorRojo();
            printf("Error (Ajuste Óptimo): No se encontró partición adecuada para %d bytes.\n", cantidadRequerida);
            colorReset();
        }

    } else if (gestorMemoria.algoritmoActual == ALG_MAPA_BITS) {
        int numBloquesRequeridos = (cantidadRequerida + gestorMemoria.tamanoBloque - 1) / gestorMemoria.tamanoBloque;
        int bloquesLibresConsecutivos = 0;
        int inicioBloqueEncontrado = -1;

        for (int i = 0; i <= NUM_BLOQUES_BITMAP - numBloquesRequeridos; i++) { // Optimizar búsqueda
            bloquesLibresConsecutivos = 0;
            for (int j = 0; j < numBloquesRequeridos; j++) {
                if (gestorMemoria.mapaBits[i + j] == -1) { // -1 es libre
                    bloquesLibresConsecutivos++;
                } else {
                    break; // Bloque ocupado, no es contiguo
                }
            }
            if (bloquesLibresConsecutivos == numBloquesRequeridos) {
                inicioBloqueEncontrado = i;
                break;
            }
        }

        if (inicioBloqueEncontrado != -1) {
            for (int i = 0; i < numBloquesRequeridos; i++) {
                gestorMemoria.mapaBits[inicioBloqueEncontrado + i] = idProceso;
            }
            gestorMemoria.memoriaDisponible -= numBloquesRequeridos * gestorMemoria.tamanoBloque;
            direccionAsignada = inicioBloqueEncontrado * gestorMemoria.tamanoBloque;
            asignado = true;
        } else {
            colorRojo();
            printf("Error (Mapa de Bits): No se encontró espacio contiguo para %d bloques (%d bytes).\n", numBloquesRequeridos, cantidadRequerida);
            colorReset();
        }
    }

    if (asignado) {
        colorVerde();
        printf("Memoria física asignada: Proceso %d, %d bytes en dirección %d (Algoritmo: %s).\n",
               idProceso, cantidadRequerida, direccionAsignada,
               (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" : "Mapa de Bits"));
        colorReset();
        registrarEvento("Memoria física asignada: Proceso %d, %d bytes, dir %d (Algoritmo: %s).",
                        idProceso, cantidadRequerida, direccionAsignada,
                        (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" : "Mapa de Bits"));
        imprimirEstadoMemoria();
    }
    return asignado;
}

// Permitir que un proceso crezca (con lógica para Mapa de Bits añadida)
bool crecerProceso(int idProceso, int cantidadAdicional) {
    if (cantidadAdicional <= 0) {
        colorRojo();
        printf("Error (crecerProceso): La cantidad adicional debe ser > 0.\n");
        colorReset();
        registrarEvento("Error crecimiento: Proceso %d, cant adicional %d inválida.", idProceso, cantidadAdicional);
        return false;
    }

    if (idProceso != gestorMemoria.creceProc1 && idProceso != gestorMemoria.creceProc2) {
        colorRojo();
        printf("Error (crecerProceso): Proceso %d no autorizado para crecer.\n", idProceso);
        colorReset();
        registrarEvento("Error crecimiento: Proceso %d no autorizado intentó crecer.", idProceso);
        return false;
    }
     // Verificar si el algoritmo actual es para particionamiento
    if (gestorMemoria.algoritmoActual != ALG_AJUSTE_OPTIMO && gestorMemoria.algoritmoActual != ALG_MAPA_BITS) {
        colorRojo();
        printf("Error (crecerProceso): Algoritmo %d no es para particionamiento. No se puede crecer memoria física.\n", gestorMemoria.algoritmoActual);
        colorReset();
        return false;
    }

    if (cantidadAdicional > gestorMemoria.memoriaDisponible) {
        colorRojo();
        printf("Error (crecerProceso): Memoria insuficiente para crecimiento (%d solicitados, %d disponibles).\n",
               cantidadAdicional, gestorMemoria.memoriaDisponible);
        colorReset();
        registrarEvento("Error crecimiento: Memoria insuficiente para Proceso %d (solicitó %d, disp %d).", idProceso, cantidadAdicional, gestorMemoria.memoriaDisponible);
        return false;
    }

    bool crecido = false;

    if (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) {
        int indiceParticionProceso = -1;
        int tamanoTotalProceso = 0;
        // Encontrar una partición del proceso. Para simplificar, intentamos expandir la última encontrada.
        // Una lógica más compleja podría intentar consolidar particiones del mismo proceso si están fragmentadas.
        for (int i = gestorMemoria.numParticiones - 1; i >= 0; i--) {
            if (gestorMemoria.particiones[i].idProceso == idProceso) {
                indiceParticionProceso = i;
                tamanoTotalProceso += gestorMemoria.particiones[i].tamano; // Acumular por si tiene varias
                // Intentar expandir esta partición si hay una libre adyacente después
                if (i + 1 < gestorMemoria.numParticiones &&
                    gestorMemoria.particiones[i + 1].libre &&
                    gestorMemoria.particiones[i + 1].tamano >= cantidadAdicional) {
                    
                    Particion *particionProcesoActual = &gestorMemoria.particiones[i];
                    Particion *particionLibreAdyacente = &gestorMemoria.particiones[i + 1];

                    particionProcesoActual->tamano += cantidadAdicional;
                    particionLibreAdyacente->inicio += cantidadAdicional;
                    particionLibreAdyacente->tamano -= cantidadAdicional;
                    gestorMemoria.memoriaDisponible -= cantidadAdicional;
                    crecido = true;

                    colorVerde();
                    printf("Proceso %d (Ajuste Óptimo) creció en %d bytes (expandiendo partición %d de %d a %d).\n",
                           idProceso, cantidadAdicional, i, particionProcesoActual->tamano - cantidadAdicional, particionProcesoActual->tamano);
                    colorReset();

                    if (particionLibreAdyacente->tamano == 0) { // Consumida
                        for (int k = i + 1; k < gestorMemoria.numParticiones - 1; k++) {
                            gestorMemoria.particiones[k] = gestorMemoria.particiones[k + 1];
                        }
                        gestorMemoria.numParticiones--;
                    }
                    break; // Crecimiento exitoso
                }
            }
        }
         // Si no se pudo expandir una partición existente adyacente,
        // intentar asignar una nueva partición para la cantidad adicional.
        if (!crecido && indiceParticionProceso != -1) { // Solo si el proceso ya tenía memoria
            colorAmarillo();
            printf("Proceso %d (Ajuste Óptimo) no pudo expandir partición existente, intentando nueva asignación para %d bytes.\n", idProceso, cantidadAdicional);
            colorReset();
            if (asignarMemoria(idProceso, cantidadAdicional)) {
                crecido = true; // asignarMemoria ya imprime y registra
            }
        } else if (!crecido && indiceParticionProceso == -1){
            colorRojo();
            printf("Error (Ajuste Óptimo): Proceso %d no tiene memoria asignada para crecer.\n", idProceso);
            colorReset();
        }


    } else if (gestorMemoria.algoritmoActual == ALG_MAPA_BITS) {
        int bloquesAdicionalesRequeridos = (cantidadAdicional + gestorMemoria.tamanoBloque - 1) / gestorMemoria.tamanoBloque;
        int ultimoBloqueDelProceso = -1;
        int primerBloqueDelProceso = -1;
        int numBloquesActuales = 0;

        for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
            if (gestorMemoria.mapaBits[i] == idProceso) {
                if (primerBloqueDelProceso == -1) primerBloqueDelProceso = i;
                ultimoBloqueDelProceso = i;
                numBloquesActuales++;
            }
        }

        if (numBloquesActuales == 0) {
            colorRojo();
            printf("Error (Mapa de Bits): Proceso %d no tiene memoria asignada para crecer.\n", idProceso);
            colorReset();
            return false; // No tiene nada que crecer
        }

        // Intentar expandir contiguamente después del último bloque
        bool expandidoContiguo = true;
        if (ultimoBloqueDelProceso + bloquesAdicionalesRequeridos < NUM_BLOQUES_BITMAP) {
            for (int i = 1; i <= bloquesAdicionalesRequeridos; i++) {
                if (gestorMemoria.mapaBits[ultimoBloqueDelProceso + i] != -1) { // Si no está libre
                    expandidoContiguo = false;
                    break;
                }
            }
            if (expandidoContiguo) {
                for (int i = 1; i <= bloquesAdicionalesRequeridos; i++) {
                    gestorMemoria.mapaBits[ultimoBloqueDelProceso + i] = idProceso;
                }
                gestorMemoria.memoriaDisponible -= bloquesAdicionalesRequeridos * gestorMemoria.tamanoBloque;
                crecido = true;
                colorVerde();
                printf("Proceso %d (Mapa de Bits) creció en %d bloques contiguos al final. Total bloques: %d.\n",
                       idProceso, bloquesAdicionalesRequeridos, numBloquesActuales + bloquesAdicionalesRequeridos);
                colorReset();
            }
        } else {
            expandidoContiguo = false; // No hay espacio suficiente al final del mapa
        }
        
        // Si no se pudo expandir contiguo al final, y si el proyecto lo permitiera,
        // se podría intentar buscar un nuevo bloque grande y mover, o asignar bloques no contiguos.
        // Para este proyecto, si no es contiguo al final, fallamos el crecimiento para Mapa de Bits.
        if (!crecido) {
             colorAmarillo();
             printf("Proceso %d (Mapa de Bits) no pudo crecer %d bloques de forma contigua adyacente. Crecimiento fallido.\n", idProceso, bloquesAdicionalesRequeridos);
             colorReset();
        }
    }

    if (crecido) {
        registrarEvento("Proceso %d creció en %d bytes (Algoritmo: %s). Memoria disponible: %d.",
                        idProceso, cantidadAdicional,
                        (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" : "Mapa de Bits"),
                        gestorMemoria.memoriaDisponible);
        imprimirEstadoMemoria();
    } else {
        // El mensaje de error específico ya se imprimió dentro de la lógica del algoritmo
        registrarEvento("Fallo crecimiento: Proceso %d no pudo crecer %d bytes adicionales (Algoritmo: %s).", idProceso, cantidadAdicional,
                        (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" : "Mapa de Bits"));
    }
    return crecido;
}


// Imprimir el estado actual de la memoria (particiones)
void imprimirEstadoMemoria(void) {
    printf("\n--- ESTADO DE MEMORIA DE PARTICIONES ---\n");
    const char *nombreAlg;
    if(gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) nombreAlg = "Ajuste Óptimo";
    else if(gestorMemoria.algoritmoActual == ALG_MAPA_BITS) nombreAlg = "Mapa de Bits";
    else if(gestorMemoria.algoritmoActual == ALG_LRU) nombreAlg = "LRU (Modo Memoria Virtual Activo)";
    else nombreAlg = "Desconocido";

    printf("Algoritmo de Particionamiento Actual: %s\n", nombreAlg);
    printf("Memoria Total Física: %d bytes\n", MEM_TOTAL_SIZE);
    printf("Memoria Física Disponible: %d bytes\n", gestorMemoria.memoriaDisponible);

    if (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) {
        printf("Número de Particiones: %d\n", gestorMemoria.numParticiones);
        printf("-----------------------------------------------------\n");
        printf("| %-6s | %-10s | %-10s | %-8s | %-7s |\n", "ID", "Inicio", "Tamaño", "ID Proc.", "Estado");
        printf("-----------------------------------------------------\n");
        for (int i = 0; i < gestorMemoria.numParticiones; i++) {
            printf("| %-6d | %-10d | %-10d | %-8d | %-7s |\n",
                   i,
                   gestorMemoria.particiones[i].inicio,
                   gestorMemoria.particiones[i].tamano,
                   gestorMemoria.particiones[i].idProceso,
                   gestorMemoria.particiones[i].libre ? "Libre" : "Ocupado");
        }
        printf("-----------------------------------------------------\n");
    } else if (gestorMemoria.algoritmoActual == ALG_MAPA_BITS) {
        printf("Tamaño del Bloque (Mapa de Bits): %d bytes\n", gestorMemoria.tamanoBloque);
        printf("Número Total de Bloques: %d\n", NUM_BLOQUES_BITMAP);
        printf("Mapa de Bits (Leyenda: -1 = Libre, ID_Proceso = Ocupado por Proceso ID):\n");
        for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
            printf("%3d ", gestorMemoria.mapaBits[i]);
            if ((i + 1) % 16 == 0 || i == NUM_BLOQUES_BITMAP - 1) { // Nueva línea cada 16 bloques o al final
                printf("\n");
            }
        }
    }
    // Procesos autorizados a crecer (siempre se muestra)
    if (gestorMemoria.creceProc1 != -1) printf("Proceso autorizado para crecer 1: ID %d\n", gestorMemoria.creceProc1);
    if (gestorMemoria.creceProc2 != -1) printf("Proceso autorizado para crecer 2: ID %d\n", gestorMemoria.creceProc2);
    printf("---------------------------------------\n\n");
}

// --- Funciones para memoria virtual con LRU (tu versión, parece bien) ---

void inicializarMemoriaVirtual(void) {
    for (int i = 0; i < MAX_PAGINAS; i++) {
        gestorMemoria.tablaPaginas[i].idProceso = -1;
        gestorMemoria.tablaPaginas[i].numPagina = -1;
        gestorMemoria.tablaPaginas[i].idCarta = -1;
        gestorMemoria.tablaPaginas[i].tiempoUltimoUso = 0;
        gestorMemoria.tablaPaginas[i].bitReferencia = false; // Podrías no necesitarla para LRU puro
        gestorMemoria.tablaPaginas[i].bitModificacion = false;
        gestorMemoria.tablaPaginas[i].enMemoria = false;
        gestorMemoria.tablaPaginas[i].marcoAsignado = -1;
    }
    gestorMemoria.numPaginas = 0;

    for (int i = 0; i < NUM_MARCOS; i++) {
        gestorMemoria.marcosMemoria[i].idProceso = -1;
        gestorMemoria.marcosMemoria[i].numPagina = -1;
        gestorMemoria.marcosMemoria[i].libre = true;
    }
    gestorMemoria.contadorTiempo = 0;
    gestorMemoria.fallosPagina = 0;
    gestorMemoria.aciertosMemoria = 0;

    colorVerde();
    printf("Memoria virtual (LRU) inicializada: %d marcos. Cada marco puede alojar %d página(s)/ficha(s).\n",
           NUM_MARCOS, PAGINAS_POR_MARCO); // PAGINAS_POR_MARCO es 4, pero tu Marco struct solo tiene 1 numPagina.
                                         // Esto es una inconsistencia a revisar en tu diseño si un marco debe tener múltiples páginas.
                                         // Si un marco aloja 1 página, PAGINAS_POR_MARCO debería ser 1.
    colorReset();
    registrarEvento("Memoria virtual (LRU) inicializada. Marcos: %d.", NUM_MARCOS);
}

int accederPagina(int idProceso, int numPaginaLogica, int idCarta) {
    gestorMemoria.contadorTiempo++;
    Pagina *paginaAccedida = NULL;
    int indicePaginaEnTabla = -1;

    // Buscar si la página ya existe en la tabla de páginas
    for (int i = 0; i < gestorMemoria.numPaginas; i++) {
        if (gestorMemoria.tablaPaginas[i].idProceso == idProceso && gestorMemoria.tablaPaginas[i].numPagina == numPaginaLogica) {
            paginaAccedida = &gestorMemoria.tablaPaginas[i];
            indicePaginaEnTabla = i;
            break;
        }
    }

    // Si la página no existe, crear una entrada en la tabla de páginas
    if (paginaAccedida == NULL) {
        if (gestorMemoria.numPaginas >= MAX_PAGINAS) {
            colorRojo();
            printf("Error (accederPagina): Tabla de páginas llena. No se puede crear página para Proceso %d, Página %d.\n", idProceso, numPaginaLogica);
            colorReset();
            return -1; // Error
        }
        indicePaginaEnTabla = gestorMemoria.numPaginas;
        paginaAccedida = &gestorMemoria.tablaPaginas[indicePaginaEnTabla];
        
        paginaAccedida->idProceso = idProceso;
        paginaAccedida->numPagina = numPaginaLogica;
        paginaAccedida->idCarta = idCarta; // Asociar la carta/ficha
        paginaAccedida->enMemoria = false;
        paginaAccedida->marcoAsignado = -1;
        paginaAccedida->bitModificacion = false; 
        // Otras inicializaciones ya hechas en inicializarMemoriaVirtual, pero por si acaso.
        
        gestorMemoria.numPaginas++;
    }

    // Actualizar información de la página accedida
    paginaAccedida->tiempoUltimoUso = gestorMemoria.contadorTiempo;
    paginaAccedida->bitReferencia = true; // Para algoritmos que lo usan (LRU no directamente, pero es buena práctica)

    if (!paginaAccedida->enMemoria) { // Fallo de página
        gestorMemoria.fallosPagina++;
        colorAmarillo();
        printf("FALLO DE PÁGINA: Proceso %d, Página Lógica %d (Carta ID %d).\n", idProceso, numPaginaLogica, idCarta);
        colorReset();
        registrarEvento("Fallo de página: Proceso %d, Pág.Lógica %d (Carta %d).", idProceso, numPaginaLogica, idCarta);

        int marcoParaAsignar = -1;
        // Buscar un marco libre
        for (int i = 0; i < NUM_MARCOS; i++) {
            if (gestorMemoria.marcosMemoria[i].libre) {
                marcoParaAsignar = i;
                break;
            }
        }

        if (marcoParaAsignar == -1) { // No hay marcos libres, se necesita reemplazo (LRU)
            marcoParaAsignar = seleccionarVictimaLRU();
            if (marcoParaAsignar == -1) { // No debería pasar si hay marcos y páginas cargadas.
                 colorRojo();
                 printf("Error CRÍTICO (accederPagina): No se pudo seleccionar víctima LRU y no hay marcos libres.\n");
                 colorReset();
                 return -1; // Error grave
            }

            // Desalojar la página víctima del marco
            for (int i = 0; i < gestorMemoria.numPaginas; i++) { // Encontrar la página que ocupaba ese marco
                if (gestorMemoria.tablaPaginas[i].marcoAsignado == marcoParaAsignar && gestorMemoria.tablaPaginas[i].enMemoria) {
                    Pagina *paginaVictima = &gestorMemoria.tablaPaginas[i];
                    paginaVictima->enMemoria = false;
                    paginaVictima->marcoAsignado = -1;
                    // Si paginaVictima->bitModificacion es true, aquí iría la "escritura a disco" simulada.
                    colorMagenta();
                    printf("REEMPLAZO LRU: Marco %d desalojado. Contenía Pág.Lógica %d del Proc %d.\n",
                           marcoParaAsignar, paginaVictima->numPagina, paginaVictima->idProceso);
                    colorReset();
                    registrarEvento("Reemplazo LRU: Marco %d (Pág.Lógica %d, Proc %d) desalojada.",
                                   marcoParaAsignar, paginaVictima->numPagina, paginaVictima->idProceso);
                    break; 
                }
            }
        }

        // Cargar la nueva página en el marco seleccionado (libre o víctima)
        paginaAccedida->enMemoria = true;
        paginaAccedida->marcoAsignado = marcoParaAsignar;

        gestorMemoria.marcosMemoria[marcoParaAsignar].idProceso = idProceso;
        gestorMemoria.marcosMemoria[marcoParaAsignar].numPagina = numPaginaLogica; // Página lógica
        gestorMemoria.marcosMemoria[marcoParaAsignar].libre = false;

        colorVerde();
        printf("Página cargada: Proceso %d, Pág.Lógica %d (Carta %d) -> Marco Físico %d.\n",
               idProceso, numPaginaLogica, idCarta, marcoParaAsignar);
        colorReset();
        registrarEvento("Página cargada: Proc %d, Pág.Lógica %d (Carta %d) -> Marco %d.",
                       idProceso, numPaginaLogica, idCarta, marcoParaAsignar);
        imprimirEstadoMemoriaVirtual(); // Mostrar estado después de cargar

    } else { // Acierto de página
        gestorMemoria.aciertosMemoria++;
        colorVerde();
        printf("ACIERTO DE MEMORIA: Proceso %d, Página Lógica %d (Carta ID %d) ya en Marco %d.\n",
               idProceso, numPaginaLogica, idCarta, paginaAccedida->marcoAsignado);
        colorReset();
        // No es necesario registrar evento para cada acierto, podría ser muy verboso.
    }
    return paginaAccedida->marcoAsignado;
}

int seleccionarVictimaLRU(void) {
    int marcoVictima = -1;
    int tiempoMasAntiguo = gestorMemoria.contadorTiempo + 1; // Inicializar a un valor futuro

    for (int i = 0; i < NUM_MARCOS; i++) { // Iterar sobre los marcos físicos
        if (!gestorMemoria.marcosMemoria[i].libre) { // Solo considerar marcos ocupados
            // Encontrar la página que está en este marco
            for (int j = 0; j < gestorMemoria.numPaginas; j++) {
                if (gestorMemoria.tablaPaginas[j].enMemoria && gestorMemoria.tablaPaginas[j].marcoAsignado == i) {
                    if (gestorMemoria.tablaPaginas[j].tiempoUltimoUso < tiempoMasAntiguo) {
                        tiempoMasAntiguo = gestorMemoria.tablaPaginas[j].tiempoUltimoUso;
                        marcoVictima = i;
                    }
                    break; // Encontrada la página para este marco
                }
            }
        }
    }
    return marcoVictima;
}

void liberarPaginasProceso(int idProceso) {
    int paginasLiberadas = 0;
    for (int i = 0; i < gestorMemoria.numPaginas; i++) {
        if (gestorMemoria.tablaPaginas[i].idProceso == idProceso) {
            if (gestorMemoria.tablaPaginas[i].enMemoria) {
                int marco = gestorMemoria.tablaPaginas[i].marcoAsignado;
                if (marco >= 0 && marco < NUM_MARCOS) { // Chequeo de seguridad
                    gestorMemoria.marcosMemoria[marco].idProceso = -1;
                    gestorMemoria.marcosMemoria[marco].numPagina = -1;
                    gestorMemoria.marcosMemoria[marco].libre = true;
                }
            }
            // Marcar la página como no utilizada (se podría reusar la entrada en tablaPaginas o compactar la tabla)
            // Por simplicidad, solo se marcan como inválidas para este proceso.
            gestorMemoria.tablaPaginas[i].idProceso = -2; // -2 para distinguirla de una nunca usada (-1)
            gestorMemoria.tablaPaginas[i].enMemoria = false;
            gestorMemoria.tablaPaginas[i].marcoAsignado = -1;
            paginasLiberadas++;
        }
    }
    if (paginasLiberadas > 0) {
        colorAmarillo();
        printf("Memoria virtual: %d páginas del Proceso %d liberadas de la tabla de páginas y marcos.\n", paginasLiberadas, idProceso);
        colorReset();
        registrarEvento("Memoria virtual: %d páginas del Proceso %d liberadas.", paginasLiberadas, idProceso);
        imprimirEstadoMemoriaVirtual();
    }
}

void imprimirEstadoMemoriaVirtual(void) {
    printf("\n--- ESTADO DE MEMORIA VIRTUAL (LRU) ---\n");
    printf("Marcos Físicos Totales: %d\n", NUM_MARCOS);
    // La constante PAGINAS_POR_MARCO puede ser confusa si cada marco solo tiene 1 página.
    // printf("Páginas Lógicas por Marco (Diseño): %d\n", PAGINAS_POR_MARCO);
    printf("Contador de Tiempo LRU: %d\n", gestorMemoria.contadorTiempo);
    printf("Fallos de Página: %d\n", gestorMemoria.fallosPagina);
    printf("Aciertos de Memoria: %d\n", gestorMemoria.aciertosMemoria);

    if (gestorMemoria.fallosPagina + gestorMemoria.aciertosMemoria > 0) {
        float tasaAciertos = (float)gestorMemoria.aciertosMemoria /
                             (gestorMemoria.fallosPagina + gestorMemoria.aciertosMemoria) * 100.0f;
        printf("Tasa de Aciertos: %.2f%%\n", tasaAciertos);
    }

    printf("\nEstado de los Marcos Físicos en Memoria Principal:\n");
    printf("---------------------------------------------------\n");
    printf("| %-8s | %-10s | %-12s | %-7s |\n", "Marco ID", "ID Proceso", "Pág. Lógica", "Estado");
    printf("---------------------------------------------------\n");
    for (int i = 0; i < NUM_MARCOS; i++) {
        printf("| %-8d | %-10d | %-12d | %-7s |\n",
               i,
               gestorMemoria.marcosMemoria[i].libre ? -1 : gestorMemoria.marcosMemoria[i].idProceso,
               gestorMemoria.marcosMemoria[i].libre ? -1 : gestorMemoria.marcosMemoria[i].numPagina,
               gestorMemoria.marcosMemoria[i].libre ? "Libre" : "Ocupado");
    }
    printf("---------------------------------------------------\n");

    printf("\nTabla de Páginas (primeras %d entradas activas o hasta 10 mostradas):\n", MAX_PAGINAS);
    printf("------------------------------------------------------------------------------------------------------\n");
    printf("| %-6s | %-10s | %-12s | %-10s | %-8s | %-12s | %-10s |\n",
           "Índice", "ID Proceso", "Pág. Lógica", "ID Carta", "En Mem?", "Marco Físico", "Último Uso");
    printf("------------------------------------------------------------------------------------------------------\n");
    int mostradas = 0;
    for (int i = 0; i < gestorMemoria.numPaginas && mostradas < 10; i++) {
        if (gestorMemoria.tablaPaginas[i].idProceso >= -1) { // Mostrar activas y nunca usadas (-1) pero no las "liberadas" (-2)
             if(gestorMemoria.tablaPaginas[i].idProceso == -2) continue; // Saltar las explícitamente liberadas
            printf("| %-6d | %-10d | %-12d | %-10d | %-8s | %-12d | %-10d |\n",
                   i,
                   gestorMemoria.tablaPaginas[i].idProceso,
                   gestorMemoria.tablaPaginas[i].numPagina,
                   gestorMemoria.tablaPaginas[i].idCarta,
                   gestorMemoria.tablaPaginas[i].enMemoria ? "Sí" : "No",
                   gestorMemoria.tablaPaginas[i].enMemoria ? gestorMemoria.tablaPaginas[i].marcoAsignado : -1,
                   gestorMemoria.tablaPaginas[i].tiempoUltimoUso);
            mostradas++;
        }
    }
    if (gestorMemoria.numPaginas == 0) printf("| (Tabla de páginas vacía)                                                                           |\n");
    printf("------------------------------------------------------------------------------------------------------\n\n");
}


// Cambiar entre algoritmos de memoria
void cambiarAlgoritmoMemoria(int nuevoAlgoritmo) {
    const char *nombreActualStr = "Desconocido";
    if(gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) nombreActualStr = "Ajuste Óptimo";
    else if(gestorMemoria.algoritmoActual == ALG_MAPA_BITS) nombreActualStr = "Mapa de Bits";
    else if(gestorMemoria.algoritmoActual == ALG_LRU) nombreActualStr = "LRU (Virtual)";


    if (nuevoAlgoritmo == gestorMemoria.algoritmoActual) {
        colorAmarillo();
        printf("Algoritmo de memoria ya establecido en: %s.\n", nombreActualStr);
        colorReset();
        // Si se selecciona LRU y ya está en LRU, simplemente imprimir estado virtual.
        if (nuevoAlgoritmo == ALG_LRU) imprimirEstadoMemoriaVirtual();
        else imprimirEstadoMemoria(); // Para particiones
        return;
    }

    const char *nombreNuevoStr = "Desconocido";
    bool esCambioValido = false;

    if (nuevoAlgoritmo == ALG_AJUSTE_OPTIMO) {
        nombreNuevoStr = "Ajuste Óptimo";
        esCambioValido = true;
    } else if (nuevoAlgoritmo == ALG_MAPA_BITS) {
        nombreNuevoStr = "Mapa de Bits";
        esCambioValido = true;
    } else if (nuevoAlgoritmo == ALG_LRU) {
        nombreNuevoStr = "LRU (Memoria Virtual)"; // Este modo es más bien para la gestión de páginas
        esCambioValido = true;                   // y no tanto un algoritmo de "asignación de memoria física" como los otros dos.
                                                 // El main lo usa, así que lo permitimos.
    }

    if (!esCambioValido) {
        colorRojo();
        printf("Error: Intento de cambiar a algoritmo de memoria no válido (%d).\n", nuevoAlgoritmo);
        colorReset();
        registrarEvento("Error: Intento de cambiar a algoritmo de memoria inválido %d.", nuevoAlgoritmo);
        return;
    }

    gestorMemoria.algoritmoActual = nuevoAlgoritmo;
    colorCian();
    printf("Algoritmo de memoria activo cambiado a: %s.\n", nombreNuevoStr);
    colorReset();
    registrarEvento("Algoritmo de memoria cambiado a: %s.", nombreNuevoStr);

    // Mostrar el estado de la memoria correspondiente al nuevo algoritmo
    if (nuevoAlgoritmo == ALG_LRU) {
        imprimirEstadoMemoriaVirtual(); // Si se cambia a LRU, mostrar el estado de la memoria virtual.
                                        // El estado de particiones (Ajuste Óptimo/Mapa Bits) sigue existiendo
                                        // pero no es el "activo" para nuevas asignaciones de `asignarMemoria()`.
    } else {
        imprimirEstadoMemoria(); // Para Ajuste Óptimo o Mapa de Bits, mostrar el estado de particiones.
    }
}