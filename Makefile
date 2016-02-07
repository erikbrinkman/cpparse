CFLAGS = -std=c++14 -O3 -Werror -Wall -Wextra -pedantic
SOURCES = $(wildcard *.cxx) $(wildcard *.hxx)

help:
	@echo "usage: make <target>"
	@echo
	@echo "Targets:"
	@echo "  all            : Compile all executables"
	@echo "  example        : Compile example program"
	@echo "  readme_example : Compile example program from readme"
	@echo "  test           : Run tests"
	@echo "  format         : Format source files with standard style"
	@echo "  todo           : List all todo flags in sources"

all: example readme_example

example: example.cxx cpparse.cxx cpparse.hxx indent_header.hxx indent.hxx indent.cxx
	g++ $(CFLAGS) -o $@ $@.cxx

readme_example: readme_example.cxx cpparse.cxx cpparse.hxx indent_header.hxx indent.hxx indent.cxx
	g++ $(CFLAGS) -o $@ $@.cxx

test: .PHONY
	$(MAKE) -C test test

format: $(SOURCES)
	clang-format-3.7 -i -style=Google $(SOURCES)

todo: $(SOURCES)
	@grep --color=always XXX $(SOURCES) || true
	@grep --color=always TODO $(SOURCES) || true
	@grep --color=always FIXME $(SOURCES) || true

.PHONY:
