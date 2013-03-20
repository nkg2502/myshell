CC = gcc
CFLAGS = -Wall
PROGNAME = myshell
OBJECTS = myshell.o utility.o
INCLUDES = myshell.h

$(PROGNAME) : $(OBJECTS)
	$(CC) $(OBJECTS) -o $(PROGNAME)
$(OBJECTS) : $(INCLUDES)

clean :
	rm -rf $(OBJECTS) 
