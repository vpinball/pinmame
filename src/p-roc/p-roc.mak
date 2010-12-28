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
	$(CPP) $(CDEFS) $(CPPFLAGS) -c $< -o $@

# add libraries for P-ROC
#LDFLAGS += -LFTDI/lib
LIBS += -lpinproc -lyaml-cpp.dll
# -lftd2xx

endif
