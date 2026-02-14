#!/bin/bash

# Default values
USE_PYENV=true
SKIP_CLONE=false
PYTHON_VERSION="3" # Default to Python 3, specific version can be passed
PYENV_PYTHON_DEFAULT_VERSION="3.13.12" # A sensible default for pyenv if not specified

usage() {
    echo "Usage: $0 [-h|--help] [-p|--pyenv] [-o|--os-python] [-s|--skip-clone] [-v <version>]"
    echo "  -h, --help            Display this help message"
    echo "  -p, --pyenv           Use pyenv installed Python (default)"
    echo "  -o, --os-python       Use OS-supplied Python"
    echo "  -s, --skip-clone      Skip git clone if 'pyo' directory exists"
    echo "  -v <version>          Specify Python version (e.g., 3.13). For pyenv, this selects the pyenv version. For OS python, this selects the major.minor version."
    exit 0
}

# Parse command line arguments
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -h|--help) usage ;;
        -p|--pyenv) USE_PYENV=true ;;
        -o|--os-python) USE_PYENV=false ;;
        -s|--skip-clone) SKIP_CLONE=true ;;
        -v)
            if [ -n "$2" ]; then
                PYTHON_VERSION="$2"
                shift
            else
                echo "Error: -v requires a version number (e.g., 3.13)."
                usage
            fi
            ;;
        *) echo "Unknown parameter: $1"; usage ;;
    esac
    shift
done

# --- Python Environment Setup ---
PYTHONBIN=""
PYLIBPREFIX=""
PREFIX=""

if [ "$USE_PYENV" = true ]; then
    echo "Configuring to use pyenv installed Python..."
    if ! command -v pyenv &> /dev/null; then
        echo "Error: pyenv is not installed or not in PATH. Please install pyenv or use -o for OS Python."
        exit 1
    fi

    # If a specific Python version was requested, use it, otherwise use the default pyenv version
    if [ "$PYTHON_VERSION" = "3" ]; then
        PYTHON_VERSION="$PYENV_PYTHON_DEFAULT_VERSION"
    fi

    PREFIX="$HOME/.pyenv/versions/$PYTHON_VERSION"
    PYTHONBIN="$PREFIX/bin/python" # pyenv python executable is usually just 'python'
    PYLIBPREFIX="$PREFIX/lib/python$(echo "$PYTHON_VERSION" | cut -d'.' -f1,2)/site-packages"

    # Verify pyenv installation exists
    if [ ! -d "$PREFIX" ]; then
        echo "Error: pyenv Python version $PYTHON_VERSION not found at $PREFIX. Please install it with 'pyenv install $PYTHON_VERSION'."
        exit 1
    fi
else
    echo "Configuring to use OS-supplied Python..."
    PREFIX="/usr" # Default OS install prefix
    # Try to find a suitable python executable based on PYTHON_VERSION
    if command -v python"$PYTHON_VERSION" &> /dev/null; then
        PYTHONBIN="/usr/bin/python$PYTHON_VERSION"
    elif command -v python3 &> /dev/null && [[ "$PYTHON_VERSION" == "3"* ]]; then
        PYTHONBIN="/usr/bin/python3"
        PYTHON_VERSION="3" # Adjust version to match found executable
        echo "Warning: Specific OS Python version $PYTHON_VERSION not found, defaulting to python3."
    elif command -v python &> /dev/null; then
        PYTHONBIN="/usr/bin/python"
        PYTHON_VERSION="2" # Assume python refers to python2 if python3 not found
        echo "Warning: No Python 3 found, defaulting to system 'python' (likely Python 2). This may cause issues."
    else
        echo "Error: No suitable Python executable found on the system for version $PYTHON_VERSION."
        exit 1
    fi

    # Determine site-packages or dist-packages path
    if [ -d "/usr/lib/python$(echo "$PYTHON_VERSION" | cut -d'.' -f1,2)/site-packages" ]; then
        PYLIBPREFIX="/usr/lib/python$(echo "$PYTHON_VERSION" | cut -d'.' -f1,2)/site-packages"
    elif [ -d "/usr/local/lib/python$(echo "$PYTHON_VERSION" | cut -d'.' -f1,2)/dist-packages" ]; then # Debian/Ubuntu
        PYLIBPREFIX="/usr/local/lib/python$(echo "$PYTHON_VERSION" | cut -d'.' -f1,2)/dist-packages"
    elif [ -d "/usr/lib/python$(echo "$PYTHON_VERSION" | cut -d'.' -f1,2)/dist-packages" ]; then # Debian/Ubuntu
        PYLIBPREFIX="/usr/lib/python$(echo "$PYTHON_VERSION" | cut -d'.' -f1,2)/dist-packages"
    else
        echo "Error: Could not determine Python site-packages or dist-packages path for OS Python $PYTHON_VERSION."
        exit 1
    fi
