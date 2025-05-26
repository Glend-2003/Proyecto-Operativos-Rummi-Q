#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "memoria.h"
#include "jugadores.h"
#include "utilidades.h"
#include "juego.h"

// Variable global para el gestor de memoria
GestorMemoria gestorMemoria;

// Declaración de funciones auxiliares privadas
void consolidarParticiones(void);

//==============================================================================
// FUNCIONES DE INICIALIZACIÓN
//==============================================================================

void inicializarMemoria(void) {
    memset(&gestorMemoria, 0, NUM_BLOQUES_BITMAP);
    gestorMemoria.tamanoBloque = TAMANO_BLOQUE_BITMAP;
    
    // Crear partición inicial que abarca toda la memoria
    gestorMemoria.particiones[0].inicio = 0;
    gestorMemoria.particiones[0].tamano = MEM_TOTAL_SIZE;
    gestorMemoria.particiones[0].idProceso = -1;
    gestorMemoria.particiones[0].libre = true;
    
    gestorMemoria.numParticiones = 1;
    gestorMemoria.memoriaDisponible = MEM_TOTAL_SIZE;
    gestorMemoria.algoritmoActual = ALG_AJUSTE_OPTIMO;
    
    inicializarMemoriaVirtual();
    
    // Seleccionar aleatoriamente dos procesos que pueden crecer
    gestorMemoria.creceProc1 = rand() % MAX_JUGADORES;
    do {
        gestorMemoria.creceProc2 = rand() % MAX_JUGADORES;
    } while (gestorMemoria.creceProc2 == gestorMemoria.creceProc1);
    
    colorVerde();
    printf("Sistema de memoria inicializado: %d bytes disponibles\n", MEM_TOTAL_SIZE);
    printf("Procesos que pueden crecer: %d y %d\n", gestorMemoria.creceProc1, gestorMemoria.creceProc2);
    colorReset();
    
    registrarEvento("Sistema de memoria inicializado: %d bytes disponibles", MEM_TOTAL_SIZE);
    registrarEvento("Procesos que pueden crecer: %d y %d", gestorMemoria.creceProc1, gestorMemoria.creceProc2);
}

void inicializarMemoriaVirtual(void) {
    // Inicializar tabla de páginas
    for (int i = 0; i < MAX_PAGINAS; i++) {
        gestorMemoria.tablaPaginas[i].idProceso = -1;
        gestorMemoria.tablaPaginas[i].numPagina = -1;
        gestorMemoria.tablaPaginas[i].idCarta = -1;
        gestorMemoria.tablaPaginas[i].tiempoUltimoUso = 0;
        gestorMemoria.tablaPaginas[i].bitReferencia = false;
        gestorMemoria.tablaPaginas[i].bitModificacion = false;
        gestorMemoria.tablaPaginas[i].enMemoria = false;
        gestorMemoria.tablaPaginas[i].marcoAsignado = -1;
    }
    
    gestorMemoria.numPaginas = 0;
    
    // Inicializar marcos de memoria
    for (int i = 0; i < NUM_MARCOS; i++) {
        gestorMemoria.marcosMemoria[i].idProceso = -1;
        gestorMemoria.marcosMemoria[i].numPagina = -1;
        gestorMemoria.marcosMemoria[i].libre = true;
    }
    
    gestorMemoria.contadorTiempo = 0;
    gestorMemoria.fallosPagina = 0;
    gestorMemoria.aciertosMemoria = 0;
    
    colorVerde();
    printf("Memoria virtual inicializada: %d marcos, %d páginas por marco\n", 
           NUM_MARCOS, PAGINAS_POR_MARCO);
    colorReset();
}

//==============================================================================
// FUNCIONES DE ASIGNACIÓN DE MEMORIA
//==============================================================================

