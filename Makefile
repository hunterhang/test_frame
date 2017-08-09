all:
	make clean -C src/api
	make -C src/api
	make clean -C src
	make -C src

clean:
	make clean -C src