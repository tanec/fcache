#include <stdio.h>

int main() {
#define p(x) printf("sizeof(%8s) = %d\n", #x, sizeof(x));

  p(char);
  p(short);
  p(unsigned short);
  p(int);
  p(unsigned int);
  p(long);
  p(unsigned long);
  p(long long);
  p(unsigned long long);
  p(float);
  p(double);
}
