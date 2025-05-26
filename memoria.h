#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdbool.h>
#include "procesos.h"

// ============== CONSTANTES ==============
#define MEM_TOTAL_SIZE       1024
#define MAX_PARTICIONES      50
#define NUM_MARCOS           6
#define PAGINAS_POR_MARCO    4
#define MAX_PAGINAS          100
#define TAMANO_BLOQUE_BITMAP 16
#define NUM_BLOQUES_BITMAP   (MEM_TOTAL_SIZE / TAMANO_BLOQUE_BITMAP)

#define ALG_AJUSTE_OPTIMO    0
#define ALG_LRU              1
#define ALG_MAPA_BITS        2

// ============== ESTRUCTURAS ==============
typedef struct {
    int inicio;
    int tamano;
    int idProceso;
    bool libre;
} Particion;

typedef struct {
    int idProceso;
    int numPagina;
    int idCarta;
    int tiempoUltimoUso;
    bool bitReferencia;
    bool bitModificacion;
    bool enMemoria;
    int marcoAsignado;
} Pagina;

typedef struct {
    int idProceso;
    int numPagina;
    bool libre;
} Marco;

typedef struct {
    // Ajuste óptimo
    Particion particiones[MAX_PARTICIONES];
    int numParticiones;
    int memoriaDisponible;
    int algoritmoActual;
    
    // Memoria virtual LRU
    Pagina tablaPaginas[MAX_PAGINAS];
    int numPaginas;
    Marco marcosMemoria[NUM_MARCOS];
    int contadorTiempo;
    int fallosPagina;
    int aciertosMemoria;
    
    // Mapa de bits
    unsigned char mapaBits[NUM_BLOQUES_BITMAP];
    int tamanoBloque;
    
    // Crecimiento de procesos
    int creceProc1;
    int creceProc2;
} GestorMemoria;

// ============== INICIALIZACIÓN ==============
void inicializarMemoria(void);
void inicializarMemoriaVirtual(void);

// ============== GESTIÓN BÁSICA ==============
bool asignarMemoria(int idProceso, int cantidadRequerida);
void liberarMemoria(int idProceso);
bool crecerProceso(int idProceso, int cantidadAdicional);
bool asignarMemoriaES(int idProceso);

// ============== MEMORIA VIRTUAL ==============
int accederPagina(int idProceso, int numPagina, int idCarta);
void liberarPaginasProceso(int idProceso);
Pagina* buscarPaginaLibre(void);
int seleccionarVictimaLRU(void);

// ============== ALGORITMOS ==============
void cambiarAlgoritmoMemoria(int nuevoAlgoritmo);

// ============== DIAGNÓSTICO ==============
void imprimirEstadoMemoria(void);
void imprimirEstadoMemoriaVirtual(void);

// ============== VARIABLE GLOBAL ==============
extern GestorMemoria gestorMemoria;

#endif /* MEMORIA_H */