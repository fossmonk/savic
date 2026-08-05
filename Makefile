# Detect the Operating System
# On Windows, the OS environment variable is typically set to "Windows_NT"
ifeq ($(OS),Windows_NT)
    INC_PATH=-I$(USERPROFILE)\raylib\include -I. -Isrc
	LIB_PATH=-L $(USERPROFILE)\raylib\lib
	LIBS=-lm -lraylib -lgdi32 -lwinmm
else
	THIS_DIR=$(realpath .)
	THIRD_PARTY=$(HOME)/projects/third-party
    # On Unix-like systems, we query 'uname'
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
		INC_PATH=-I$(THIRD_PARTY)/raylib/src -I$(THIS_DIR) -Isrc
		LIB_PATH=-L$(THIRD_PARTY)/raylib/src
		LIBS=-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL -lraylib
    else
		INC_PATH=-I$(THIRD_PARTY)/raylib/src -I$(THIS_DIR) -Isrc
		LIB_PATH=-L$(THIRD_PARTY)/raylib/src
		LIBS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
endif

SRCS := $(wildcard *.c)
C_OPTS=-Wall -Wextra
APP=saviz

all:
	@echo "Building SAVIZ..."
	@cc -o $(APP) $(SRCS) $(INC_PATH) $(LIB_PATH) $(LIBS) $(C_OPTS)
	@echo "Build Successful."

debug:
	@cc -o $(APP)_debug $(SRCS) $(INC_PATH) $(LIB_PATH) $(LIBS) $(C_OPTS) -O0 -ggdb