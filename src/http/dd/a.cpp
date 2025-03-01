#include <vector>
#include <cstdio>

using namespace std;

int main() {
    // resize() 在需要扩容时, 会以"2倍"的方式进行扩容
    vector<char> vec; 
    printf("%zd %zd\n", vec.size(), vec.capacity()); 
    vec.resize(1024); 
    printf("%zd %zd\n", vec.size(), vec.capacity()); 
    vec.resize(1300); 
    printf("%zd %zd\n", vec.size(), vec.capacity()); 


    // reserve() 在需要扩容时, 只会扩容至指定的空间大小. 
    vector<char> vec2;
    printf("%zd %zd\n", vec.size(), vec.capacity()); 
    vec2.reserve(1024); 
    printf("%zd %zd\n", vec2.size(), vec2.capacity()); 
    vec2.reserve(1300); 
    printf("%zd %zd\n", vec2.size(), vec2.capacity()); 
}