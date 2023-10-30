files:
	gcc sent/deliver.c -o deliver
	mv deliver sent
	gcc received/server.c -o server
	mv server received
