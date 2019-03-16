#ifndef CONVERT_H
#define CONVERT_H

#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

/*  These functions are mainly used for built-in command 'inc'
 */

bool checknum(string);
int toInt(string);
string toStr(int);

/*  Check if a string is a number
 *  Used to check variable
 */
bool check_num(string num) {
  for (size_t i = 0; i < num.size(); i++) {
    if (!isdigit(num[i])) {
      return false;
    }
  }
  return true;
}

/*  Convert string to integer
 */
int toInt(string num) {
  int res = 0, size = num.size();
  for (int i = 0, j = size; i < size; i++) {
    res += (num[--j] - '0') * pow(10, i);
  }
  return res;
}

/*  Convert integer to string
 */
string toStr(int num) {
  stringstream ss;
  ss << num;
  string res = ss.str();
  return res;
}

#endif
