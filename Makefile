CC := g++
LDFLAGS := -lprotobuf -pthread -lsqlite3

TARGET := myserver

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(wildcard *.cpp) MSG.pb.cc
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)