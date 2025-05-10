#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdbool.h>
#include "procesos.h" // Asegúrate que procesos.h esté disponible y correcto

// Definición de constantes para memoria
#define MEM_TOTAL_SIZE     1024    // Tamaño total de la memoria (1 KB)
#define MAX_PARTICIONES    50      // Máximo número de particiones
#define NUM_MARCOS         6       // Número de marcos en memoria principal para LRU
#define PAGINAS_POR_MARCO  4       // Número de páginas por marco (según tu definición)
#define MAX_PAGINAS        100     // Máximo número de páginas totales para LRU

// Constantes para Mapa de Bits
#define TAMANO_BLOQUE_BITMAP 16    // Define el tamaño de los bloques en bytes
#define NUM_BLOQUES_BITMAP (MEM_TOTAL_SIZE / TAMANO_BLOQUE_BITMAP) // Total de bloques

// Algoritmos de gestión de memoria (usados en gestorMemoria.algoritmoActual)
#define ALG_AJUSTE_OPTIMO  0
#define ALG_LRU            1 // Usado para la parte de memoria virtual
#define ALG_MAPA_BITS      2 // Para particionamiento con Mapa de Bits

// Estructura para una partición de memoria (para Ajuste Óptimo)
typedef struct {
    int inicio;
    int tamano;
    int idProceso;
    bool libre;
} Particion;

// Estructura para una página de memoria (para LRU)
typedef struct {
    int idProceso;
    int numPagina;
    int idCarta;        // ID de la carta/ficha almacenada
    int tiempoUltimoUso;
    bool bitReferencia;
    bool bitModificacion;
    bool enMemoria;
    int marcoAsignado;
} Pagina;

// Estructura para un marco de página en memoria principal (para LRU)
typedef struct {
    int idProceso;
    int numPagina; // Página que ocupa este marco
    bool libre;
} Marco;

// Estructura principal para gestión de memoria
typedef struct {
    // Para Ajuste Óptimo
    Particion particiones[MAX_PARTICIONES];
    int numParticiones;

    // Para Mapa de Bits
    // Usaremos 'signed char' para poder almacenar -1 para libre, o el ID del proceso (0 a MAX_JUGADORES-1)
    signed char mapaBits[NUM_BLOQUES_BITMAP];
    int tamanoBloque; // Debe ser TAMANO_BLOQUE_BITMAP

    // Común para particionamiento
    int memoriaDisponible;
    int algoritmoActual; // Puede ser ALG_AJUSTE_OPTIMO o ALG_MAPA_BITS (o ALG_LRU si lo usas para cambiar modo)

    // Para memoria virtual con LRU
    Pagina tablaPaginas[MAX_PAGINAS];
    int numPaginas;
    Marco marcosMemoria[NUM_MARCOS];
    int contadorTiempo;
    int fallosPagina;
    int aciertosMemoria;

    // Para crecimiento de procesos
    int creceProc1;
    int creceProc2;
} GestorMemoria;

// Variable global para el gestor de memoria
extern GestorMemoria gestorMemoria; // Declaración externa

// Funciones para gestión de memoria (particionamiento y general)
void inicializarMemoria(void);
bool asignarMemoria(int idProceso, int cantidadRequerida); // Usada por ambos algoritmos de partición
void liberarMemoria(int idProceso);                     // Usada por ambos algoritmos de partición
bool crecerProceso(int idProceso, int cantidadAdicional);   // Debe manejar ambos algoritmos de partición
void imprimirEstadoMemoria(void);                       // Muestra estado de particiones
void cambiarAlgoritmoMemoria(int nuevoAlgoritmo);       // Cambia entre AJUSTE_OPTIMO, MAPA_BITS, y LRU

// Funciones específicas para memoria virtual con LRU
void inicializarMemoriaVirtual(void);
int accederPagina(int idProceso, int numPagina, int idCarta);
void liberarPaginasProceso(int idProceso); // Libera páginas virtuales de un proceso
// Pagina* buscarPaginaLibre(void); // No parece ser usada, puedes quitarla si es así
int seleccionarVictimaLRU(void);
void imprimirEstadoMemoriaVirtual(void);

// Función para simular asignación de memoria para E/S
bool asignarMemoriaES(int idProceso);

#endif /* MEMORIA_H */