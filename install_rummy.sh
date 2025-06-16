#!/bin/bash

# Script de instalación y configuración para el proyecto PVM Rummy
# Sistemas Operativos - Sistemas Distribuidos
# Autor: [Tu Nombre]
# Fecha: Junio 2024

set -e  # Salir si cualquier comando falla

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Función para imprimir con colores
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo -e "${BLUE}$1${NC}"
}

# Verificar si se ejecuta como usuario normal (no root)
check_user() {
    if [[ $EUID -eq 0 ]]; then
        print_error "No ejecutes este script como root. Úsalo como usuario normal."
        print_warning "El script pedirá sudo cuando sea necesario."
        exit 1
    fi
}

# Detectar distribución de Linux
detect_distro() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        DISTRO=$ID
        VERSION=$VERSION_ID
    else
        print_error "No se pudo detectar la distribución de Linux"
        exit 1
    fi
    
    print_status "Distribución detectada: $DISTRO $VERSION"
}

# Instalar dependencias
install_dependencies() {
    print_header "=== INSTALANDO DEPENDENCIAS ==="
    
    case $DISTRO in
        ubuntu|debian)
            print_status "Actualizando lista de paquetes..."
            sudo apt-get update
            
            print_status "Instalando herramientas de desarrollo..."
            sudo apt-get install -y build-essential gcc make
            
            print_status "Instalando PVM..."
            sudo apt-get install -y pvm-dev pvm libpvm3-dev
            
            # Verificar si rsh está disponible (requerido por PVM)
            if ! command -v rsh &> /dev/null; then
                print_status "Instalando rsh (requerido por PVM)..."
                sudo apt-get install -y rsh-client
            fi
            ;;
        centos|rhel|fedora)
            print_status "Instalando herramientas de desarrollo..."
            if command -v dnf &> /dev/null; then
                sudo dnf groupinstall -y "Development Tools"
                sudo dnf install -y gcc make
            else
                sudo yum groupinstall -y "Development Tools"
                sudo yum install -y gcc make
            fi
            
            print_warning "PVM debe instalarse manualmente en CentOS/RHEL/Fedora"
            print_warning "Descarga desde: http://www.csm.ornl.gov/pvm/"
            ;;
        *)
            print_error "Distribución no soportada: $DISTRO"
            print_warning "Instala manualmente: gcc, make, pvm-dev"
            ;;
    esac
}

# Verificar instalación de PVM
verify_pvm() {
    print_header "=== VERIFICANDO INSTALACIÓN DE PVM ==="
    
    # Verificar comando pvm
    if command -v pvm &> /dev/null; then
        print_status "✓ Comando 'pvm' encontrado"
    else
        print_error "✗ Comando 'pvm' no encontrado"
        return 1
    fi
    
    # Verificar headers
    PVM_HEADER_PATHS=(
        "/usr/include/pvm3.h"
        "/usr/include/pvm3/pvm3.h"
        "/usr/local/include/pvm3.h"
    )
    
    PVM_HEADER=""
    for path in "${PVM_HEADER_PATHS[@]}"; do
        if [[ -f "$path" ]]; then
            PVM_HEADER="$path"
            print_status "✓ Headers PVM encontrados en: $path"
            break
        fi
    done
    
    if [[ -z "$PVM_HEADER" ]]; then
        print_error "✗ Headers PVM no encontrados"
        return 1
    fi
    
    # Verificar librerías
    PVM_LIB_PATHS=(
        "/usr/lib/*/libpvm3.a"
        "/usr/local/lib/*/libpvm3.a"
        "/usr/lib/pvm3/lib/*/libpvm3.a"
    )
    
    PVM_LIB=""
    for path in "${PVM_LIB_PATHS[@]}"; do
        if ls $path 1> /dev/null 2>&1; then
            PVM_LIB=$(ls $path | head -1)
            print_status "✓ Librería PVM encontrada en: $PVM_LIB"
            break
        fi
    done
    
    if [[ -z "$PVM_LIB" ]]; then
        print_error "✗ Librería PVM no encontrada"
        return 1
    fi
    
    # Detectar arquitectura
    if [[ $(uname -m) == "x86_64" ]]; then
        PVM_ARCH="LINUX64"
    else
        PVM_ARCH="LINUX"
    fi
    
    print_status "Arquitectura detectada: $PVM_ARCH"
    
    return 0
}

