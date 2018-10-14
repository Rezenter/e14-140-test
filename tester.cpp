#include <iostream> //debug
using namespace std;

string prefix = "";

#include "ExampleModule.h"

int main(int argc, char **argv)
{
    cout << prefix << "main\n";
    ExampleModule test("name", "type");
    return 0;
}