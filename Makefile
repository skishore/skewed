BUILD := build
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.o)))
EXECUTABLE := $(BUILD)/main

CC := clang++
CC_FLAGS := -Wall -std=c++11 -stdlib=libc++
LD_FLAGS := $(CC_FLAGS) -levent

all: $(EXECUTABLE)

clean:
	rm -f $(OBJ_FILES) $(EXECUTABLE)
	rmdir -p $(BUILD)

$(EXECUTABLE):	$(OBJ_FILES) 
	mkdir -p $(BUILD)
	$(CC) $(LD_FLAGS) -o $@ $^

$(BUILD)/%.o: %.cpp
	mkdir -p $(BUILD)
	$(CC) $(CC_FLAGS) -c -o $@ $<
