CFLAGS = -pthread -lm
PORT = 19123

server: srv_compile srv_execute

client: cln_compile cln_execute

srv_compile: 
	gcc examSrv3.c $(CFLAGS) -o srv3

srv_execute:
	./srv3 $(PORT)

cln_compile:
	gcc TcpClient.c -o cln

cln_execute:
	./cln localhost $(PORT)

clean:
	rm -f srv3 cln