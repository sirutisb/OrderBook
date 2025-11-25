#include <iostream>
//#include "orderBook.h"
#include <vector>

int main() {
	std::vector<int> vec;
	for (int i = 0; i < 100; i++) {
		std::cout << i << " -> " << vec.size() << "/" << vec.capacity() << "\n";
		vec.push_back(i);
	}
}