bool asignarMemoria(int idProceso, int cantidadRequerida) {
    if (cantidadRequerida <= 0) {
        printf("Error: La cantidad de memoria solicitada debe ser mayor que cero\n");
        return false;
    }

    if (cantidadRequerida > gestorMemoria.memoriaDisponible) {
        printf("Error: No hay suficiente memoria disponible (%d solicitados, %d disponibles)\n",
               cantidadRequerida, gestorMemoria.memoriaDisponible);
        return false;
    }

    bool asignado = false;
    int direccionAsignada = -1;

    if (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) {
        // Buscar la partición libre de MENOR tamaño que pueda contener el proceso
        int indiceOptimo = -1;
        int tamanoOptimo = INT_MAX;

        for (int i = 0; i < gestorMemoria.numParticiones; i++) {
            if (gestorMemoria.particiones[i].libre &&
                gestorMemoria.particiones[i].tamano >= cantidadRequerida) {

                if (gestorMemoria.particiones[i].tamano < tamanoOptimo) {
                    tamanoOptimo = gestorMemoria.particiones[i].tamano;
                    indiceOptimo = i;
                }
            }
        }

        if (indiceOptimo == -1) {
            printf("Error (Ajuste Óptimo): No se encontró una partición adecuada para %d bytes\n", cantidadRequerida);
            asignado = false;
        } else {
            Particion *particion = &gestorMemoria.particiones[indiceOptimo];

            // Si la partición es exactamente del tamaño requerido
            if (particion->tamano == cantidadRequerida) {
                particion->libre = false;
                particion->idProceso = idProceso;
                direccionAsignada = particion->inicio;
            }
            // Si la partición es mayor, dividirla
            else {
                if (gestorMemoria.numParticiones >= MAX_PARTICIONES) {
                     printf("Error (Ajuste Óptimo): Se alcanzó el límite máximo de particiones\n");
                     return false;
                }

                // Hacer espacio para la nueva partición
                for (int i = gestorMemoria.numParticiones; i > indiceOptimo + 1; i--) {
                    gestorMemoria.particiones[i] = gestorMemoria.particiones[i - 1];
                }

                // Crear la nueva partición libre restante
                gestorMemoria.particiones[indiceOptimo + 1].inicio = particion->inicio + cantidadRequerida;
                gestorMemoria.particiones[indiceOptimo + 1].tamano = particion->tamano - cantidadRequerida;
                gestorMemoria.particiones[indiceOptimo + 1].idProceso = -1;
                gestorMemoria.particiones[indiceOptimo + 1].libre = true;

                // Ajustar la partición original para el proceso
                particion->tamano = cantidadRequerida;
                particion->libre = false;
                particion->idProceso = idProceso;
                direccionAsignada = particion->inicio;

                gestorMemoria.numParticiones++;
            }

            gestorMemoria.memoriaDisponible -= cantidadRequerida;
            asignado = true;
        }

    } else if (gestorMemoria.algoritmoActual == ALG_MAPA_BITS) {
        int numBloquesRequeridos = (cantidadRequerida + gestorMemoria.tamanoBloque - 1) / gestorMemoria.tamanoBloque;
        int bloquesLibresConsecutivos = 0;
        int inicioBloqueLibre = -1;

        for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
            if (gestorMemoria.mapaBits[i] == -1) {
                if (inicioBloqueLibre == -1) {
                    inicioBloqueLibre = i;
                }
                bloquesLibresConsecutivos++;
                if (bloquesLibresConsecutivos == numBloquesRequeridos) {
                    for (int j = 0; j < numBloquesRequeridos; j++) {
                        gestorMemoria.mapaBits[inicioBloqueLibre + j] = idProceso;
                    }
                    gestorMemoria.memoriaDisponible -= numBloquesRequeridos * gestorMemoria.tamanoBloque;
                    direccionAsignada = inicioBloqueLibre * gestorMemoria.tamanoBloque;
                    asignado = true;
                    break;
                }
            } else {
                bloquesLibresConsecutivos = 0;
                inicioBloqueLibre = -1;
            }
        }

        if (!asignado) {
             printf("Error (Mapa de Bits): No se encontró espacio contiguo para %d bytes\n", cantidadRequerida);
        }
    }

    if (asignado) {
         colorVerde();
         printf("Memoria asignada: Proceso %d, %d bytes, dirección %d (Algoritmo: %s)\n",
                idProceso, cantidadRequerida, direccionAsignada,
                gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" : "Mapa de Bits");
         colorReset();
         registrarEvento("Memoria asignada: Proceso %d, %d bytes, dirección %d (Algoritmo: %s)",
                        idProceso, cantidadRequerida, direccionAsignada,
                        gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" : "Mapa de Bits");
    }

    return asignado;
}

bool asignarMemoriaES(int idProceso) {
    int memoriaBase = 16;
    int memoriaPorCartaSimulada = 4;
    int numCartasSimuladas = 5 + (rand() % 8);
    int cantidadRequerida = memoriaBase + (numCartasSimuladas * memoriaPorCartaSimulada);
    
    bool asignado = asignarMemoria(idProceso, cantidadRequerida);
    
    if (asignado && (idProceso == gestorMemoria.creceProc1 || idProceso == gestorMemoria.creceProc2)) {
        int crecimiento = (rand() % 21) + 10;
        
        colorMagenta();
        printf("El proceso %d (autorizado para crecer) intentará solicitar crecimiento en %d bytes\n", idProceso, crecimiento);
        colorReset();
        
        crecerProceso(idProceso, crecimiento);
    }
    
    return asignado;
}

bool crecerProceso(int idProceso, int cantidadAdicional) {
    if (cantidadAdicional <= 0) {
        printf("Error: La cantidad adicional debe ser mayor que cero\n");
        return false;
    }
    
    // Verificar si el proceso es uno de los que pueden crecer
    if (idProceso != gestorMemoria.creceProc1 && idProceso != gestorMemoria.creceProc2) {
        colorRojo();
        printf("Error: El proceso %d no está autorizado para crecer\n", idProceso);
        colorReset();
        return false;
    }
    
    if (cantidadAdicional > gestorMemoria.memoriaDisponible) {
        colorRojo();
        printf("Error: No hay suficiente memoria disponible para el crecimiento (%d solicitados, %d disponibles)\n", 
               cantidadAdicional, gestorMemoria.memoriaDisponible);
        colorReset();
        return false;
    }
    
    int partidaEncontrada = -1;
    
    for (int i = 0; i < gestorMemoria.numParticiones; i++) {
        if (gestorMemoria.particiones[i].idProceso == idProceso) {
            partidaEncontrada = i;
            
            // Verificar si hay una partición libre adyacente suficiente
            if (i + 1 < gestorMemoria.numParticiones && 
                gestorMemoria.particiones[i + 1].libre && 
                gestorMemoria.particiones[i + 1].tamano >= cantidadAdicional) {
                
                // Reducir el tamaño de la partición libre
                gestorMemoria.particiones[i + 1].inicio += cantidadAdicional;
                gestorMemoria.particiones[i + 1].tamano -= cantidadAdicional;
                
                // Aumentar el tamaño de la partición del proceso
                gestorMemoria.particiones[i].tamano += cantidadAdicional;
                gestorMemoria.memoriaDisponible -= cantidadAdicional;
                
                colorVerde();
                printf("Proceso %d creció en %d bytes. Nueva partición: inicio %d, tamaño %d\n", 
                       idProceso, cantidadAdicional, gestorMemoria.particiones[i].inicio, gestorMemoria.particiones[i].tamano);
                colorReset();
                
                // Si la partición libre quedó con tamaño 0, eliminarla
                if (gestorMemoria.particiones[i + 1].tamano == 0) {
                    for (int j = i + 1; j < gestorMemoria.numParticiones - 1; j++) {
                        gestorMemoria.particiones[j] = gestorMemoria.particiones[j + 1];
                    }
                    gestorMemoria.numParticiones--;
                }
                
                registrarEvento("Proceso %d creció en %d bytes", idProceso, cantidadAdicional);
                return true;
            }
        }
    }
    
    // Si no se pudo expandir una partición existente, asignar nueva memoria
    if (partidaEncontrada != -1) {
        if (asignarMemoria(idProceso, cantidadAdicional)) {
            colorVerde();
            printf("Proceso %d creció en %d bytes (nueva partición)\n", idProceso, cantidadAdicional);
            colorReset();
            return true;
        }
    }
    
    colorRojo();
    printf("Error: No se pudo hacer crecer el proceso %d\n", idProceso);
    colorReset();
    return false;
}

//==============================================================================
// FUNCIONES DE LIBERACIÓN DE MEMORIA
//==============================================================================

void liberarMemoria(int idProceso) {
    bool liberada = false;
    int bytesLiberados = 0;

    if (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) {
        for (int i = 0; i < gestorMemoria.numParticiones; i++) {
            if (gestorMemoria.particiones[i].idProceso == idProceso) {
                int tamanoLiberado = gestorMemoria.particiones[i].tamano;
                gestorMemoria.particiones[i].libre = true;
                gestorMemoria.particiones[i].idProceso = -1;
                gestorMemoria.memoriaDisponible += tamanoLiberado;
                bytesLiberados += tamanoLiberado;
                liberada = true;

                colorAmarillo();
                printf("Memoria liberada (Ajuste Óptimo): Proceso %d, %d bytes\n", idProceso, tamanoLiberado);
                colorReset();

                consolidarParticiones();
                i = -1; // Reiniciar el bucle después de consolidar
            }
        }
        
        if (!liberada) {
             printf("No se encontró memoria asignada al proceso %d para liberar con Ajuste Óptimo.\n", idProceso);
        }

    } else if (gestorMemoria.algoritmoActual == ALG_MAPA_BITS) {
        int bloquesLiberados = 0;
        for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
            if (gestorMemoria.mapaBits[i] == idProceso) {
                gestorMemoria.mapaBits[i] = -1;
                gestorMemoria.memoriaDisponible += gestorMemoria.tamanoBloque;
                bloquesLiberados++;
                liberada = true;
            }
        }

        if (liberada) {
             bytesLiberados = bloquesLiberados * gestorMemoria.tamanoBloque;
             colorAmarillo();
             printf("Memoria liberada (Mapa de Bits): Proceso %d, %d bloques (%d bytes)\n",
                    idProceso, bloquesLiberados, bytesLiberados);
             colorReset();
        } else {
             printf("No se encontró memoria asignada al proceso %d para liberar con Mapa de Bits.\n", idProceso);
        }

    } else {
        printf("Error: Algoritmo de liberación de memoria desconocido\n");
        return;
    }

    if (liberada) {
        registrarEvento("Memoria liberada: Proceso %d, %d bytes (Algoritmo: %s)", idProceso, bytesLiberados,
                       gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" :
                       (gestorMemoria.algoritmoActual == ALG_MAPA_BITS ? "Mapa de Bits" : "Desconocido"));
    }

    liberarPaginasProceso(idProceso);
}

