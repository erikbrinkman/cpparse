CFLAGS = -std=c++14 -O3 -Werror -Wall -Wextra -pedantic
SOURCES = cpparse.cxx cpparse.hxx example.cxx

help:
	@echo "usage: make <target>"
	@echo
	@echo "Targets:"
	@echo "  example : Compile example program"
	@echo "  test    : Run tests"
	@echo "  format  : Format source files with standard style"

example: $(SOURCES)
	g++ $(CFLAGS) -o $@ $@.cxx

test:
	@echo "No tests yet :(" && false

format: $(SOURCES)
	clang-format-3.7 -i -style=Google $(SOURCES)

todo: $(SOURCES)
	@grep --color=always XXX $(SOURCES) || true
	@grep --color=always TODO $(SOURCES) || true
	@grep --color=always FIXME $(SOURCES) || true
