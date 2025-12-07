compile: a.c b.c c.c
	gcc a.c -o a `pkg-config fuse --cflags --libs`
	gcc b.c -o b `pkg-config fuse --cflags --libs`
	gcc c.c -o c `pkg-config fuse --cflags --libs`

clean:
	rm -f a b c

prepare:
	sudo mkdir -p /mnt/fuse
	sudo chown $(USER):$(USER) /mnt/fuse
	mkdir -p /tmp/testa
	mkdir -p /tmp/testb
	mkdir -p /tmp/testc