void consolidarParticiones(void) {
    int i = 0;
    
    while (i < gestorMemoria.numParticiones - 1) {
        // Si esta partición y la siguiente están libres
        if (gestorMemoria.particiones[i].libre && gestorMemoria.particiones[i + 1].libre) {
            // Fusionar las particiones
            gestorMemoria.particiones[i].tamano += gestorMemoria.particiones[i + 1].tamano;
            
            // Eliminar la partición siguiente
            for (int j = i + 1; j < gestorMemoria.numParticiones - 1; j++) {
                gestorMemoria.particiones[j] = gestorMemoria.particiones[j + 1];
            }
            
            gestorMemoria.numParticiones--;
        } else {
            i++;
        }
    }
}

void liberarPaginasProceso(int idProceso) {
    for (int i = 0; i < gestorMemoria.numPaginas; i++) {
        if (gestorMemoria.tablaPaginas[i].idProceso == idProceso) {
            // Si la página está en memoria, liberar el marco
            if (gestorMemoria.tablaPaginas[i].enMemoria) {
                int marco = gestorMemoria.tablaPaginas[i].marcoAsignado;
                gestorMemoria.marcosMemoria[marco].idProceso = -1;
                gestorMemoria.marcosMemoria[marco].numPagina = -1;
                gestorMemoria.marcosMemoria[marco].libre = true;
            }
            
            // Marcar la página como no utilizada
            gestorMemoria.tablaPaginas[i].idProceso = -1;
            gestorMemoria.tablaPaginas[i].numPagina = -1;
            gestorMemoria.tablaPaginas[i].idCarta = -1;
            gestorMemoria.tablaPaginas[i].enMemoria = false;
            gestorMemoria.tablaPaginas[i].marcoAsignado = -1;
        }
    }
    
    registrarEvento("Páginas liberadas: Proceso %d", idProceso);
}

