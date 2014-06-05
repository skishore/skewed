#include <iostream>
#include <memory>

int main(int argc, char** argv) {
  std::unique_ptr<std::string> message(new std::string("Welcome to skewed!"));
  std::cout << *message << std::endl;
  return 0;
}
