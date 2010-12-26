# only P-ROC specific output files and rules
#
PROCOBJS =
OBJDIRS += $(OBJ)/p-roc

ifdef PROC

DEFS += -DPROC_SUPPORT

PROCOBJS += $(OBJ)/p-roc/p-roc.o
PROCOBJS += $(OBJ)/p-roc/display.o
PROCOBJS += $(OBJ)/p-roc/gameitems.o

# P-ROC functions
$(OBJ)/p-roc/%.o: src/p-roc/%.cpp
	@echo Compiling $<...
	$(CPP) $(CDEFS) $(CPPFLAGSPEDANTIC) -c $< -o $@

# add P-ROC folders for external (library, etc.) and internal includes
CFLAGS += -Ip-roc/includes -Isrc/p-roc
CPPFLAGS += -Ip-roc/includes -Isrc/p-roc

# add libraries for P-ROC
LDFLAGS += -Lp-roc/gcc
LIBS += -llibpinproc -llibyaml-cpp.dll -lftd2xx.lib

endif
