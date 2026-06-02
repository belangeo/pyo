# Makefile for pyo~

lib.name = pyo~

class.sources = pyo~.c

PKG_CONFIG := pkg-config
# Try to use python3-embed if available (standard on modern Linux), fall back to python3
PYTHON := $(shell $(PKG_CONFIG) --exists python3-embed && echo python3-embed || echo python3)

PY_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PYTHON))
PY_LIBS   := $(shell $(PKG_CONFIG) --libs $(PYTHON))

cflags += $(PY_CFLAGS) -Wno-cast-function-type -I../
ldlibs += $(PY_LIBS)

datafiles = pyo-help.pd README.md

PDLIBBUILDER_DIR=../../3rdParty/pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder 
