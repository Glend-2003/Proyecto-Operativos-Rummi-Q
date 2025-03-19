# Nombre del ejecutable
TARGET = rummi

# Archivos fuente
SRCS = main.c jugador.c juego.c mesa.c bcp.c tabla_procesos.c

# Archivos objeto (compilados)
OBJS = $(SRCS:.c=.o)

# Compilador y opciones
CC = gcc
CFLAGS = -Wall -pthread -g  # -Wall muestra advertencias, -pthread para hilos, -g para depuración

# Regla principal para compilar el ejecutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regla para compilar cada archivo .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos compilados
clean:
	rm -f $(OBJS) $(TARGET)