//==============================================================================
// FUNCIONES DE MEMORIA VIRTUAL (LRU)
//==============================================================================

int accederPagina(int idProceso, int numPagina, int idCarta) {
    gestorMemoria.contadorTiempo++;
    
    Pagina *pagina = NULL;
    
    for (int i = 0; i < gestorMemoria.numPaginas; i++) {
        if (gestorMemoria.tablaPaginas[i].idProceso == idProceso && 
            gestorMemoria.tablaPaginas[i].numPagina == numPagina) {
            pagina = &gestorMemoria.tablaPaginas[i];
            break;
        }
    }
    
    // Si la página no existe, crearla
    if (pagina == NULL) {
        if (gestorMemoria.numPaginas >= MAX_PAGINAS) {
            colorRojo();
            printf("Error: Se alcanzó el límite máximo de páginas\n");
            colorReset();
            return -1;
        }
        
        pagina = &gestorMemoria.tablaPaginas[gestorMemoria.numPaginas];
        pagina->idProceso = idProceso;
        pagina->numPagina = numPagina;
        pagina->idCarta = idCarta;
        pagina->tiempoUltimoUso = gestorMemoria.contadorTiempo;
        pagina->bitReferencia = true;
        pagina->bitModificacion = false;
        pagina->enMemoria = false;
        pagina->marcoAsignado = -1;
        
        gestorMemoria.numPaginas++;
    }
    
    // Actualizar tiempo de último uso
    pagina->tiempoUltimoUso = gestorMemoria.contadorTiempo;
    pagina->bitReferencia = true;
    
    // Si la página está en memoria, es un acierto
    if (pagina->enMemoria) {
        gestorMemoria.aciertosMemoria++;
        
        colorVerde();
        printf("Acierto de memoria: Proceso %d, Página %d, Marco %d, Tiempo: %d\n", 
               idProceso, numPagina, pagina->marcoAsignado, pagina->tiempoUltimoUso);
        colorReset();
        
        return pagina->marcoAsignado;
    }
    
    // Si no está en memoria, es un fallo de página
    gestorMemoria.fallosPagina++;
    
    colorAmarillo();
    printf("Fallo de página: Proceso %d, Página %d (Carta %d), Tiempo: %d\n", 
           idProceso, numPagina, idCarta, gestorMemoria.contadorTiempo);
    colorReset();
    
    // Buscar un marco libre
    int marcoLibre = -1;
    for (int i = 0; i < NUM_MARCOS; i++) {
        if (gestorMemoria.marcosMemoria[i].libre) {
            marcoLibre = i;
            break;
        }
    }
    
    // Si no hay marcos libres, usar LRU para seleccionar víctima
    if (marcoLibre == -1) {
        marcoLibre = seleccionarVictimaLRU();

        Pagina *paginaVictima = NULL;
        for (int i = 0; i < gestorMemoria.numPaginas; i++) {
            if (gestorMemoria.tablaPaginas[i].enMemoria && 
                gestorMemoria.tablaPaginas[i].marcoAsignado == marcoLibre) {
                paginaVictima = &gestorMemoria.tablaPaginas[i];
                break;
            }
        }
        
        if (paginaVictima != NULL) {
            colorAmarillo();
            printf("Reemplazo LRU: Víctima Proceso %d, Página %d, Marco %d, Tiempo último uso: %d\n", 
                   paginaVictima->idProceso, paginaVictima->numPagina, marcoLibre, paginaVictima->tiempoUltimoUso);
            colorReset();
            
            // Marcar la página víctima como no en memoria
            paginaVictima->enMemoria = false;
            paginaVictima->marcoAsignado = -1;
            
            registrarEvento("Reemplazo LRU: Víctima Proceso %d, Página %d, Marco %d, Tiempo: %d", 
                           paginaVictima->idProceso, paginaVictima->numPagina, marcoLibre, paginaVictima->tiempoUltimoUso);
        }
    }
    
    // Asignar el marco a la página
    pagina->enMemoria = true;
    pagina->marcoAsignado = marcoLibre;
    
    // Actualizar el marco
    gestorMemoria.marcosMemoria[marcoLibre].idProceso = idProceso;
    gestorMemoria.marcosMemoria[marcoLibre].numPagina = numPagina;
    gestorMemoria.marcosMemoria[marcoLibre].libre = false;
    
    colorVerde();
    printf("Página cargada: Proceso %d, Página %d -> Marco %d, Tiempo: %d\n", 
           idProceso, numPagina, marcoLibre, gestorMemoria.contadorTiempo);
    colorReset();
    
    registrarEvento("Fallo de página: Proceso %d, Página %d -> Marco %d, Tiempo: %d", 
                   idProceso, numPagina, marcoLibre, gestorMemoria.contadorTiempo);
    
    return marcoLibre;
}

