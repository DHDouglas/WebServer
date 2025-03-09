#include "config.h"

using namespace std;

int main(int argc, char* argv[]) {
    Config config(argc, argv); 
    config.printArgs();
}