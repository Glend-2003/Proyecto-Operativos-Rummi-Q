# Makefile para el proyecto PVM Rummy
# Sistemas Operativos - Sistemas Distribuidos

# Configuración del compilador y flags
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -D_GNU_SOURCE
PVM_ROOT = /usr/lib/pvm3
PVM_ARCH = LINUX64

# Directorios
SRCDIR = .
BINDIR = $(HOME)/pvm3/bin/$(PVM_ARCH)
OBJDIR = obj

# Archivos fuente
MASTER_SRC = Master_Rummy.c
SLAVE_SRC = Slave_Rummy.c

# Archivos objeto
MASTER_OBJ = $(OBJDIR)/Master_Rummy.o
SLAVE_OBJ = $(OBJDIR)/Slave_Rummy.o

# Ejecutables
MASTER_EXE = $(BINDIR)/MASTER_RUMMY
SLAVE_EXE = $(BINDIR)/SLAVE_RUMMY

# Librerías y flags de PVM
PVM_INCDIR = $(PVM_ROOT)/include
PVM_LIBDIR = $(PVM_ROOT)/lib/$(PVM_ARCH)
LDLIBS = -lpvm3 -lgpvm3
LDFLAGS = -L$(PVM_LIBDIR)
INCLUDES = -I$(PVM_INCDIR)

# Regla por defecto
all: setup $(MASTER_EXE) $(SLAVE_EXE)

# Crear directorios necesarios
setup:
	@echo "Configurando directorios para PVM..."
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)
	@mkdir -p logs
	@mkdir -p bcp
	@echo "Directorios creados correctamente."

# Compilar Master
$(MASTER_EXE): $(MASTER_OBJ)
	@echo "Enlazando Master..."
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)
	@echo "Master compilado: $@"

# Compilar Slave
$(SLAVE_EXE): $(SLAVE_OBJ)
	@echo "Enlazando Slave..."
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)
	@echo "Slave compilado: $@"

# Compilar objeto del Master
$(MASTER_OBJ): $(MASTER_SRC)
	@echo "Compilando Master..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compilar objeto del Slave
$(SLAVE_OBJ): $(SLAVE_SRC)
	@echo "Compilando Slave..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Limpiar archivos compilados
clean:
	@echo "Limpiando archivos temporales..."
	rm -rf $(OBJDIR)
	rm -f $(MASTER_EXE) $(SLAVE_EXE)
	rm -f *.o
	@echo "Limpieza completada."

# Limpiar logs y datos del juego
clean-logs:
	@echo "Limpiando logs y datos del juego..."
	rm -f *.dat *.log
	rm -f logs/*
	rm -f bcp/*
	@echo "Logs limpiados."

# Limpiar todo
distclean: clean clean-logs
	@echo "Limpieza completa realizada."

# Ejecutar el juego (solo Master, los Slaves son lanzados automáticamente)
run: $(MASTER_EXE)
	@echo "Iniciando juego Rummy distribuido..."
	@echo "Asegúrate de que PVM esté funcionando (ejecuta 'pvm' antes)"
	cd $(SRCDIR) && $(MASTER_EXE)

# Verificar configuración de PVM
check-pvm:
	@echo "Verificando configuración de PVM..."
	@echo "PVM_ROOT: $(PVM_ROOT)"
	@echo "PVM_ARCH: $(PVM_ARCH)"
	@echo "Directorio de includes: $(PVM_INCDIR)"
	@echo "Directorio de librerías: $(PVM_LIBDIR)"
	@echo "Directorio de binarios: $(BINDIR)"
	@if [ -d "$(PVM_ROOT)" ]; then \
		echo "✓ PVM_ROOT existe"; \
	else \
		echo "✗ PVM_ROOT no encontrado"; \
	fi
	@if [ -f "$(PVM_INCDIR)/pvm3.h" ]; then \
		echo "✓ Archivo pvm3.h encontrado"; \
	else \
		echo "✗ Archivo pvm3.h no encontrado"; \
	fi
	@if [ -f "$(PVM_LIBDIR)/libpvm3.a" ]; then \
		echo "✓ Librería libpvm3.a encontrada"; \
	else \
		echo "✗ Librería libpvm3.a no encontrada"; \
	fi

# Instalar PVM (requiere permisos de administrador)
install-pvm:
	@echo "Para instalar PVM 3.4.6, ejecuta los siguientes comandos:"
	@echo "sudo apt-get update"
	@echo "sudo apt-get install pvm-dev"
	@echo "O descarga desde: http://www.csm.ornl.gov/pvm/"

# Mostrar ayuda
help:
	@echo "Makefile para el proyecto PVM Rummy"
	@echo ""
	@echo "Targets disponibles:"
	@echo "  all         - Compilar Master y Slave"
	@echo "  setup       - Crear directorios necesarios"
	@echo "  clean       - Limpiar archivos compilados"
	@echo "  clean-logs  - Limpiar logs y datos del juego"
	@echo "  distclean   - Limpieza completa"
	@echo "  run         - Ejecutar el juego"
	@echo "  check-pvm   - Verificar configuración de PVM"
	@echo "  install-pvm - Mostrar instrucciones de instalación de PVM"
	@echo "  help        - Mostrar esta ayuda"
	@echo ""
	@echo "Antes de ejecutar, asegúrate de:"
	@echo "1. Tener PVM 3.4.6 instalado"
	@echo "2. Ejecutar 'pvm' para iniciar el daemon"
	@echo "3. Configurar las máquinas en el archivo hostfile si usas múltiples hosts"

# Debug: compilar con información de depuración adicional
debug: CFLAGS += -DDEBUG -O0
debug: all

# Release: compilar optimizado
release: CFLAGS += -O2 -DNDEBUG
release: all

.PHONY: all setup clean clean-logs distclean run check-pvm install-pvm help debug release