# Configurar directorios de PVM
setup_pvm_directories() {
    print_header "=== CONFIGURANDO DIRECTORIOS PVM ==="
    
    # Crear estructura de directorios
    PVM_HOME="$HOME/pvm3"
    mkdir -p "$PVM_HOME/bin/$PVM_ARCH"
    mkdir -p "$PVM_HOME/src"
    mkdir -p "$PVM_HOME/logs"
    mkdir -p "$PVM_HOME/bcp"
    
    print_status "Directorios creados en: $PVM_HOME"
    
    # Configurar variables de entorno
    ENV_FILE="$HOME/.pvm_env"
    cat > "$ENV_FILE" << EOF
# Configuración PVM para proyecto Rummy
export PVM_ROOT=/usr/lib/pvm3
export PVM_ARCH=$PVM_ARCH
export PVM_HOME=$PVM_HOME
export PATH=\$PVM_HOME/bin/\$PVM_ARCH:\$PATH
EOF
    
    print_status "Variables de entorno configuradas en: $ENV_FILE"
    print_warning "Ejecuta 'source $ENV_FILE' para cargar las variables"
}

# Crear archivos de configuración
create_config_files() {
    print_header "=== CREANDO ARCHIVOS DE CONFIGURACIÓN ==="
    
    # Archivo de hosts para PVM
    HOSTFILE="$HOME/pvm_hosts"
    cat > "$HOSTFILE" << EOF
# Archivo de configuración de hosts para PVM
# Formato: hostname [lo=loginname] [so=soname] [dx=name] [ep=path] [wd=path] [sp=value]

# Host local (Master)
$(hostname)

# Hosts remotos (descomenta y modifica según tu red)
# slave1.local
# slave2.local
# 192.168.1.101
# 192.168.1.102
EOF
    
    print_status "Archivo de hosts creado: $HOSTFILE"
    
    # Script de inicio para PVM
    STARTUP_SCRIPT="$HOME/start_pvm.sh"
    cat > "$STARTUP_SCRIPT" << 'EOF'
#!/bin/bash
# Script para iniciar PVM con configuración del proyecto Rummy

# Cargar variables de entorno
if [[ -f "$HOME/.pvm_env" ]]; then
    source "$HOME/.pvm_env"
fi

echo "Iniciando PVM para proyecto Rummy..."
echo "Archivo de hosts: $HOME/pvm_hosts"

# Verificar si pvmd ya está ejecutándose
if pgrep pvmd > /dev/null; then
    echo "ADVERTENCIA: pvmd ya está ejecutándose"
    echo "Terminando procesos existentes..."
    pkill pvmd
    sleep 2
fi

# Iniciar PVM
if [[ -f "$HOME/pvm_hosts" ]]; then
    pvm "$HOME/pvm_hosts"
else
    pvm
fi
EOF
    
    chmod +x "$STARTUP_SCRIPT"
    print_status "Script de inicio creado: $STARTUP_SCRIPT"
}

