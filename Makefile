CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lSDL2 -lm -lSDL2_ttf
TARGET = tictac
SRC = tictac.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

