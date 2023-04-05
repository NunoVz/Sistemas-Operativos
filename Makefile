CC = gcc
FLAGS = -g -pthread -lrt -Wall
OBJS = Functions.c HeaderFile.h
MAIN = SystemManager.c
SENSOR = Sensor.c
USERCONSOLE = UserConsole.c
TARGET = system
TARGET2 = sensor
TARGET3 = user_console

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(FLAGS) $(MAIN) $(OBJS) -o $(TARGET) -lrt
		$(CC) $(FLAGS) $(SENSOR) $(OBJS) -o $(TARGET2) -lrt
		$(CC) $(FLAGS) $(USERCONSOLE) $(OBJS) -o $(TARGET3) -lrt
		
clean:
				$(RM) $(TARGET)
				$(RM) $(TARGET2)
				$(RM) $(TARGET3)
