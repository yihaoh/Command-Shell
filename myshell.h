#ifndef MYSHELL_H
#define MYSHELL_H

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "argument.h"
#include "environment.h"
using namespace std;

/*  myShell class has all attributes needed to run the shell
 */
class myShell
{
 private:
  Env * env;                // Environments for the shell
  vector<string> pipe_cmd;  // Each string stores the whole command line, only used in pipeline
  bool exit_status;

 public:
  myShell(Env *);
  void run();
  bool prompt_shell(string &);
  bool builtin(Arg *);
  size_t count_pipe(string);
  Arg ** split_pipe_cmd(size_t);
  void execute_all(Arg **, size_t);
  bool check_redir();
  bool has_unexpected(Arg **, size_t);
  string process_cmd(string);
};

#endif
