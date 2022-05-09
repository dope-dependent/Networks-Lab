all: 1 2

1: user1.c librsocket.a
	gcc user1.c -L. -lrsocket -lpthread -o 1

2: user2.c librsocket.a
	gcc user2.c -L. -lrsocket -lpthread -o 2
