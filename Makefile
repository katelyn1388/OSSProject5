CC = gcc -g3
CFLAGS = -g3 -Wall
TARGET1 = oss
TARGET2 = worker

OBJS1 = oss.o
OBJS2 = worker.o 

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1) resources.h

$(TARGET2): $(OBJS2) 
	$(CC) -o $(TARGET2) $(OBJS2) resources.h

exec_master.o:	oss.c 
	$(CC) $(CFLAGS) -c oss.c resources.h

exec_worker.o:	worker.c
	$(CC) $(CFLAGS) -c worker.c resources.h

clean:
	/bin/rm -f *.o $(TARGET1) $(TARGET2)
	/bin/rm -f *.txt
