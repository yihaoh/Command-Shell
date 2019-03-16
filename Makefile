SOURCES=myshell.cpp environment.cpp argument.cpp
OBJS=$(patsubst %.cpp, %.o, $(SOURCES))
CPPFLAGS=-ggdb3 -Wall -Werror -pedantic -std=gnu++03

myShell: $(OBJS)
	g++ $(CPPFLAGS) -o myShell $(OBJS)
%.o: %.cpp myshell.h environment.h argument.h 
	g++ $(CPPFLAGS) -c $<

clean:
	rm myShell *~ *.o