# Compilar el proyecto
compile_project() {
    print_header "=== COMPILANDO PROYECTO ==="
    
    # Verificar que los archivos fuente existen
    if [[ ! -f "Master_Rummy.c" ]]; then
        print_error "Archivo Master_Rummy.c no encontrado"
        return 1
    fi
    
    if [[ ! -f "Slave_Rummy.c" ]]; then
        print_error "Archivo Slave_Rummy.c no encontrado"
        return 1
    fi
    
    # Cargar variables de entorno
    source "$HOME/.pvm_env"
    
    # Compilar usando Makefile si existe
    if [[ -f "Makefile" ]]; then
        print_status "Usando Makefile para compilar..."
        make clean || true
        make all
    else
        print_status "Compilando manualmente..."
        
        # Detectar rutas de PVM
        PVM_INCLUDE=""
        if [[ -f "/usr/include/pvm3.h" ]]; then
            PVM_INCLUDE="/usr/include"
        elif [[ -f "/usr/include/pvm3/pvm3.h" ]]; then
            PVM_INCLUDE="/usr/include/pvm3"
        fi
        
        PVM_LIBDIR=""
        for dir in /usr/lib/*/; do
            if [[ -f "$dir/libpvm3.a" ]]; then
                PVM_LIBDIR="$dir"
                break
            fi
        done
        
        # Compilar Master
        print_status "Compilando Master..."
        gcc -Wall -g -I"$PVM_INCLUDE" -c Master_Rummy.c -o Master_Rummy.o
        gcc -L"$PVM_LIBDIR" -o "$PVM_HOME/bin/$PVM_ARCH/MASTER_RUMMY" Master_Rummy.o -lpvm3 -lgpvm3
        
        # Compilar Slave
        print_status "Compilando Slave..."
        gcc -Wall -g -I"$PVM_INCLUDE" -c Slave_Rummy.c -o Slave_Rummy.o
        gcc -L"$PVM_LIBDIR" -o "$PVM_HOME/bin/$PVM_ARCH/SLAVE_RUMMY" Slave_Rummy.o -lpvm3 -lgpvm3
        
        # Limpiar archivos objeto
        rm -f *.o
    fi
    
    # Verificar que los ejecutables se crearon
    if [[ -f "$PVM_HOME/bin/$PVM_ARCH/MASTER_RUMMY" ]] && [[ -f "$PVM_HOME/bin/$PVM_ARCH/SLAVE_RUMMY" ]]; then
        print_status "✓ Compilación exitosa"
        print_status "Ejecutables creados en: $PVM_HOME/bin/$PVM_ARCH/"
    else
        print_error "✗ Error en la compilación"
        return 1
    fi
}

# Ejecutar tests básicos
run_tests() {
    print_header "=== EJECUTANDO TESTS BÁSICOS ==="
    
    # Test 1: Verificar que los ejecutables funcionan
    print_status "Test 1: Verificando ejecutables..."
    if ldd "$PVM_HOME/bin/$PVM_ARCH/MASTER_RUMMY" > /dev/null 2>&1; then
        print_status "✓ MASTER_RUMMY: dependencias OK"
    else
        print_error "✗ MASTER_RUMMY: problema con dependencias"
    fi
    
    if ldd "$PVM_HOME/bin/$PVM_ARCH/SLAVE_RUMMY" > /dev/null 2>&1; then
        print_status "✓ SLAVE_RUMMY: dependencias OK"
    else
        print_error "✗ SLAVE_RUMMY: problema con dependencias"
    fi
    
    # Test 2: Verificar configuración de PVM
    print_status "Test 2: Verificando configuración PVM..."
    source "$HOME/.pvm_env"
    
    if [[ -n "$PVM_ROOT" ]] && [[ -n "$PVM_ARCH" ]]; then
        print_status "✓ Variables de entorno configuradas"
    else
        print_error "✗ Variables de entorno no configuradas"
    fi
}

# Mostrar instrucciones finales
show_final_instructions() {
    print_header "=== INSTALACIÓN COMPLETADA ==="
    
    cat << EOF

🎉 ¡La instalación se completó exitosamente!

📁 Estructura de archivos creada:
   $HOME/pvm3/bin/$PVM_ARCH/    - Ejecutables
   $HOME/pvm3/logs/             - Logs del sistema
   $HOME/pvm3/bcp/              - Archivos BCP
   $HOME/pvm_hosts              - Configuración de hosts
   $HOME/start_pvm.sh           - Script de inicio
   $HOME/.pvm_env               - Variables de entorno

🚀 Para ejecutar el proyecto:

1. Cargar variables de entorno:
   source ~/.pvm_env

2. Iniciar PVM:
   $HOME/start_pvm.sh

3. En el prompt de PVM, verificar configuración:
   pvm> conf
   pvm> quit

4. Ejecutar el juego:
   cd $(pwd)
   make run
   # O manualmente:
   $HOME/pvm3/bin/$PVM_ARCH/MASTER_RUMMY

📋 Archivos que se generarán durante la ejecución:
   - BCP_Jugador_*.dat      - Bloques de Control de Proceso
   - TablaProc*.dat         - Tablas de Procesos
   - bitacoraMaster.dat     - Log del Master
   - bitacoraSlave.dat      - Log del Slave

🔧 Para depuración:
   - Logs de PVM: /tmp/pvml.[usuario]
   - Estado de PVM: ps aux | grep pvmd

⚠️  Notas importantes:
   - Asegúrate de que el firewall permita comunicación PVM
   - Para múltiples máquinas, configura SSH sin contraseña
   - Modifica $HOME/pvm_hosts para añadir hosts remotos

EOF
}

# Función principal
main() {
    print_header "======================================"
    print_header "  INSTALADOR PVM RUMMY - SISTEMAS DISTRIBUIDOS"
    print_header "======================================"
    echo
    
    check_user
    detect_distro
    
    print_status "Iniciando instalación..."
    echo
    
    # Preguntar al usuario qué componentes instalar
    echo "¿Qué deseas hacer?"
    echo "1) Instalación completa (recomendado)"
    echo "2) Solo verificar PVM existente"
    echo "3) Solo compilar proyecto"
    echo "4) Solo configurar directorios"
    echo
    read -p "Selecciona una opción (1-4): " OPTION
    
    case $OPTION in
        1)
            install_dependencies
            verify_pvm || { print_error "PVM no está correctamente instalado"; exit 1; }
            setup_pvm_directories
            create_config_files
            compile_project
            run_tests
            show_final_instructions
            ;;
        2)
            verify_pvm || { print_error "PVM no está correctamente instalado"; exit 1; }
            print_status "✓ PVM está correctamente instalado"
            ;;
        3)
            setup_pvm_directories
            compile_project
            ;;
        4)
            setup_pvm_directories
            create_config_files
            ;;
        *)
            print_error "Opción inválida"
            exit 1
            ;;
    esac
    
    print_status "¡Proceso completado!"
}

# Manejar interrupciones
trap 'print_error "Instalación interrumpida"; exit 1' INT TERM

# Ejecutar función principal
main "$@"