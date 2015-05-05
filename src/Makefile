OBJECTS = pic-update

all:$(OBJECTS)

PKG_NAME=pic-update

pic-update: pic-update.o
	$(CC) $(LDFLAGS) $^ -o $@

pic-update.o: pic-update.c
	$(CC) $(CFLAGS) -c $<

.PHONY:clean
clean:
	@-rm *.o pic-update
