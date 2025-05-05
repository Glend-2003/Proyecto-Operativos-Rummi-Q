#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "memoria.h"
#include "jugadores.h"
#include "utilidades.h"
#include "juego.h" // Cambio: Incluir juego.h en lugar de juego.c

// Variable global para el gestor de memoria
GestorMemoria gestorMemoria;

// Declaración de funciones auxiliares privadas
void consolidarParticiones(void);

// Inicializar el sistema de memoria
void inicializarMemoria(void) {
    // Inicializar las particiones para ajuste óptimo
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
    
    // Inicializar la memoria virtual
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
    
    // Registrar evento
    registrarEvento("Sistema de memoria inicializado: %d bytes disponibles", MEM_TOTAL_SIZE);
    registrarEvento("Procesos que pueden crecer: %d y %d", gestorMemoria.creceProc1, gestorMemoria.creceProc2);
}
bool asignarMemoriaES(int idProceso) {
    // En lugar de intentar acceder directamente a jugadores para saber el número de cartas,
    // vamos a simular una cantidad de memoria requerida basada en una base fija
    // más una cantidad aleatoria para emular diferentes tamaños de solicitud de memoria
    int memoriaBase = 16; // Mínimo 16 bytes para la operación de E/S
    int memoriaPorCartaSimulada = 4; // Simulamos 4 bytes por cada "carta" procesada
    
    // Simulamos un número de "cartas" para calcular la memoria requerida
    // Podría ser aleatorio o basado en alguna otra métrica si fuera necesario.
    // Aquí usamos un rango aleatorio para variar las solicitudes.
    int numCartasSimuladas = 5 + (rand() % 8); // Simula entre 5 y 12 "cartas" procesadas
    int cantidadRequerida = memoriaBase + (numCartasSimuladas * memoriaPorCartaSimulada);
    
    // Llamar a la función principal de asignación de memoria con el algoritmo actual
    // Esta función usará Ajuste Óptimo o Mapa de Bits según esté configurado gestorMemoria.algoritmoActual
    bool asignado = asignarMemoria(idProceso, cantidadRequerida);
    
    // Si la asignación inicial tuvo éxito, y si este proceso es uno de los que pueden crecer,
    // intentar simular un crecimiento en su necesidad de memoria.
    // La lógica de crecimiento se aplica independientemente del algoritmo de asignación actual,
    // pero la forma en que crecerProceso lo maneje dependerá del algoritmo.
    
    // Nota: La lógica de `crecerProceso` necesita ser compatible tanto con Ajuste Óptimo
    // como con Mapa de Bits si ambos algoritmos manejan crecimiento.
    // Si crecerProceso solo fue implementado para un algoritmo, esta parte solo será efectiva con ese.
    
    if (asignado && (idProceso == gestorMemoria.creceProc1 || idProceso == gestorMemoria.creceProc2)) {
        
        // Calcular crecimiento (aleatorio entre 10 y 30 bytes adicionales solicitados)
        int crecimiento = (rand() % 21) + 10;
        
        colorMagenta();
        printf("El proceso %d (autorizado para crecer) intentará solicitar crecimiento en %d bytes\n", idProceso, crecimiento);
        colorReset();
        
        // Intentar hacer crecer el proceso. La implementación de crecerProceso
        // es responsable de cómo maneja esta solicitud con el algoritmo actual.
        crecerProceso(idProceso, crecimiento);
    }
    
    return asignado; // Retorna si la asignación inicial fue exitosa
}

