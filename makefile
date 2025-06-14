# Makefile para el proyecto RummiQ con PVM
CC = gcc
CFLAGS = -Wall -pthread -I$(PVM_ROOT)/include
LDFLAGS = -L$(PVM_ROOT)/lib/$(PVM_ARCH) -lpvm3 -pthread

# Archivos objeto para el juego base
OBJS = jugadores.o mesa.o procesos.o utilidades.o memoria.o juego.o

# Targets principales
all: juego master slave

# Compilar el juego principal (sin PVM)
juego: main.o $(OBJS)
	$(CC) -o juego main.o $(OBJS) $(LDFLAGS)

# Compilar el master PVM
master: master.o $(OBJS)
	$(CC) -o master master.o $(OBJS) $(LDFLAGS)

# Compilar el slave PVM
slave: slave.o procesos.o utilidades.o
	$(CC) -o slave slave.o procesos.o utilidades.o $(LDFLAGS)

# Reglas para compilar archivos objeto
main.o: main.c juego.h jugadores.h mesa.h procesos.h utilidades.h memoria.h
	$(CC) $(CFLAGS) -c main.c

master.o: master.c juego.h jugadores.h procesos.h
	$(CC) $(CFLAGS) -c master.c

slave.o: slave.c procesos.h
	$(CC) $(CFLAGS) -c slave.c

juego.o: juego.c juego.h jugadores.h mesa.h procesos.h utilidades.h memoria.h
	$(CC) $(CFLAGS) -c juego.c

jugadores.o: jugadores.c jugadores.h mesa.h procesos.h utilidades.h memoria.h
	$(CC) $(CFLAGS) -c jugadores.c

mesa.o: mesa.c mesa.h jugadores.h
	$(CC) $(CFLAGS) -c mesa.c

procesos.o: procesos.c procesos.h utilidades.h
	$(CC) $(CFLAGS) -c procesos.c

utilidades.o: utilidades.c utilidades.h jugadores.h juego.h
	$(CC) $(CFLAGS) -c utilidades.c

memoria.o: memoria.c memoria.h procesos.h jugadores.h utilidades.h juego.h
	$(CC) $(CFLAGS) -c memoria.c

# Limpiar archivos compilados
clean:
	rm -f *.o juego master slave
	rm -f bcp/*.txt
	rm -f *.log *.txt *.dat
	rm -rf bcp

# Crear directorios necesarios
dirs:
	mkdir -p bcp

# Ejecutar con PVM
run-pvm: master slave
	@echo "Asegúrate de que PVM esté iniciado (pvmd3)"
	@echo "Luego ejecuta: ./master"

# Ayuda
help:
	@echo "Targets disponibles:"
	@echo "  make all      - Compila todo (juego, master y slave)"
	@echo "  make juego    - Compila solo el juego (sin PVM)"
	@echo "  make master   - Compila el programa master PVM"
	@echo "  make slave    - Compila el programa slave PVM"
	@echo "  make clean    - Limpia archivos compilados"
	@echo "  make dirs     - Crea directorios necesarios"
	@echo "  make run-pvm  - Instrucciones para ejecutar con PVM"

.PHONY: all clean dirs run-pvm help