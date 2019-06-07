#include <iostream>
#include <vector>
#include "Algorithm.h"


int main() {
	std::vector<int> replaceTo(26, 0);
	std::string str("12341234123412341234123426");
	algorithm(str, replaceTo);
}
