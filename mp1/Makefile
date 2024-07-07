COMPILERFLAGS = -g -Wall -Wextra -Wno-sign-compare

SERVEROBJECTS = obj/http_server.o
CLIENTOBJECTS = obj/http_client.o

.PHONY: all clean

all : obj http_server http_client

http_server: $(SERVEROBJECTS)
	$(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

http_client: $(CLIENTOBJECTS) 
	$(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

clean :
	$(RM) *.o http_server.o http_client.o

obj/%.o: src/%.c
	$(CC) $(COMPILERFLAGS) -c -o $@ $<

obj:
	mkdir -p obj
