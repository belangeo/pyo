# Makefile for pyo~

lib.name = pyo~

class.sources = pyo~.c

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

cflags += $(PY_CFLAGS) -Wno-cast-function-type -I../
ldlibs += $(PY_LIBS)

datafiles = pyo-help.pd README.md

PDLIBBUILDER_DIR=../../3rdParty/pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder 
