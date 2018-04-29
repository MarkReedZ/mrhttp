#include <iostream>
#include <stdlib.h>
using namespace std;

extern "C" void cppfoo()
{
    string hello = "Hello world!";
    cout << hello << endl;
}
