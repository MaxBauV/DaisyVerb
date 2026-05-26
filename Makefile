# Project Name
TARGET = DaisyVerb

# Sources
# CPP_SOURCES = FDN_stereo.cpp
CPP_SOURCES = DattorroVerb.cpp

# Library Locations
LIBDAISY_DIR = libs/libDaisy
DAISYSP_DIR = libs/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
