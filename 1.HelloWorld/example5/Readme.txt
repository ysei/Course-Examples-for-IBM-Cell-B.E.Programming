spu-gcc spu_hello.c -o spu_hello
./spu_hello

embedspu hello_spu spu_hello spu_hello_embedded.o

gcc ppu_hello5.c spu_hello_embedded.o -lspe2 -o ppu_hello5
./ppu_hello5
