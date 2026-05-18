# Makefile for pyo~

lib.name = pyo~

class.sources = pyo~.c

PKG_CONFIG := pkg-config
PYTHON=python3

PY_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PYTHON))
PY_LIBS   := $(shell $(PKG_CONFIG) --libs $(PYTHON))

cflags += $(PY_CFLAGS) -Wno-cast-function-type -I../
ldlibs += $(PY_LIBS)

datafiles = pyo-help.pd README.md

PDLIBBUILDER_DIR=../pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder 
