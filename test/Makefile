CFLAGS = -std=c++14 -O3 -Werror -Wall -Wextra -pedantic

test: copying

copying: copying.cxx
	g++ -std=c++14 copying.cxx 2>&1 | grep 'error: use of deleted function ‘cpparse::Flag<T>::Flag(const cpparse::Flag<T>&)' > /dev/null

format:
	clang-format-3.7 -i -style=Google *.cxx
