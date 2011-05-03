#Variables utilisées :
CC=g++ #Compilateur
EDL=g++ #Linker
CCFLAGS=-Wall #Options de compilations
EDLFLAGS=-Wall
EXE=rip #Nom du binaire à construire

OBJ=client.o command.o ripd.o io_tools.o panic.o tcpserver.o tcpclient.o
LIBS=-lpthread



$(EXE): $(OBJ)
	@echo building $<
	$(EDL)  -o $(EXE) $(EDLFLAGS) $(OBJ) $(LIBS)
	@echo done

%.o : %.cpp *.h
	@echo building $< ...
	$(CC) $(CCFLAGS) -c $<
	@echo done
	
clean: 
	@echo -n cleaning repository... 
	@rm -f *.o
	@rm -f .*.swp
	@rm -f *~
	@rm -f *.log
	@rm -f *.pid
	@rm -f *.out
	@echo cleaned.
	
coffee :
	@echo You go to work!
	@vi -p tcpserver.cpp tcpserver.h tcpclient.cpp tcpclient.h ripd.cpp ripd.h command.cpp command.h io_tools.cpp io_tools.h client.cpp if.h
