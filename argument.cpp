#include "argument.h"

/*=============================    Constructors    ================================*/
// Normal Constructor
Arg::Arg(Env * e) : argument(NULL), num_arg(0), env(e) {
  redirect = vector<int>(3, 0);
  pre_arg = vector<string>();
}

// Normal Constructor
Arg::Arg(string arg, Env * e) : argument(NULL), num_arg(0), env(e) {
  redirect = vector<int>(3, 0);
  pre_arg = vector<string>();
  parse_cmd(arg);
}

// Copy Concstructor
Arg::Arg(const Arg & rhs) : argument(NULL), num_arg(0), env(NULL) {
  redirect = vector<int>(3, 0);
  pre_arg = vector<string>();
  copy(rhs);
}
/*==================================================================================*/

/*
 * 
 */

/*================================   Functions   ===================================*/

/*  Overwrite = operator, rule of three
 */
Arg & Arg::operator=(const Arg & rhs) {
  if (this != &rhs) {
    clear();
    copy(rhs);
  }
  return *this;
}

/*  Destructor
 */
Arg::~Arg() {
  clear();
  env = NULL;
}

/*  Free all memory on the heap used by Arg object
 *  env is freed in Env class, so don't free here
 */
void Arg::clear() {
  for (size_t i = 0; i < num_arg; i++) {
    free(argument[i]);
  }
  free(argument);
  env = NULL;
}

/*  Deep copy another Arg object, used in copy constructor and =
 */
void Arg::copy(const Arg & rhs) {
  num_arg = rhs.num_arg;
  argument = (char **)malloc(num_arg * sizeof(*argument));
  for (size_t i = 0; i < num_arg; i++) {
    argument[i] = (char *)malloc((strlen(rhs.argument[i]) + 1) * sizeof(*argument[i]));
    strcpy(argument[i], rhs.argument[i]);
  }
  redirect = rhs.redirect;
  pre_arg = rhs.pre_arg;
}

/*  Parse command into correct format which can be processed
 *  pre_parse distinguish between arguments and  put all arguments in pre_arg
 *  final_parse deals with '\', '$' and redirect
 */
void Arg::parse_cmd(string arg) {
  pre_parse(arg);
  final_parse();
}

/*  Divide arguments by single or multiple spaces
 *  But take space literally when it is preceded by \
 *  \ can only apply to the next char after (non-greedy)
 *  store result in vector pre_arg
 */
void Arg::pre_parse(string arg) {
  string tmp = "";
  unsigned left = 0, right = 0;
  while (arg[left] == ' ') {
    left++;
    right++;
  }
  while (right < arg.size()) {
    if (arg[right] == ' ' && arg[right - 1] != '\\') {
      tmp = arg.substr(left, right - left);
      if (!tmp.empty()) {
        pre_arg.push_back(process_var(tmp));
      }
      while (arg[right] == ' ') {
        right++;
      }
      left = right;
      right++;

      // command set is a special case
      if (pre_arg[0] == "set" && pre_arg.size() == 2) {
        pre_arg.push_back(process_var(arg.substr(left, arg.size() - left)));
        return;
      }
    }
    else {
      right++;
    }
  }

  if (arg[left] != ' ' && left < right) {
    tmp = arg.substr(left, right - left);
    if (!tmp.empty()) {
      pre_arg.push_back(process_var(tmp));
    }
  }
}

/*  Check and pre-process redirection (is_redirect)
 *  Parse '$' and '\' correctly (process_var)
 *  Store result in char* argument that later is used by execve
 */
void Arg::final_parse() {
  for (size_t i = 0; i < pre_arg.size(); i++) {
    // don't take in redirection symbol and all arguments afterward
    if (is_redirect(pre_arg[i], i) && pre_arg[0] != "set") {
      i++;
      pre_arg[i] = pre_arg[i];
      continue;
    }
    string cur_arg = pre_arg[i];  // fully parsed
    argument = (char **)realloc(argument, (num_arg + 1) * sizeof(*argument));
    argument[num_arg] = (char *)malloc((cur_arg.size() + 1) * sizeof(*argument[num_arg]));
    strcpy(argument[num_arg], cur_arg.c_str());
    num_arg++;
  }
  argument = (char **)realloc(argument, (num_arg + 1) * sizeof(*argument));
  argument[num_arg] = NULL;
  num_arg++;
}

/*  Check if an argument is '<', '>' or '2>' which indicates redirection
 *  If redirection exists, pre set the file that output/input is redirected to, return true
 *  Return false if the argument does not indicate redirection
 */
bool Arg::is_redirect(string arg, size_t num) {
  if (arg == ">") {
    redirect[0] = num + 1;
    return true;
  }
  else if (arg == "<") {
    redirect[1] = num + 1;
    return true;
  }
  else if (arg == "2>") {
    redirect[2] = num + 1;
    return true;
  }
  return false;
}

/*  Use dup2() to invoke redirection before execve
 */
void Arg::invoke_redirect() {
  for (size_t i = 0; i < redirect.size(); i++) {
    if (i == 0 && redirect[i] != 0) {
      int fd = open(pre_arg[redirect[i]].c_str(), O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
      if (dup2(fd, STDOUT_FILENO) < 0) {
        perror("dup2");
        exit(EXIT_FAILURE);
      }
      close(fd);
    }
    else if (i == 1 && redirect[i] != 0) {
      int fd = open(pre_arg[redirect[i]].c_str(), O_RDONLY, S_IRUSR);
      if (dup2(fd, STDIN_FILENO) < 0) {
        perror("dup2");
        exit(EXIT_FAILURE);
      }
      close(fd);
    }
    else if (i == 2 && redirect[i] != 0) {
      int fd = open(pre_arg[redirect[i]].c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
      if (dup2(fd, STDERR_FILENO) < 0) {
        perror("dup2");
        exit(EXIT_FAILURE);
      }
      close(fd);
    }
  }
}

/*  Fully parsed the command line, deal with \
 *  Translate the rest of variable
 */
string Arg::process_var(string cmd) {
  string res = "";
  string tp = "";
  bool backslash = false;  // true if last char is backslash

  for (size_t i = 0; i < cmd.size(); i++) {
    if (backslash) {  //prev elem is backslash
      res.push_back(cmd[i]);
      backslash = false;
    }
    else {  //prev elem is not backslash
      if (cmd[i] == '\\') {
        backslash = true;
        continue;
      }
      else if (cmd[i] == '$') {
        if (i == cmd.size() - 1) {
          res.push_back(cmd[i]);
        }
        else if (i + 1 < cmd.size() && !(cmd[i + 1] == '_' || isalnum(cmd[i + 1]))) {
          res.push_back(cmd[i]);
        }
        // take the longest var name only
        else {
          size_t left = i + 1;
          size_t right = i + 1;
          while (right <= cmd.size() && (cmd[right] == '_' || isalnum(cmd[right]))) {
            right++;
          }
          tp = cmd.substr(left, right - left);
          string search_res = env->search_var(tp);  //search for variable
          if (search_res != "" && search_res != ">" && search_res != "<" && search_res != "2>") {
            res += search_res;
          }
          i = right - 1;
        }
      }
      else {
        res.push_back(cmd[i]);
      }
      backslash = false;
    }
  }
  if (cmd[cmd.size() - 1] == '\\') {
    res.push_back('\\');
  }
  return res;
}

/*==================================================================================*/
