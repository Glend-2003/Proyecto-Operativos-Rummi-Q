#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdbool.h>
#include "procesos.h"

// Definición de constantes para memoria
#define MEM_TOTAL_SIZE     1024    // Tamaño total de la memoria (1 KB)
#define MAX_PARTICIONES    50      // Máximo número de particiones
#define NUM_MARCOS         6       // Número de marcos en memoria principal
#define PAGINAS_POR_MARCO  4       // Número de páginas por marco
#define MAX_PAGINAS        100     // Máximo número de páginas totales

// Algoritmos de asignación de memoria
#define ALG_AJUSTE_OPTIMO  0
#define ALG_LRU            1

// Estructura para una partición de memoria en el algoritmo de Ajuste Óptimo
typedef struct {
    int inicio;         // Dirección de inicio de la partición
    int tamano;         // Tamaño de la partición
    int idProceso;      // ID del proceso que está utilizando esta partición (-1 si está libre)
    bool libre;         // Indica si la partición está libre
} Particion;

// Estructura para una página de memoria
typedef struct {
    int idProceso;      // ID del proceso al que pertenece
    int numPagina;      // Número de página dentro del proceso
    int idCarta;        // ID de la carta almacenada en esta página (-1 si no hay carta)
    int tiempoUltimoUso; // Tiempo del último uso (para LRU)
    bool bitReferencia;  // Bit de referencia (para algoritmos que lo requieran)
    bool bitModificacion; // Bit de modificación (para algoritmos que lo requieran)
    bool enMemoria;      // Indica si la página está en memoria principal
    int marcoAsignado;   // Marco asignado en memoria principal (-1 si no está en memoria)
} Pagina;

// Estructura para un marco de página en memoria principal
typedef struct {
    int idProceso;      // ID del proceso que está utilizando este marco (-1 si está libre)
    int numPagina;      // Número de página que está en este marco (-1 si está libre)
    bool libre;         // Indica si el marco está libre
} Marco;

// Estructura principal para gestión de memoria
typedef struct {
    // Para ajuste óptimo
    Particion particiones[MAX_PARTICIONES];
    int numParticiones;
    int memoriaDisponible;
    int algoritmoActual;
    
    // Para memoria virtual con LRU
    Pagina tablaPaginas[MAX_PAGINAS];
    int numPaginas;
    Marco marcosMemoria[NUM_MARCOS];
    int contadorTiempo;    // Contador global para el algoritmo LRU
    int fallosPagina;      // Contador de fallos de página
    int aciertosMemoria;   // Contador de aciertos de memoria
    
    // Contador de crecimiento de procesos
    int creceProc1;        // ID del primer proceso que puede crecer
    int creceProc2;        // ID del segundo proceso que puede crecer
} GestorMemoria;

// Funciones para gestión de memoria con ajuste óptimo
void inicializarMemoria(void);
bool asignarMemoria(int idProceso, int cantidadRequerida);
void liberarMemoria(int idProceso);
bool crecerProceso(int idProceso, int cantidadAdicional);
void imprimirEstadoMemoria(void);

// Funciones para memoria virtual con LRU
void inicializarMemoriaVirtual(void);
int accederPagina(int idProceso, int numPagina, int idCarta);
void liberarPaginasProceso(int idProceso);
Pagina* buscarPaginaLibre(void);
int seleccionarVictimaLRU(void);
void imprimirEstadoMemoriaVirtual(void);

// Funciones para cambio de algoritmo
void cambiarAlgoritmoMemoria(int nuevoAlgoritmo);

// Función para asignar memoria cuando un proceso entra en E/S
bool asignarMemoriaES(int idProceso);

// Variable global para el gestor de memoria
extern GestorMemoria gestorMemoria;

#endif /* MEMORIA_H */