int seleccionarVictimaLRU(void) {
    int marcoVictima = -1;
    int tiempoMasAntiguo = INT_MAX;
    
    // Buscar la página con el tiempo de último uso MÁS ANTIGUO (menor)
    for (int i = 0; i < NUM_MARCOS; i++) {
        if (!gestorMemoria.marcosMemoria[i].libre) {
            for (int j = 0; j < gestorMemoria.numPaginas; j++) {
                if (gestorMemoria.tablaPaginas[j].enMemoria && 
                    gestorMemoria.tablaPaginas[j].marcoAsignado == i) {
                    
                    if (gestorMemoria.tablaPaginas[j].tiempoUltimoUso < tiempoMasAntiguo) {
                        tiempoMasAntiguo = gestorMemoria.tablaPaginas[j].tiempoUltimoUso;
                        marcoVictima = i;
                    }
                    break;
                }
            }
        }
    }
    
    if (marcoVictima == -1) {
        for (int i = 0; i < NUM_MARCOS; i++) {
            if (!gestorMemoria.marcosMemoria[i].libre) {
                marcoVictima = i;
                break;
            }
        }
    }
    
    return marcoVictima;
}

//==============================================================================
// FUNCIONES DE VISUALIZACIÓN
//==============================================================================

void imprimirEstadoMemoria(void) {
    printf("\n=== ESTADO DE LA MEMORIA ===\n");
    printf("Algoritmo actual: %s\n", gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" : (gestorMemoria.algoritmoActual == ALG_LRU ? "LRU (Memoria Virtual)" : "Mapa de Bits"));
    printf("Memoria total: %d bytes\n", MEM_TOTAL_SIZE);
    printf("Memoria disponible: %d bytes\n", gestorMemoria.memoriaDisponible);

    if (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) {
         printf("Número de particiones: %d\n", gestorMemoria.numParticiones);
         printf("\nParticiones:\n");
         printf("%-10s %-10s %-10s %-10s\n", "Inicio", "Tamaño", "Proceso", "Estado");
         printf("--------------------------------------\n");
         for (int i = 0; i < gestorMemoria.numParticiones; i++) {
             printf("%-10d %-10d %-10d %-10s\n",
                    gestorMemoria.particiones[i].inicio,
                    gestorMemoria.particiones[i].tamano,
                    gestorMemoria.particiones[i].idProceso,
                    gestorMemoria.particiones[i].libre ? "Libre" : "Ocupado");
         }
    } else if (gestorMemoria.algoritmoActual == ALG_MAPA_BITS) {
        printf("Tamaño del bloque: %d bytes\n", gestorMemoria.tamanoBloque);
        printf("Número de bloques: %d\n", NUM_BLOQUES_BITMAP);
        printf("\nMapa de Bits:\n");
        for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
             printf("%d", gestorMemoria.mapaBits[i]);
             if ((i + 1) % 32 == 0) printf("\n");
        }
        printf("\n");
    }

    printf("\nProcesos que pueden crecer: %d y %d\n", gestorMemoria.creceProc1, gestorMemoria.creceProc2);
    printf("===========================\n\n");
}