fi

if [ -z "$PYTHONBIN" ] || [ ! -x "$PYTHONBIN" ]; then
    echo "Error: Python executable not found or not executable: $PYTHONBIN"
    exit 1
fi

echo "Using Python executable: $PYTHONBIN"
echo "Pyo will be installed to: $PYLIBPREFIX"

############################
# Clean out previous build #
############################
echo "Cleaning out previous pyo build files..."
sudo rm -rf "$PYLIBPREFIX"/pyo*
sudo rm -rf "$PYLIBPREFIX"/_pyo*
sudo rm -rf "$PYLIBPREFIX"/pyolib

################
# DEPENDENCIES #
################
INSTALLER=""
DEV_TAG=""

echo "Detecting OS and setting package manager..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    INSTALLER="brew install"
    DEV_TAG="" # Homebrew packages usually don't have separate -dev packages for headers
    echo "Detected macOS. Using Homebrew."
elif [ -f "/etc/os-release" ]; then
    . /etc/os-release
    if [[ "$ID" == "void" ]]; then
        INSTALLER="sudo xbps-install -y"
        DEV_TAG="devel"
        echo "Detected Void Linux. Using xbps-install."
    elif [[ "$ID" == "debian" ]] || [[ "$ID_LIKE" == "debian" ]]; then
        INSTALLER="sudo apt install -y"
        DEV_TAG="-dev"
        echo "Detected Debian-based Linux. Using apt install."
    else
        echo "Warning: Unrecognized Linux distribution. Attempting apt install as a fallback. Manual intervention may be required."
        INSTALLER="sudo apt install -y"
        DEV_TAG="-dev"
    fi
else
    echo "Warning: Unrecognized operating system. Attempting apt install as a fallback. Manual intervention may be required."
    INSTALLER="sudo apt install -y"
    DEV_TAG="-dev"
fi

if [ -z "$INSTALLER" ]; then
    echo "Error: Could not determine a package installer for your system."
    exit 1
fi

echo "Installing dependencies..."
# Common dependencies for pyo
$INSTALLER jackd$DEV_TAG \
           portaudio$DEV_TAG \
           portmidi$DEV_TAG \
           liblo$DEV_TAG \
           libsndfile$DEV_TAG \
           libsndfile1$DEV_TAG # Sometimes needed for sndfile on Debian-based

# Python development headers might be needed for some setups if not covered by python install
if [[ "$ID" == "debian" ]] || [[ "$ID_LIKE" == "debian" ]]; then
    $INSTALLER python3$PYTHON_VERSION-dev
fi


###################
# Clone and build #
###################
PYO_DIR="pyo" # The directory name for the pyo repository

if [ "$SKIP_CLONE" = true ]; then
    echo "Skip clone option enabled."
    if [ -d "$PYO_DIR" ]; then
        echo "'$PYO_DIR' directory already exists. Skipping git clone."
    else
        echo "'$PYO_DIR' directory does not exist, but skip clone was requested. Exiting."
        exit 1
    fi
else
    if [ -d "$PYO_DIR" ]; then
        echo "'$PYO_DIR' directory already exists. Skipping git clone. To force a fresh clone, remove the directory first."
    else
        echo "Cloning pyo repository..."
        git clone https://github.com/belangeo/pyo.git
        if [ $? -ne 0 ]; then
            echo "Error: git clone failed. Exiting."
            exit 1
        fi
    fi
fi

cd "$PYO_DIR" || { echo "Error: Failed to change directory to '$PYO_DIR'."; exit 1; }

echo "Building and installing pyo..."
sudo "$PYTHONBIN" setup.py clean
sudo "$PYTHONBIN" setup.py install --use-jack --use-double

echo "Pyo installation complete."
