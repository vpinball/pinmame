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

# add P-ROC folders for external (library, etc.) and internal includes
CFLAGS += -Ip-roc/include -IYAML_CPP/include -Isrc/p-roc
CPPFLAGS += -Ip-roc/include -IYAML_CPP/include -Isrc/p-roc

# add libraries for P-ROC
LDFLAGS += -Lp-roc/lib -LYAML_CPP\lib
LIBS += -lpinproc -lyaml-cpp
# -lftd2xx

endif
