#include "../cpparse.hxx"

int main() {
  cpparse::Parser parser;
  auto flag = parser.add_flag<>("flag", true);
}
