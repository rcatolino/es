#Variables utilisées :
CC=g++ #Compilateur
EDL=g++ #Linker
CCFLAGS=-Wall #Options de compilations
EDLFLAGS=-Wall
EXE=rip #Nom du binaire à construire

OBJ=client.o command.o
LIBS=



$(EXE): $(OBJ) $(LIBS)
	@echo building $<
	$(EDL)  -o $(EXE) $(EDLFLAGS) $(OBJ) $(LIBS)
	@echo done

%.o : %.cpp *.h
	@echo building $< ...
	$(CC) $(CCFLAGS) -c $<
	@echo done
	
clean: 
	@echo -n cleaning repository... 
	@rm -rf *.o
	@echo cleaned.
