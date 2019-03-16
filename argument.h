#ifndef ARG_H
#define ARG_H
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "environment.h"
using namespace std;

/* Arg class has all attributes needed to process a whole command
 * e.g. ls -l -a
 */
class Arg
{
 private:
  char ** argument;        // will be used directly for second argument of execve
  size_t num_arg;          // number of arguments
  Env * env;               // Pointer to the environment of myShell
  vector<string> pre_arg;  // raw arguments that has not been fully parsed
  vector<int> redirect;    // store index that has the redirect file name in pre_arg

 public:
  Arg(Env *);
  Arg(string, Env *);
  Arg(const Arg &);
  Arg & operator=(const Arg &);
  ~Arg();
  void clear();
  void copy(const Arg &);
  void parse_cmd(string);
  void pre_parse(string);
  void final_parse();
  void invoke_redirect();
  bool is_redirect(string, size_t);
  string process_var(string);
  // Get argument
  char ** get_arg() { return argument; }

  // Get the command alone (no extra argument)
  char * get_cmd() { return argument[0]; }

  // Return the number of arguments in whole command
  size_t get_arg_num() { return num_arg; }

  // Get redirect destination files
  vector<int> get_redirect() { return redirect; }

  // Get pre_arg
  vector<string> get_pre_arg() { return pre_arg; }
};

#endif
