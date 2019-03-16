#include "environment.h"

#include "conversion.h"
/*=============================    Constructors    ================================*/

// Normal Constructor
Env::Env(char ** environ) : env(NULL), ECE551PATH(NULL), env_size(0), path_size(0) {
  var["ECE551PATH"] = string(getenv("PATH"));
  background_var["ECE551PATH"] = string(getenv("PATH"));
  init_env(environ);
  init_path();
}

// Copy Constructor
Env::Env(const Env & rhs) : env(NULL), ECE551PATH(NULL), env_size(0), path_size(0) {
  copy(rhs);
}

/*================================================================================*/

/*
 *
 */

/*================================   Functions   ===================================*/

/*  Search for command under either PATH or user-specified directory
 *  Return empty string if command not found, return full path of the command if found
 *  Returned result will be used directly as first argument of execve
 */
string Env::search_cmd(const char * cmd) {
  DIR * d;
  struct dirent * dir;
  string res(cmd);

  // If user specifies a path
  if (res.find("/") != string::npos) {
    size_t last_path = res.size() - 1;
    while (last_path >= 0) {
      if (res[last_path] == '/') {
        break;
      }
      last_path--;
    }
    d = opendir(res.substr(0, last_path).c_str());
    if (d != NULL) {
      while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, res.substr(last_path + 1, res.size() - last_path - 1).c_str()) ==
                0 &&
            !(dir->d_type & DT_DIR)) {
          closedir(d);
          return res;
        }
      }
      closedir(d);
    }
    res = "";
  }

  // If user does not specify a path
  else {
    res = "";
    for (size_t i = 0; i < path_size; i++) {
      d = opendir(ECE551PATH[i]);
      if (d != NULL) {
        while ((dir = readdir(d)) != NULL) {
          if (strcmp(dir->d_name, cmd) == 0 && !(dir->d_type & DT_DIR)) {
            res += string(ECE551PATH[i]) + "/" + string(cmd);
            closedir(d);
            return res;
          }
        }
        closedir(d);
      }
    }
  }

  return res;
}

/*  Deep copy an Env object over assignment operator
 */
Env & Env::operator=(const Env & rhs) {
  if (this != &rhs) {
    clear_env();
    clear_path();
    copy(rhs);
  }
  return *this;
}

/*  Initialize the Env object
 *  Set env and env_size
 */
void Env::init_env(char ** environ) {
  while (environ[env_size] != NULL) {
    string tmp = environ[env_size];
    size_t equal_sign = tmp.find('=');
    string key = tmp.substr(0, equal_sign);
    string value = tmp.substr(equal_sign + 1, tmp.size() - equal_sign - 1);
    var[key] = value;
    background_var[key] = value;
    env_size++;
  }
  update_env();
}

/*  Update environment used for execve
 */
void Env::update_env() {
  int track = 0;
  for (map<string, string>::iterator it = var.begin(); it != var.end(); it++) {
    string tp_var = it->first + "=" + it->second;
    env = (char **)realloc(env, (track + 1) * sizeof(*env));
    env[track] = (char *)malloc((tp_var.size() + 1) * sizeof(*env[env_size]));
    strcpy(env[track], tp_var.c_str());
    track++;
  }
  env = (char **)realloc(env, (track + 1) * sizeof(*env));
  env[track] = NULL;
  track++;
  env_size = track;
}

/*  Initialize/update ECE551PATH based on
 *  exported variable "ECE551PATH"
 */
void Env::init_path() {
  char * tmp_path = (char *)malloc((var["ECE551PATH"].size() + 1) * sizeof(*tmp_path));
  strcpy(tmp_path, var["ECE551PATH"].c_str());
  char * token = strtok(tmp_path, ":");

  while (token != NULL) {
    ECE551PATH = (char **)realloc(ECE551PATH, (path_size + 1) * sizeof(*ECE551PATH));
    ECE551PATH[path_size] = (char *)malloc((strlen(token) + 1) * sizeof(*ECE551PATH[path_size]));
    strcpy(ECE551PATH[path_size], token);
    path_size++;
    token = strtok(NULL, ":");
  }

  free(tmp_path);
}

/*   Destroy Env object, free all memories used
 */
void Env::clear_env() {
  for (size_t i = 0; i < env_size; i++) {
    free(env[i]);
  }
  free(env);
  env = NULL;
  env_size = 0;
}

void Env::clear_path() {
  for (size_t i = 0; i < path_size; i++) {
    free(ECE551PATH[i]);
  }
  free(ECE551PATH);
  ECE551PATH = NULL;
  path_size = 0;
}

/*  Deep copy an Env Object
 */
void Env::copy(const Env & rhs) {
  path_size = rhs.path_size;
  env_size = rhs.env_size;
  ECE551PATH = (char **)malloc(path_size * sizeof(*ECE551PATH));
  env = (char **)malloc(env_size * sizeof(*env));
  for (size_t i = 0; i < env_size; i++) {
    env[i] = (char *)malloc(strlen(rhs.env[i]) * sizeof(*env[i]));
    strcpy(env[i], rhs.env[i]);
  }
  for (size_t i = 0; i < path_size; i++) {
    ECE551PATH[i] = (char *)malloc(strlen(rhs.ECE551PATH[i]) * sizeof(*ECE551PATH[i]));
    strcpy(ECE551PATH[i], rhs.ECE551PATH[i]);
  }
}

/*  Check if a variable exist in both exported and unexported
 *  Return the value if exist, otherwise return empty string
 */
string Env::search_var(string key) {
  if (background_var.count(key)) {
    return background_var[key];
  }
  return "";
}

/*  Export the variable if it is in background, otherwise report error
 *  Move/update variable from background_var to var
 *  Need to update env and ECE551PATH if export is successful
 *  Usually called by command "export"
 */
void Env::env_export(char * new_path) {
  string tp = new_path;
  if (background_var.count(tp)) {
    clear_env();
    var[tp] = background_var[tp];
    update_env();
    clear_path();
    init_path();
  }
  else {
    cout << "Export: variable does not exist!" << endl;
  }
}

/*  Check if a variable exist
 *  If exist: increment it by one if its value is a number, otherwise set value to 1
 *  If not exist: create the variable and set its value to 1
 */
void Env::increment(string key) {
  string tp = search_var(key);
  if (tp == "") {
    add_update_var(key, "1");
  }
  else if (check_num(tp)) {
    add_update_var(key, toStr((toInt(tp) + 1)));
  }
  else {
    add_update_var(key, "1");
  }
}

/*  Add or update a variable 
 *  simply add to background, updated var needs to be exported before goes into effect
 */
void Env::add_update_var(string key, string val) {
  background_var[key] = val;
}

/*==================================================================================*/
