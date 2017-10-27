#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <array>
#include <memory>

using namespace std;

struct X {
  int x;
};

int main() {
  const unique_ptr<X> y = unique_ptr<X>(new X{5});
  cout << y->x << endl;
  y->x = 6;
  cout << y->x << endl;
}
