#ifndef ENV_H
#define ENV_H

#include <iostream>
using namespace std;
#include <dirent.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

/*  Environment class has all attributes needed for environment
 */
class Env
{
 private:
  char ** env;                         // will be used as the third argument of execve
  char ** ECE551PATH;                  // ECE551PATH for shell, used for searching command
  size_t env_size;                     // number of elements in char** env
  size_t path_size;                    // number of elements in char** ECE551PATH
  map<string, string> var;             // store all exported variable
  map<string, string> background_var;  // has all most up-to-date variable, exported and unexported

 public:
  Env(char **);
  Env(const Env &);
  string search_cmd(const char *);
  Env & operator=(const Env &);
  void clear_env();
  void clear_path();
  void copy(const Env &);
  void init_env(char **);
  void update_env();
  void init_path();
  void add_update_var(string, string);
  string search_var(string);
  void env_export(char *);
  void increment(string);

  map<string, string> get_vars() { return background_var; }

  // Destructor
  ~Env() {
    clear_env();
    clear_path();
  }

  // Return the environment needed for execve
  char ** get_env() { return env; }
};

#endif
