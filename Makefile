# Project Name
TARGET = DaisyVerb

# Paths
SRC_DIR	= src
LIBS_DIR = libs

# Sources
# CPP_SOURCES = FDN_stereo.cpp
CPP_SOURCES = $(SRC_DIR)/AllpassFilter.cpp
CPP_SOURCES += $(SRC_DIR)/VersioReverb.cpp
CPP_SOURCES += DattorroVerb.cpp

# Library Locations
LIBDAISY_DIR = $(LIBS_DIR)/libDaisy
DAISYSP_DIR = $(LIBS_DIR)/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
