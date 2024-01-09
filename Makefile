SDS_PATH=./sds
LUNITY=-I./Unity/src ./Unity/build/libunity.a

DEBUGFLAGS=-fsanitize=address
CFLAGS=-g -O0 -std=c11
WFLAGS=-Wall -Wextra -Werror=incompatible-pointer-types-discards-qualifiers -Werror=return-type -Wno-unused-command-line-argument
DEPS= \
	-Isrc \
	$(LUNITY) \
	-Illhttp/build -Lllhttp/build llhttp/build/libllhttp.a \
	-I$(SDS_PATH) \
	-I.

.PHONY: build_deps
build_deps:
	cd llhttp; \
	npm install && npm run build && make; \
	cd ../Unity; \
	mkdir -p build && cd build; \
	cmake .. && make

.PHONY: test
test: http.o test/main.o $(SDS_PATH)/sds.o
	$(CC) -o ./run_tests $(CFLAGS) $(WFLAGS) $(DEPS) $(DEBUGFLAGS) $^; \
	./run_tests

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $(WFLAGS) $(DEPS) $^