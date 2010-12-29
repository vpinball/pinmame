# only P-ROC specific output files and rules
#
OBJDIRS += $(OBJ)/p-roc

ifdef PROC

# add precompiler defines for P-ROC
DEFS += -DPROC_SUPPORT

# add objects for P-ROC
PROCOBJS = \
 $(OBJ)/p-roc/p-roc.o \
 $(OBJ)/p-roc/display.o \
 $(OBJ)/p-roc/gameitems.o

# add libraries for P-ROC
PROCLIBS = \
 -lpinproc \
 -lftd2xx \
 -lyaml-cpp

# P-ROC functions
$(OBJ)/p-roc/%.o: src/p-roc/%.cpp
	@echo Compiling $<...
	$(CPP) $(CDEFS) $(CPPFLAGS) -c $< -o $@

endif