// Implementación de la función liberarMemoria actualizada
void liberarMemoria(int idProceso) {
    bool liberada = false;
    int bytesLiberados = 0;

    if (gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO) {
        // Lógica para Ajuste Óptimo (la que ya tenías)
        for (int i = 0; i < gestorMemoria.numParticiones; i++) {
            if (gestorMemoria.particiones[i].idProceso == idProceso) {
                // Liberar esta partición
                int tamanoLiberado = gestorMemoria.particiones[i].tamano;
                gestorMemoria.particiones[i].libre = true;
                gestorMemoria.particiones[i].idProceso = -1; // Marcar como libre
                gestorMemoria.memoriaDisponible += tamanoLiberado;
                bytesLiberados += tamanoLiberado;
                liberada = true;

                colorAmarillo();
                printf("Memoria liberada (Ajuste Óptimo): Proceso %d, %d bytes\n", idProceso, tamanoLiberado);
                colorReset();

                // Fusionar con particiones libres adyacentes (solo para Ajuste Óptimo)
                consolidarParticiones();
                // Después de consolidar, es posible que el índice 'i' ya no sea válido para la siguiente iteración
                // Podrías necesitar reiniciar el bucle o ajustar 'i'.
                // Para simplificar, re-verificamos todas las particiones después de consolidar.
                i = -1; // Reiniciar el bucle para volver a verificar desde el principio
            }
        }
         if (!liberada) {
             printf("No se encontró memoria asignada al proceso %d para liberar con Ajuste Óptimo.\n", idProceso);
         }

    } else if (gestorMemoria.algoritmoActual == ALG_MAPA_BITS) {
        // Lógica para Mapa de Bits
        int bloquesLiberados = 0;
        for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
            if (gestorMemoria.mapaBits[i] == idProceso) { // Encontró un bloque que pertenece a este proceso
                gestorMemoria.mapaBits[i] = -1; // Marcar como libre (-1 indica libre)
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
        // Manejar caso de algoritmo no válido
        printf("Error: Algoritmo de liberación de memoria desconocido\n");
        // Asumiendo que la memoria no se libera si el algoritmo es desconocido
        return;
    }

    if (liberada) {
        // Registrar evento
        registrarEvento("Memoria liberada: Proceso %d, %d bytes (Algoritmo: %s)", idProceso, bytesLiberados,
                       gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" :
                       (gestorMemoria.algoritmoActual == ALG_MAPA_BITS ? "Mapa de Bits" : "Desconocido"));
    }

    // También liberar páginas virtuales (esto aplica independientemente del algoritmo de particionamiento)
    // Asegúrate de que liberarPaginasProceso maneje correctamente los IDs de proceso
    liberarPaginasProceso(idProceso);
}

// Consolidar particiones libres adyacentes
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
            
            // Reducir el número de particiones
            gestorMemoria.numParticiones--;
            
            // No incrementar i para verificar si hay más particiones adyacentes
        } else {
            // Avanzar a la siguiente partición
            i++;
        }
    }
}

