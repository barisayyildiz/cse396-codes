#include <iostream>
#include <thread>

int calculate()
{
  int total = 0;
  for(int i=0; i<1000000000; i++) {
    total += i;
  }
  std::cout << "calculated : " << total << std::endl;
  return total;
}

int main()
{
  std::thread t1(calculate);
  t1.join();
  std::cout << "main thead" << std::endl;
  return 0;
}
