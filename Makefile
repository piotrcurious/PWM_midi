CXX = g++
CXXFLAGS = -I. -Itests

all: tests/runner tests/gui_bridge

tests/runner: tests/test_main.cpp jazz_logic.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^

tests/gui_bridge: tests/gui_bridge.cpp jazz_logic.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^

clean:
	rm -f tests/runner tests/gui_bridge progression.mid

.PHONY: all clean
