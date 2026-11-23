##############################################################################
# Configuration for Makefile
#

PROJECT := constellation
PROJECT_TYPE := genericfx

##############################################################################
# Sources
#

# C sources 
UCSRC = header.c

# C++ sources 
UCXXSRC = unit.cc

# List ASM source files here
UASMSRC = 

UASMXSRC = 

##############################################################################
# Include Paths
#

UINCDIR  = ../hfh-core

##############################################################################
# Library Paths
#

ULIBDIR = 

##############################################################################
# Libraries
#

ULIBS  = -lm

##############################################################################
# Macros
#

UDEFS = -fno-math-errno

