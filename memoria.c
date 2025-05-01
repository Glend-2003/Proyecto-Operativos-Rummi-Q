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
    memset(&gestorMemoria, 0, sizeof(GestorMemoria));
    
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

// Implementación del Algoritmo de Ajuste Óptimo
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
    
    // Buscar la partición libre de tamaño óptimo
    // El óptimo es la partición libre más pequeña que pueda contener la cantidad requerida
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
        printf("Error: No se encontró una partición adecuada para %d bytes\n", cantidadRequerida);
        return false;
    }
    
    // Obtener la partición seleccionada
    Particion *particion = &gestorMemoria.particiones[indiceOptimo];
    
    // Si la partición es exactamente del tamaño requerido
    if (particion->tamano == cantidadRequerida) {
        particion->libre = false;
        particion->idProceso = idProceso;
    }
    // Si la partición es mayor, dividirla
    else {
        // Crear una nueva partición para el proceso
        if (gestorMemoria.numParticiones >= MAX_PARTICIONES) {
            printf("Error: Se alcanzó el límite máximo de particiones\n");
            return false;
        }
        
        // Hacer espacio para la nueva partición
        for (int i = gestorMemoria.numParticiones; i > indiceOptimo + 1; i--) {
            gestorMemoria.particiones[i] = gestorMemoria.particiones[i - 1];
        }
        
        // Crear la nueva partición para el proceso
        gestorMemoria.particiones[indiceOptimo + 1].inicio = particion->inicio + cantidadRequerida;
        gestorMemoria.particiones[indiceOptimo + 1].tamano = particion->tamano - cantidadRequerida;
        gestorMemoria.particiones[indiceOptimo + 1].idProceso = -1;
        gestorMemoria.particiones[indiceOptimo + 1].libre = true;
        
        // Ajustar la partición original
        particion->tamano = cantidadRequerida;
        particion->libre = false;
        particion->idProceso = idProceso;
        
        // Incrementar el número de particiones
        gestorMemoria.numParticiones++;
    }
    
    // Actualizar memoria disponible
    gestorMemoria.memoriaDisponible -= cantidadRequerida;
    
    colorVerde();
    printf("Memoria asignada: Proceso %d, %d bytes, dirección %d\n", 
           idProceso, cantidadRequerida, particion->inicio);
    colorReset();
    
    // Registrar evento
    registrarEvento("Memoria asignada: Proceso %d, %d bytes, dirección %d", 
                   idProceso, cantidadRequerida, particion->inicio);
    
    return true;
}

// Liberar la memoria asignada a un proceso
void liberarMemoria(int idProceso) {
    bool liberada = false;
    
    for (int i = 0; i < gestorMemoria.numParticiones; i++) {
        if (gestorMemoria.particiones[i].idProceso == idProceso) {
            // Liberar esta partición
            int tamanoLiberado = gestorMemoria.particiones[i].tamano;
            gestorMemoria.particiones[i].libre = true;
            gestorMemoria.particiones[i].idProceso = -1;
            gestorMemoria.memoriaDisponible += tamanoLiberado;
            liberada = true;
            
            colorAmarillo();
            printf("Memoria liberada: Proceso %d, %d bytes\n", idProceso, tamanoLiberado);
            colorReset();
            
            // Fusionar con particiones libres adyacentes
            consolidarParticiones();
        }
    }
    
    if (liberada) {
        // Registrar evento
        registrarEvento("Memoria liberada: Proceso %d", idProceso);
    } else {
        printf("No se encontró memoria asignada al proceso %d\n", idProceso);
    }
    
    // También liberar páginas virtuales
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

// Asignar memoria cuando un proceso entra en E/S
// Asignar memoria cuando un proceso entra en E/S
bool asignarMemoriaES(int idProceso) {
    // En lugar de intentar acceder directamente a jugadores,
    // Obtendremos la cantidad de cartas del jugador a través de la interfaz API existente
    
    // Calcular memoria requerida usando una base fija
    // sin depender directamente de la variable jugadores
    int memoriaBase = 16; // Mínimo 16 bytes
    int memoriaPorCarta = 4; // 4 bytes por carta
    
    // Vamos a obtener el número de cartas indirectamente
    // asumiendo un valor base más una cantidad aleatoria
    // para simular diferentes tamaños de solicitud de memoria
    int numCartas = 5 + (rand() % 8); // Entre 5 y 12 cartas
    int cantidadRequerida = memoriaBase + (numCartas * memoriaPorCarta);
    
    // Asignar la memoria utilizando el algoritmo actual
    bool asignado = asignarMemoria(idProceso, cantidadRequerida);
    
    // Si el proceso puede crecer y hay suficientes cartas (simuladas), intentar crecer
    if (asignado && (idProceso == gestorMemoria.creceProc1 || idProceso == gestorMemoria.creceProc2) && 
        numCartas > 5) {
        
        // Calcular crecimiento (aleatorio entre 10 y 30 bytes)
        int crecimiento = (rand() % 21) + 10;
        
        colorMagenta();
        printf("El proceso %d intentará crecer en %d bytes\n", idProceso, crecimiento);
        colorReset();
        
        // Intentar crecer
        crecerProceso(idProceso, crecimiento);
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
    printf("Memoria total: %d bytes\n", MEM_TOTAL_SIZE);
    printf("Memoria disponible: %d bytes\n", gestorMemoria.memoriaDisponible);
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
    if (nuevoAlgoritmo != ALG_AJUSTE_OPTIMO && nuevoAlgoritmo != ALG_LRU) {
        printf("Error: Algoritmo de memoria no válido\n");
        return;
    }
    
    gestorMemoria.algoritmoActual = nuevoAlgoritmo;
    
    const char *nombres[] = {"Ajuste Óptimo", "LRU (Least Recently Used)"};
    colorCian();
    printf("Algoritmo de memoria cambiado a: %s\n", nombres[nuevoAlgoritmo]);
    colorReset();
    
    // Registrar evento
    registrarEvento("Algoritmo de memoria cambiado a: %s", nombres[nuevoAlgoritmo]);
}