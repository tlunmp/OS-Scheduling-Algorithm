CC = gcc
CFLAGS = 
LDFLAGS = -pthread -lm 
TARGET = oss
TARGET1 = user 
TARGET2 = control 
OBJ = oss.o 
OBJ1 = user.o
OBJ2 = control.o
SRC = oss.c
SRC1 = user.c
SRC2 = control.c


all: $(TARGET) $(TARGET1)
	
$(TARGET):$(OBJ)
	$(CC)  -o $(TARGET) $(LDFLAGS) $(OBJ) $(OBJ2)

$(TARGET1):$(OBJ1)
	$(CC)  -o $(TARGET1) $(LDFLAGS) $(OBJ1) 


$(OBJ): $(SRC)
	$(CC)  $(CFLAGS) -c $(SRC) $(SRC2)

$(OBJ1): $(SRC1)
	$(CC)  $(CFLAGS) -c $(SRC1)


clean:
	/bin/rm -f *.o $(TARGET)
	/bin/rm -f *.o $(TARGET1)
