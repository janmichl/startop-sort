MAKE_FLAGS?=-j5

all: clean update
	mkdir -p build;
	cd build; cmake ..;
	cd build; ${MAKE} ${MAKE_FLAGS}

run:
	./build/main

clean:
	rm -Rf build

.PHONY: clean update run
