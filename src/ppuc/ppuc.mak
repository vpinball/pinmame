# only PPUC specific output files and rules
#

OBJDIRS += $(OBJ)/ppuc

# add precompiler defines for PPUC
DEFS += -DPPUC_SUPPORT

# add objects for PPUC
PPUCOBJS = \
 $(OBJ)/ppuc/ppuc.o


# PPUC functions
$(OBJ)/ppuc/%.o: src/ppuc/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CCFLAGS) -c $< -o $@
