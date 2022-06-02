
all: ppux.o

ppux.o: ppux.cpp drawlist.cpp drawlist.hpp drawlist_fwd.hpp drawlist_render.hpp pixelfont.cpp pixelfont.hpp utf8decoder.cpp
	$(CXX) -std=c++17 -c ppux.cpp

clean:
	$(RM) ppux.o
