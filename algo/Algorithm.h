#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <iostream>
#include <string>
#include <regex>
#include <vector>

int myatoi(const char *s)
{
    const char *p = s;
    int neg = 0, val = 0;

    while ((*p == '\n') || (*p == '\t') || (*p == ' ') ||
           (*p == '\f') || (*p == '\r') || (*p == '\v')) {
        p++;
    }
    if ((*p == '-') || (*p == '+')) {
        if (*p == '-') {
            neg = 1;
        }
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        if (neg) {
            val = (10 * val) - (*p - '0');
        } else {
            val = (10 * val) + (*p - '0');
        }
        p++;
    }
    return val;
}

std::string toString(__uint128_t num) {
  std::string str;
  do {
    int digit = num % 10;
    str = std::to_string(digit) + str;
    num = (num - digit) / 10;
  } while (num != 0);
  return str;
}


void algorithm(std::string & incoming, std::vector<int> replaceTo) {
	std::regex regExp("(\\d( )*){5}");
	std::smatch match;

	if(std::regex_match(incoming.cbegin(), incoming.cend(), match, regExp)) {
		std::string output;
  	for(unsigned i = 0; i < match.size(); ++i) {
    	output += match.str(i);
  	}

		std::string toConvert = output.substr(2, incoming.length());
		// remove spaces
		__uint128_t number = myatoi(incoming.c_str());
		number *= 1000000;
		number += 252100;
		std::string firstTwoNums = toString(number);
		__uint128_t newNumber = myatoi(firstTwoNums.c_str());
		__uint128_t finallNumber = number + newNumber;
		int result = finallNumber % 97;

		if(result == 1) {
			int index = 0;
			for(unsigned i = 0; i < output.size(); ++i) {
    		if(isdigit(output[i])) {
    			output.replace(i, 1, std::string(replaceTo.at(index)));
    			index++;
    		}
  		}
		}
	}
}



#endif
