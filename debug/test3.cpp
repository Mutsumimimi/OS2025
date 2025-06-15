#include <iostream>
// using namespace std;

#define MONTH 12

int main(){
    // std::cout << "Hello World!" << std::endl;
    // std::cout << "THis is for testing of \\n \n." << std::endl;
    // fflush(stdout);
    int month = 0;
    std::cin >> month;
    if(month == MONTH){
        std::cout << "yes, it's " << MONTH << std::endl;
    } else {
        std::cout << "no, it's " << MONTH << std::endl;
    }
    size_t size;
    return 0;
}