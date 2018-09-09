PREFIX := obj
OBJS := $(patsubst %.cpp,$(PREFIX)/%.o, $(wildcard *.cpp)) 
BIN := Compressor

#INCLUDES += -I../LibTiff
CXXFLAGS += -std=c++11 -g $(INCLUDES)
LFLAGS +=  -ltiff

all: $(PREFIX)/$(BIN)

$(PREFIX)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(PREFIX)/$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LFLAGS) $(OBJS) -o $@ 

clean:
	rm -rf $(PREFIX)/*.o

