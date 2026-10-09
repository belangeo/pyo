# Makefile for pyo~

lib.name = pyo~

class.sources = pyo~.c

ifeq ($(OS),Windows_NT)
ifeq ($(origin PYTHON_DIR),undefined)
ifeq ($(origin PYTHON_EXECUTABLE),undefined)
PYTHON_EXECUTABLE := $(shell python find_python.py)
endif
ifeq ($(strip $(PYTHON_EXECUTABLE)),)
$(error Put a native Python with pyo on PATH, or set PYTHON_EXECUTABLE or PYTHON_DIR)
endif
PYTHON_DIR := $(shell "$(PYTHON_EXECUTABLE)" -c "import sys; print(sys.base_prefix)")
ifeq ($(strip $(PYTHON_DIR)),)
$(error Unable to determine the selected Python installation)
endif
endif
endif

WINDOWS_PYTHON_DIR :=
ifeq ($(OS),Windows_NT)
WINDOWS_PYTHON_DIR := $(strip $(PYTHON_DIR))
endif

ifneq ($(WINDOWS_PYTHON_DIR),)
override PYTHON_DIR := $(subst \,/,$(PYTHON_DIR))
PYTHON_EXECUTABLE ?= $(PYTHON_DIR)/python.exe
PYTHON_LIB ?= $(shell "$(PYTHON_EXECUTABLE)" -c "import sys, sysconfig; print('python%d%d%s' % (*sys.version_info[:2], '_d' if sysconfig.get_config_var('Py_DEBUG') else ''))")
ifeq ($(strip $(PYTHON_LIB)),)
$(error Unable to determine the selected Python embedding library)
endif
$(info ++++ info: using Python $(PYTHON_EXECUTABLE))
PY_CFLAGS := -I"$(PYTHON_DIR)/include" -DPYO_PYTHON_HOME='"$(PYTHON_DIR)"'
PY_LIBS := "$(PYTHON_DIR)/libs/$(PYTHON_LIB).lib"
else
PKG_CONFIG := pkg-config
# `make --debug` enables the debug Python runtime when available.
PYTHON_DEBUG := $(filter --debug%,$(MAKEFLAGS))
PYTHON := $(shell if [ -n "$(PYTHON_DEBUG)" ]; then \
    $(PKG_CONFIG) --exists python3d-embed && echo python3d-embed || echo python3d; \
else \
    $(PKG_CONFIG) --exists python3-embed && echo python3-embed || echo python3; \
fi)

PY_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PYTHON))
PY_LIBS   := $(shell $(PKG_CONFIG) --libs $(PYTHON))
endif

cflags += $(PY_CFLAGS) -pthread -Wno-cast-function-type -I../
ldlibs += $(PY_LIBS) -pthread

datafiles = pyo-help.pd README.md
ifneq ($(WINDOWS_PYTHON_DIR),)
datafiles += $(PYTHON_LIB).dll
endif

PDLIBBUILDER_DIR=../../3rdParty/pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder 

ifneq ($(WINDOWS_PYTHON_DIR),)
# Pd needs the matching Python DLL before it can load the external.
.PHONY: python-runtime
all: python-runtime
python-runtime:
	@if ! cmp -s "$(PYTHON_DIR)/$(PYTHON_LIB).dll" "$(PYTHON_LIB).dll"; then \
	    cp "$(PYTHON_DIR)/$(PYTHON_LIB).dll" "$(PYTHON_LIB).dll"; \
	fi
endif
