target = gull

source = $(wildcard gull*.c)
object = $(source:%.c=cache/%.o)
depend = -pthread

cache/%.o: %.c
	mkdir -p cache
	gcc -Wall -c $< -o $@

gull: $(object)
	gcc $^ $(depend) -o $@

test: gull
	$(shell pwd)/gull

clean:
	rm gull $(object)
	rm -rf cache

once: gull
	$(shell pwd)/gull
	make clean
