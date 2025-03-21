#ifndef PROCESOS_STUB_H
#define PROCESOS_STUB_H

// Declaración de las funciones de procesos que usamos en otros archivos
// sin modificar el procesos.h original

// Struct BCP forward declaration (para evitar redefiniciones)
struct BCP;

// Funciones BCP que utilizas pero no están declaradas correctamente
struct BCP* crearBCP(int id);
void guardarBCP(struct BCP* bcp);
void liberarBCP(struct BCP* bcp);

// Funciones tabla de procesos
void inicializarTabla(void);
void registrarProcesoEnTabla(int id, int estado);  // Usamos int para evitar conflictos de tipos
void actualizarProcesoEnTabla(int id, int estado);
void imprimirEstadisticasTabla(void);
void liberarTabla(void);

#endif // PROCESOS_STUB_H