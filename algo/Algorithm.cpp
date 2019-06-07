#include <iostream>
#include <string>
#include <sstream>
#include <vector>


void trim(std::string & str) {
  while(str.find(' ') != std::string::npos) {
    str.erase(str.find(' '), 1);
  }
}

std::string replace26Digits(std::string & str26Digits, std::string & replaceTo) {  
  std::string trimmed(str26Digits);
  trim(trimmed);

  long long firstTwoDigits = std::stoi(trimmed.substr(0, 2));
  trimmed = trimmed.substr(2);

  long long numberWithoutFirstTwoDigits;
  std::istringstream iss(trimmed);
  iss >> numberWithoutFirstTwoDigits;
  numberWithoutFirstTwoDigits *= 1000000;
  numberWithoutFirstTwoDigits += 252100;

  long result = (firstTwoDigits + numberWithoutFirstTwoDigits) % 97;

  if(result == 1) {
    int index = 0;
    for(unsigned i = 0; i < str26Digits.length(); ++i) {
      if(!isspace(str26Digits[i])) {
        str26Digits[i] = replaceTo[index];
        ++index;
      }
    }
  }
  return str26Digits;
}

std::string algorithm(std::string & inputString, std::string & replaceTo) {
  std::string response(inputString);
  const int seqLength = 26;
  int digitCounter = 0;
  int start = 0;
  int end = 0;
  bool foundFirstDigit = false;

  for(unsigned i = 0; i < response.length(); ++i) {
    if(isdigit(response[i])) {
      ++digitCounter;
      if(!foundFirstDigit) {
        start = i;
        foundFirstDigit = true;
      }
    }
    
    if(!isdigit(response[i]) && !isspace(response[i]) && digitCounter < seqLength ) {
      digitCounter = 0;
      foundFirstDigit = false;
    }
    if(digitCounter == seqLength) {
      end = i;
      break;
    }
  }
  if(digitCounter != seqLength) {
    return response;
  } else {
    std::string digitsWithSpaces = response.substr(start, end - start + 1);
    std::string replacedString = replace26Digits(digitsWithSpaces, replaceTo);
    response.replace(start, end - start + 1, replacedString);
    return response;
  }
}

int main() {
  std::string replaceTo("00000000000000000000000000");
  std::string shouldNotChange = "testtest123 4123412    341  234 1234123426testse";

  std::string res = algorithm(shouldNotChange, replaceTo);
  std::cout << shouldNotChange << std::endl << "should not change" << std::endl << res << std::endl;

  std::string sholdChange = "testetstess00000000 0000000000 00000072testets";
  res = algorithm(sholdChange, replaceTo);
  std::cout << sholdChange << std::endl << "should change" << std::endl << res << std::endl;
}