// Implementación de la función asignarMemoria actualizada
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
        // Lógica para Ajuste Óptimo (la que ya tenías)

        // Buscar la partición libre de tamaño óptimo
        int indiceOptimo = -1;
        int tamanoOptimo = INT_MAX;

        for (int i = 0; i < gestorMemoria.numParticiones; i++) {
            if (gestorMemoria.particiones[i].libre &&
                gestorMemoria.particiones[i].tamano >= cantidadRequerida) {

                // Verificar si esta partición es mejor que la actual óptima
                if (gestorMemoria.particiones[i].tamano < tamanoOptimo) {
                    tamanoOptimo = gestorMemoria.particiones[i].tamano;
                    indiceOptimo = i;
                }
            }
        }

        // Si no se encontró una partición adecuada
        if (indiceOptimo == -1) {
            printf("Error (Ajuste Óptimo): No se encontró una partición adecuada para %d bytes\n", cantidadRequerida);
            asignado = false;
        } else {
            // Obtener la partición seleccionada
            Particion *particion = &gestorMemoria.particiones[indiceOptimo];

            // Si la partición es exactamente del tamaño requerido
            if (particion->tamano == cantidadRequerida) {
                particion->libre = false;
                particion->idProceso = idProceso;
                direccionAsignada = particion->inicio;
            }
            // Si la partición es mayor, dividirla
            else {
                // Crear una nueva partición para el resto del espacio libre
                if (gestorMemoria.numParticiones >= MAX_PARTICIONES) {
                     printf("Error (Ajuste Óptimo): Se alcanzó el límite máximo de particiones\n");
                     return false; // No se pudo crear nueva partición
                }

                // Hacer espacio para la nueva partición, insertándola *después* de la partición asignada
                for (int i = gestorMemoria.numParticiones; i > indiceOptimo + 1; i--) {
                    gestorMemoria.particiones[i] = gestorMemoria.particiones[i - 1];
                }

                // Crear la nueva partición libre restante
                gestorMemoria.particiones[indiceOptimo + 1].inicio = particion->inicio + cantidadRequerida;
                gestorMemoria.particiones[indiceOptimo + 1].tamano = particion->tamano - cantidadRequerida;
                gestorMemoria.particiones[indiceOptimo + 1].idProceso = -1; // Indicar que está libre
                gestorMemoria.particiones[indiceOptimo + 1].libre = true;

                // Ajustar la partición original para el proceso
                particion->tamano = cantidadRequerida;
                particion->libre = false;
                particion->idProceso = idProceso;
                direccionAsignada = particion->inicio;

                // Incrementar el número de particiones
                gestorMemoria.numParticiones++;
            }

            // Actualizar memoria disponible
            gestorMemoria.memoriaDisponible -= cantidadRequerida;
            asignado = true;
        }

    } else if (gestorMemoria.algoritmoActual == ALG_MAPA_BITS) {
        // Lógica para Mapa de Bits
        int numBloquesRequeridos = (cantidadRequerida + gestorMemoria.tamanoBloque - 1) / gestorMemoria.tamanoBloque;
        int bloquesLibresConsecutivos = 0;
        int inicioBloqueLibre = -1;

        for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
            if (gestorMemoria.mapaBits[i] == -1) { // Bloque libre (-1 indica libre)
                if (inicioBloqueLibre == -1) {
                    inicioBloqueLibre = i;
                }
                bloquesLibresConsecutivos++;
                if (bloquesLibresConsecutivos == numBloquesRequeridos) {
                    // Encontrado un bloque contiguo del tamaño adecuado
                    // Marcar bloques como ocupados con el ID del proceso
                    for (int j = 0; j < numBloquesRequeridos; j++) {
                        gestorMemoria.mapaBits[inicioBloqueLibre + j] = idProceso;
                    }
                    gestorMemoria.memoriaDisponible -= numBloquesRequeridos * gestorMemoria.tamanoBloque;
                    direccionAsignada = inicioBloqueLibre * gestorMemoria.tamanoBloque;
                    asignado = true;
                    break; // Encontrado y asignado, salir del bucle
                }
            } else { // Bloque ocupado, reiniciar contador de bloques libres consecutivos
                bloquesLibresConsecutivos = 0;
                inicioBloqueLibre = -1;
            }
        }

        if (!asignado) {
             printf("Error (Mapa de Bits): No se encontró espacio contiguo para %d bytes\n", cantidadRequerida);
        }

    } else {
         // Manejar caso de algoritmo no válido (si se añade más en el futuro)
         printf("Error: Algoritmo de asignación de memoria desconocido\n");
         return false;
    }

    if (asignado) {
         colorVerde();
         printf("Memoria asignada: Proceso %d, %d bytes, dirección %d (Algoritmo: %s)\n",
                idProceso, cantidadRequerida, direccionAsignada,
                gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" :
                (gestorMemoria.algoritmoActual == ALG_MAPA_BITS ? "Mapa de Bits" : "Desconocido"));
         colorReset();
         // Registrar evento con algoritmo
         registrarEvento("Memoria asignada: Proceso %d, %d bytes, dirección %d (Algoritmo: %s)",
                        idProceso, cantidadRequerida, direccionAsignada,
                        gestorMemoria.algoritmoActual == ALG_AJUSTE_OPTIMO ? "Ajuste Óptimo" :
                        (gestorMemoria.algoritmoActual == ALG_MAPA_BITS ? "Mapa de Bits" : "Desconocido"));
    }

    return asignado;
}
// Permitir que un proceso crezca (requiere más memoria)
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
    
    // Verificar si hay suficiente memoria disponible
    if (cantidadAdicional > gestorMemoria.memoriaDisponible) {
        colorRojo();
        printf("Error: No hay suficiente memoria disponible para el crecimiento (%d solicitados, %d disponibles)\n", 
               cantidadAdicional, gestorMemoria.memoriaDisponible);
        colorReset();
        return false;
    }
    
    // Buscar todas las particiones del proceso
    int partidaEncontrada = -1;
    
    for (int i = 0; i < gestorMemoria.numParticiones; i++) {
        if (gestorMemoria.particiones[i].idProceso == idProceso) {
            partidaEncontrada = i;
            
            // Verificar si hay una partición libre adyacente a esta que sea suficiente
            if (i + 1 < gestorMemoria.numParticiones && 
                gestorMemoria.particiones[i + 1].libre && 
                gestorMemoria.particiones[i + 1].tamano >= cantidadAdicional) {
                
                // Reducir el tamaño de la partición libre
                gestorMemoria.particiones[i + 1].inicio += cantidadAdicional;
                gestorMemoria.particiones[i + 1].tamano -= cantidadAdicional;
                
                // Aumentar el tamaño de la partición del proceso
                gestorMemoria.particiones[i].tamano += cantidadAdicional;
                
                // Actualizar memoria disponible
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
                
                // Registrar evento
                registrarEvento("Proceso %d creció en %d bytes", idProceso, cantidadAdicional);
                
                return true;
            }
            
            // Si encontramos una partición del proceso pero no podemos expandirla,
            // continuamos buscando otras
        }
    }
    
    // Si no encontramos una partición adecuada para expandir, intentar asignar nueva memoria
    if (partidaEncontrada != -1) {
        // Asignar nueva memoria para el crecimiento
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

// Imprimir el estado actual de la memoria
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
        // Implementar la impresión del mapa de bits (puede ser compleja si es bit a bit)
        // Un enfoque simple es imprimir el estado de cada byte/bloque
        for (int i = 0; i < NUM_BLOQUES_BITMAP; i++) {
             printf("%d", gestorMemoria.mapaBits[i]); // Imprime 0 o 1
             if ((i + 1) % 32 == 0) printf("\n"); // Salto de línea cada 32 bloques para mejor visualización
        }
        printf("\n");
    }

    printf("\nProcesos que pueden crecer: %d y %d\n", gestorMemoria.creceProc1, gestorMemoria.creceProc2);
    printf("===========================\n\n");
}

