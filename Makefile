CC = gcc
FLAGS = -g -pthread -lrt -Wall
OBJS = Functions.c HeaderFile.h
MAIN = SystemManager.c
SENSOR = Sensor.c
USERCONSOLE = UserConsole.c
TARGET = system
TARGET2 = sensor
TARGET3 = user_console
LOG = ./files/log.txt

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(FLAGS) $(MAIN) $(OBJS) -g -o  $(TARGET) -lrt
		$(CC) $(FLAGS) $(SENSOR) $(OBJS) -g -o $(TARGET2) -lrt
		$(CC) $(FLAGS) $(USERCONSOLE) $(OBJS) -g -o $(TARGET3) -lrt
		
clean:
	$(RM) $(TARGET) $(TARGET2) $(TARGET3) $(LOG)
