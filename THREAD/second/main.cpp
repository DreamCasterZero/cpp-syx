#include <iostream>
#include <thread>

int main(){
    unsigned int n = std::thread::hardware_concurrency();
    std::cout << "支持 " << n << " 个并发线程\n";
    return 0;
}