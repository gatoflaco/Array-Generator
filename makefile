HDR = Headers
OBJ = Objects
SRC = Sources

ALL all: build
DEBUG debug: build-debug

CXXFLAGS := -std=c++11 -lm
CXXFLAGS += -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy\
-Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs\
-Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow \
-Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused -O3

build: generate
build-debug: CXXFLAGS += -g -g3
build-debug: CXXFLAGS:=$(filter-out -O3, $(CXXFLAGS))
build-debug: build

generate: $(HDR)/* $(SRC)/*
	$(CXX) $(CXXFLAGS) -I $(HDR) -o generate $(SRC)/*.cpp

clean:
	$(RM) generate