//---------------------- Funciones para memoria virtual con LRU -----------------------

// Inicializar la memoria virtual
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

// Acceder a una página (leer o escribir)
int accederPagina(int idProceso, int numPagina, int idCarta) {
    gestorMemoria.contadorTiempo++;
    
    // Buscar la página en la tabla de páginas
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
    
    // Actualizar tiempo de último uso (LRU)
    pagina->tiempoUltimoUso = gestorMemoria.contadorTiempo;
    pagina->bitReferencia = true;
    
    // Si la página no está en memoria, cargarla (fallo de página)
    if (!pagina->enMemoria) {
        gestorMemoria.fallosPagina++;
        
        colorAmarillo();
        printf("Fallo de página: Proceso %d, Página %d (Carta %d)\n", 
               idProceso, numPagina, idCarta);
        colorReset();
        
        // Buscar un marco libre
        int marcoLibre = -1;
        for (int i = 0; i < NUM_MARCOS; i++) {
            if (gestorMemoria.marcosMemoria[i].libre) {
                marcoLibre = i;
                break;
            }
        }
        
        // Si no hay marcos libres, seleccionar víctima usando LRU
        if (marcoLibre == -1) {
            marcoLibre = seleccionarVictimaLRU();
            
            // Obtener la página que estaba en el marco
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
                printf("Reemplazo LRU: Víctima Proceso %d, Página %d, Marco %d\n", 
                       paginaVictima->idProceso, paginaVictima->numPagina, marcoLibre);
                colorReset();
                
                // Marcar la página víctima como no en memoria
                paginaVictima->enMemoria = false;
                paginaVictima->marcoAsignado = -1;
                
                // Registrar evento
                registrarEvento("Reemplazo LRU: Víctima Proceso %d, Página %d, Marco %d", 
                               paginaVictima->idProceso, paginaVictima->numPagina, marcoLibre);
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
        printf("Página cargada: Proceso %d, Página %d -> Marco %d\n", 
               idProceso, numPagina, marcoLibre);
        colorReset();
        
        // Registrar evento
        registrarEvento("Fallo de página: Proceso %d, Página %d -> Marco %d", 
                       idProceso, numPagina, marcoLibre);
    } else {
        // La página ya está en memoria (acierto)
        gestorMemoria.aciertosMemoria++;
        
        colorVerde();
        printf("Acierto de memoria: Proceso %d, Página %d, Marco %d\n", 
               idProceso, numPagina, pagina->marcoAsignado);
        colorReset();
    }
    
    return pagina->marcoAsignado;
}

// Seleccionar una página víctima usando el algoritmo LRU
int seleccionarVictimaLRU(void) {
    int marcoPagina = -1;
    int tiempoMasAntiguo = INT_MAX;
    
    // Buscar la página con el tiempo de uso más antiguo
    for (int i = 0; i < NUM_MARCOS; i++) {
        if (!gestorMemoria.marcosMemoria[i].libre) {
            // Encontrar la página en este marco
            for (int j = 0; j < gestorMemoria.numPaginas; j++) {
                if (gestorMemoria.tablaPaginas[j].enMemoria && 
                    gestorMemoria.tablaPaginas[j].marcoAsignado == i) {
                    
                    // Si esta página es más antigua que la actual
                    if (gestorMemoria.tablaPaginas[j].tiempoUltimoUso < tiempoMasAntiguo) {
                        tiempoMasAntiguo = gestorMemoria.tablaPaginas[j].tiempoUltimoUso;
                        marcoPagina = i;
                    }
                    
                    break;
                }
            }
        }
    }
    
    return marcoPagina;
}

// Liberar todas las páginas de un proceso
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
    
    // Registrar evento
    registrarEvento("Páginas liberadas: Proceso %d", idProceso);
}