void imprimirEstadoMemoriaVirtual(void) {
    printf("\n=== ESTADO DE LA MEMORIA VIRTUAL (LRU) ===\n");
    printf("Algoritmo: LRU (Least Recently Used)\n");
    printf("Marcos totales: %d\n", NUM_MARCOS);
    printf("Páginas por marco: %d\n", PAGINAS_POR_MARCO);
    printf("Fallos de página: %d\n", gestorMemoria.fallosPagina);
    printf("Aciertos de memoria: %d\n", gestorMemoria.aciertosMemoria);
    printf("Tiempo actual del sistema: %d\n", gestorMemoria.contadorTiempo);
    
    if (gestorMemoria.fallosPagina + gestorMemoria.aciertosMemoria > 0) {
        float tasaAciertos = (float)gestorMemoria.aciertosMemoria / 
                            (gestorMemoria.fallosPagina + gestorMemoria.aciertosMemoria) * 100;
        printf("Tasa de aciertos: %.2f%%\n", tasaAciertos);
    }
    
    printf("\nMarcos en memoria principal:\n");
    printf("%-8s %-10s %-10s %-12s %-10s\n", "Marco", "Proceso", "Página", "Tiempo Uso", "Estado");
    printf("--------------------------------------------------------\n");
    
    for (int i = 0; i < NUM_MARCOS; i++) {
        printf("%-8d %-10d %-10d ", 
               i,
               gestorMemoria.marcosMemoria[i].idProceso,
               gestorMemoria.marcosMemoria[i].numPagina);
        
        // Buscar el tiempo de último uso de la página en este marco
        int tiempoUso = -1;
        if (!gestorMemoria.marcosMemoria[i].libre) {
            for (int j = 0; j < gestorMemoria.numPaginas; j++) {
                if (gestorMemoria.tablaPaginas[j].enMemoria && 
                    gestorMemoria.tablaPaginas[j].marcoAsignado == i) {
                    tiempoUso = gestorMemoria.tablaPaginas[j].tiempoUltimoUso;
                    break;
                }
            }
        }
        
        printf("%-12d %-10s\n", 
               tiempoUso,
               gestorMemoria.marcosMemoria[i].libre ? "Libre" : "Ocupado");
    }
    
    printf("\nPáginas en memoria (ordenadas por tiempo de uso):\n");
    printf("%-8s %-8s %-8s %-12s %-8s %-8s\n", 
           "Proceso", "Página", "Carta ID", "Tiempo Uso", "Marco", "En Mem.");
    printf("----------------------------------------------------------\n");
    
    // Crear array temporal para ordenar páginas por tiempo de uso
    typedef struct {
        int indice;
        int tiempoUso;
    } PaginaOrdenada;
    
    PaginaOrdenada paginasOrdenadas[MAX_PAGINAS];
    int numPaginasActivas = 0;
    
    for (int i = 0; i < gestorMemoria.numPaginas; i++) {
        if (gestorMemoria.tablaPaginas[i].idProceso != -1) {
            paginasOrdenadas[numPaginasActivas].indice = i;
            paginasOrdenadas[numPaginasActivas].tiempoUso = gestorMemoria.tablaPaginas[i].tiempoUltimoUso;
            numPaginasActivas++;
        }
    }
    
    // Ordenar por tiempo de uso (más reciente primero)
    for (int i = 0; i < numPaginasActivas - 1; i++) {
        for (int j = 0; j < numPaginasActivas - i - 1; j++) {
            if (paginasOrdenadas[j].tiempoUso < paginasOrdenadas[j + 1].tiempoUso) {
                PaginaOrdenada temp = paginasOrdenadas[j];
                paginasOrdenadas[j] = paginasOrdenadas[j + 1];
                paginasOrdenadas[j + 1] = temp;
            }
        }
    }
    
    // Mostrar hasta 10 páginas
    int mostradas = 0;
    for (int i = 0; i < numPaginasActivas && mostradas < 10; i++) {
        int idx = paginasOrdenadas[i].indice;
        Pagina *p = &gestorMemoria.tablaPaginas[idx];
        
        printf("%-8d %-8d %-8d %-12d %-8d %-8s\n", 
               p->idProceso,
               p->numPagina,
               p->idCarta,
               p->tiempoUltimoUso,
               p->marcoAsignado,
               p->enMemoria ? "Sí" : "No");
        mostradas++;
    }
    
    if (numPaginasActivas > 10) {
        printf("... y %d páginas más\n", numPaginasActivas - 10);
    }
    
    printf("===========================\n\n");
}

//==============================================================================
// FUNCIONES DE CONFIGURACIÓN
//==============================================================================

void cambiarAlgoritmoMemoria(int nuevoAlgoritmo) {
    if (nuevoAlgoritmo != ALG_AJUSTE_OPTIMO && nuevoAlgoritmo != ALG_LRU && nuevoAlgoritmo != ALG_MAPA_BITS) {
        printf("Error: Algoritmo de memoria no válido\n");
        return;
    }
    
    gestorMemoria.algoritmoActual = nuevoAlgoritmo;
    
    const char *nombres[] = {"Ajuste Óptimo", "LRU (Least Recently Used)", "Mapa de Bits"}; 
    colorCian();
    printf("Algoritmo de memoria cambiado a: %s\n", nombres[nuevoAlgoritmo]);
    colorReset();

    registrarEvento("Algoritmo de memoria cambiado a: %s", nombres[nuevoAlgoritmo]);
}