// Imprimir el estado actual de la memoria virtual
void imprimirEstadoMemoriaVirtual(void) {
    printf("\n=== ESTADO DE LA MEMORIA VIRTUAL ===\n");
    printf("Marcos totales: %d\n", NUM_MARCOS);
    printf("Páginas por marco: %d\n", PAGINAS_POR_MARCO);
    printf("Fallos de página: %d\n", gestorMemoria.fallosPagina);
    printf("Aciertos de memoria: %d\n", gestorMemoria.aciertosMemoria);
    
    if (gestorMemoria.fallosPagina + gestorMemoria.aciertosMemoria > 0) {
        float tasaAciertos = (float)gestorMemoria.aciertosMemoria / 
                            (gestorMemoria.fallosPagina + gestorMemoria.aciertosMemoria) * 100;
        printf("Tasa de aciertos: %.2f%%\n", tasaAciertos);
    }
    
    printf("\nMarcos en memoria principal:\n");
    printf("%-8s %-10s %-10s %-10s\n", "Marco", "Proceso", "Página", "Estado");
    printf("--------------------------------------\n");
    
    for (int i = 0; i < NUM_MARCOS; i++) {
        printf("%-8d %-10d %-10d %-10s\n", 
               i,
               gestorMemoria.marcosMemoria[i].idProceso,
               gestorMemoria.marcosMemoria[i].numPagina,
               gestorMemoria.marcosMemoria[i].libre ? "Libre" : "Ocupado");
    }
    
    printf("\nPáginas (máximo 10 mostradas):\n");
    printf("%-8s %-8s %-8s %-8s %-12s %-8s\n", 
           "Proceso", "Página", "Carta ID", "En Mem.", "Marco", "Últ. Uso");
    printf("------------------------------------------------------\n");
    
    int mostradas = 0;
    for (int i = 0; i < gestorMemoria.numPaginas && mostradas < 10; i++) {
        if (gestorMemoria.tablaPaginas[i].idProceso != -1) {
            printf("%-8d %-8d %-8d %-8s %-12d %-8d\n", 
                   gestorMemoria.tablaPaginas[i].idProceso,
                   gestorMemoria.tablaPaginas[i].numPagina,
                   gestorMemoria.tablaPaginas[i].idCarta,
                   gestorMemoria.tablaPaginas[i].enMemoria ? "Sí" : "No",
                   gestorMemoria.tablaPaginas[i].marcoAsignado,
                   gestorMemoria.tablaPaginas[i].tiempoUltimoUso);
            mostradas++;
        }
    }
    
    printf("===========================\n\n");
}

// Cambiar entre algoritmos de memoria
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