#include "myshell.h"

/*============================== Constructor ===============================*/
myShell::myShell(Env * e) : env(e), pipe_cmd(vector<string>()), exit_status(false) {}

/*
 *
 */

/*================================   Functions   ===================================*/

/*  Run the command shell
 *  Different execution for pipeline or non-pipeline command
 *  Loop forever until meets EOF in stdin or user exit
 */
void myShell::run() {
  string line;
  while (prompt_shell(line)) {
    if (line.find_first_not_of(' ') == string::npos) {
      continue;
    }
    else if (line != "") {
      size_t num_pipe = count_pipe(line);
      if (num_pipe == 0) {
        cout << "Pipeline/Redirection syntax error!" << endl;
        pipe_cmd = vector<string>();
      }
      else {
        Arg ** cmd_list = split_pipe_cmd(num_pipe);
        if (!has_unexpected(cmd_list, num_pipe)) {
          execute_all(cmd_list, num_pipe);
        }

        // clean memory
        for (size_t i = 0; i < num_pipe; i++) {
          delete cmd_list[i];
        }
        delete[] cmd_list;
        pipe_cmd = vector<string>();

        if (exit_status) {
          break;
        }
      }
    }
  }
}

/*  Print out heading of the shell
 *  Return true if not EOF of stdin, otherwise return false
 */
bool myShell::prompt_shell(string & line) {
  char * cur_dir = get_current_dir_name();
  cout << "\033[1;32mmyShell\033[0m:"
       << "\033[1;34m" << cur_dir << "\033[0m"
       << "$ ";
  free(cur_dir);
  getline(cin, line);
  return cin.good();
}

/*  Create an Arg object for each sub-command in pipeline
 *  Return the list of Arg object when done
 */
Arg ** myShell::split_pipe_cmd(size_t count) {
  Arg ** res = new Arg *[count]();
  for (size_t i = 0; i < count; i++) {
    res[i] = new Arg(pipe_cmd[i], env);
  }
  return res;
}

/*  Execute all commands
 *  Check if it is a single command or pipeline
 *  Check if a command is built-in before fork
 *  use pipe() to create pipe, dup2 to bind input/output
 */
void myShell::execute_all(Arg ** cmd_list, size_t num) {
  int * pipefd = new int[num * 2];
  pid_t * pid = new pid_t[num];

  // Pipe creation
  for (size_t i = 0; i < num; i++) {
    if (pipe(pipefd + 2 * i) == -1) {
      perror("pipe");
      exit(EXIT_FAILURE);
    }
  }

  // Run through each command in the cmd_list
  for (size_t i = 0; i < num; i++) {
    // Check whether the cmd is built-in, and/or it exists before fork and execute
    if (!builtin(cmd_list[i])) {
      if (env->search_cmd(cmd_list[i]->get_cmd()) == "") {
        cout << "Command " << cmd_list[i]->get_cmd() << " not found" << endl;
      }

      // command exists and not built-in, go into fork and execute process
      else {
        size_t read = (i - 1) * 2;
        size_t write = 2 * i + 1;
        pid[i] = fork();
        if (pid[i] == 0) {
          if (i != 0) {
            if (dup2(pipefd[read], STDIN_FILENO) < 0) {
              perror("pipe dup read");
              exit(EXIT_FAILURE);
            }
            close(pipefd[read]);
            close(pipefd[read + 1]);
          }
          if (i != num - 1) {
            if (dup2(pipefd[write], STDOUT_FILENO) < 0) {
              perror("pipe dup write");
              exit(EXIT_FAILURE);
            }
            close(pipefd[write]);
            close(pipefd[write - 1]);
          }

          // Execute the actual command
          string command = cmd_list[i]->get_cmd();
          string f_cmd =
              command.find("/") == string::npos ? env->search_cmd(command.c_str()) : command;
          cmd_list[i]->invoke_redirect();  // invoke redirection if any
          execve(f_cmd.c_str(), cmd_list[i]->get_arg(), env->get_env());
          perror("execve");
          exit(EXIT_FAILURE);
        }
        else {
          // Close used file so they don't hold after finished
          if (i > 0) {
            close(pipefd[2 * (i - 1)]);
            close(pipefd[2 * i - 1]);
          }
        }
      }
    }
  }

  // Make sure to close all pipeline file when finished
  for (size_t i = 0; i < 2 * num; i++) {
    close(pipefd[i]);
  }

  // Wait for all child processes to terminate
  pid_t w;
  int status = 0;
  while ((w = wait(&status)) > 0) {
    if (w == -1) {
      perror("wait");
      exit(EXIT_FAILURE);
    }
    if (WIFEXITED(status)) {
      cout << "Program exited with status " << WEXITSTATUS(status) << endl;
    }
    else if (WIFSIGNALED(status)) {
      cout << "Program was killed by signal " << WTERMSIG(status) << endl;
    }
  }

  // Free all memory used for execution
  delete[] pipefd;
  delete[] pid;
}

/* Check if the command is built-in: cd, set, export, inc
 * If yes, do the command and return true, otherwise do nothing and return false
 */
bool myShell::builtin(Arg * arg) {
  string cmd = arg->get_cmd();
  char ** arguments = arg->get_arg();
  size_t num_arg = arg->get_arg_num();

  if (cmd == "exit" && num_arg == 2) {
    exit_status = true;
    return true;
  }

  // command is "cd"
  else if (cmd == "cd" && num_arg < 4) {
    if (arguments[1] == NULL) {
      if (chdir(getenv("HOME")) == -1) {
        perror("cd");
      }
    }
    else if (chdir(arguments[1]) == -1) {
      perror("cd");
    }
    return true;
  }

  // command is "set"
  else if (cmd == "set" && (num_arg == 4 || num_arg == 3)) {
    string arg1 = arguments[1];
    bool legit_var = true;
    // Check if var name is legit
    for (size_t i = 0; i < arg1.size(); i++) {
      if (!(arg1[i] == '_' || isalnum(arg1[i]))) {
        legit_var = false;
        break;
      }
    }
    if (legit_var) {
      if (num_arg == 4) {
        env->add_update_var(string(arguments[1]), string(arguments[2]));
      }
      else {
        env->add_update_var(string(arguments[1]), "");
      }
    }
    else {
      cout << "set: Variable name can only contain number, letter and '_'" << endl;
    }
    return true;
  }

  // command is "export"
  else if (cmd == "export" && num_arg == 3) {
    env->env_export(arguments[1]);
    return true;
  }

  // command is "inc"
  else if (cmd == "inc" && num_arg == 3) {
    env->increment(string(arguments[1]));
    return true;
  }

  // argument number error
  else if (cmd == "set" || cmd == "cd" || cmd == "export" || cmd == "inc" || cmd == "exit") {
    cout << "built-in syntax error: " << cmd << endl;
    return true;
  }

  return false;
}

/*  Count how many sub-commands are in the commands
 *  Distinguish each command by | in between
 *  Return 0 if there is syntax error in pipeline
 */
size_t myShell::count_pipe(string line) {
  line = process_cmd(line);
  size_t res = 0;
  size_t i = 0, j = line.size() - 1;
  //get rid of all spaces in front and back
  while (line[i] == ' ') {
    i++;
  }
  while (line[j] == ' ') {
    j--;
  }

  // first argument cannot be pipe
  if (line[i] == '|' || (line[j] == '|' && line.substr(i, i + 3) != "set")) {
    return 0;
  }

  // two pointer to catch a command
  size_t left = i, right = i + 1;
  while (right <= j) {
    // catch a cmd before |
    if (line[right] == '|' && right - 1 >= 0 && right + 1 < line.size() &&
        line[right - 1] != '\\') {
      while (line[left] == ' ') {
        left++;
      }

      // if set cmd found, take everything afterward
      if (line.substr(left, 3) == "set") {
        pipe_cmd.push_back(line.substr(left, line.size() - left));
        return ++res;
      }

      // not set cmd
      pipe_cmd.push_back(line.substr(left, right - left));
      left = right + 1;
      right++;
      res++;

      // check redirection syntax
      if (!check_redir()) {
        return 0;
      }
    }
    right++;
  }
  if (left < right) {
    // if set cmd found, take everything afterward
    if (line.substr(left, 3) == "set") {
      pipe_cmd.push_back(line.substr(left, line.size() - left));
      return ++res;
    }

    // normal command
    pipe_cmd.push_back(line.substr(left, right - left));
    res++;

    // check redirection syntax
    if (!check_redir()) {
      return 0;
    }
  }

  return res;
}

/*  Check if redirection sign is first or last argument
 *  of a command
 */
bool myShell::check_redir() {
  // check if redirection syntax error
  if (!pipe_cmd.empty()) {
    int r = pipe_cmd.back().size() - 1;
    int l = 0;
    while (pipe_cmd.back()[l] == ' ') {
      l++;
    }
    while (pipe_cmd.back()[r] == ' ') {
      r--;
    }
    if (pipe_cmd.back()[l] == '>' || pipe_cmd.back()[l] == '<' || pipe_cmd.back()[r] == '>' ||
        pipe_cmd.back()[r] == '<' || pipe_cmd.back().substr(0, 2) == "2>") {
      return false;
    }
  }
  return true;
}

/*  Check if there is unexpected redirection sign left
 */
bool myShell::has_unexpected(Arg ** cmd_list, size_t num) {
  for (size_t i = 0; i < num; i++) {
    vector<string> tp = cmd_list[i]->get_pre_arg();
    if (tp[0] == "set") {
      continue;
    }
    for (size_t j = 0; j < tp.size(); j++) {
      if (tp[j] == ">" || tp[j] == "2>" || tp[j] == "<") {
        if (j + 1 < tp.size() && (tp[j + 1] == ">" || tp[j + 1] == "2>" || tp[j + 1] == "<")) {
          cout << "Redirect: unexpected '" << tp[j + 1] << "'" << endl;
          return true;
        }
      }
      if (count(tp[j].begin(), tp[j].end(), '>') + count(tp[j].begin(), tp[j].end(), '<') > 1) {
        bool is_val = false;
        map<string, string> tp_map = env->get_vars();
        for (map<string, string>::iterator it = tp_map.begin(); it != tp_map.end(); it++) {
          if (it->second == tp[j]) {
            is_val = true;
          }
        }
        if (!is_val) {
          cout << "Redirect: unexpected extra redirect operator!" << endl;
          return true;
        }
      }
    }
  }
  return false;
}

/*  Process $: replace variable name with its value
 *             only translate pipeline and redirection if any variable contains them
 *  return a command line with pipeline and redirection translated
 */
string myShell::process_cmd(string cmd) {
  string res = "";
  string tp = "";

  for (size_t i = 0; i < cmd.size(); i++) {
    if (cmd[i] == '$' && i - 1 >= 0 && cmd[i - 1] != '\\') {
      if (i == cmd.size() - 1) {
        res.push_back(cmd[i]);
      }
      else if (i + 1 < cmd.size() && !(cmd[i + 1] == '_' || isalnum(cmd[i + 1]))) {
        res.push_back(cmd[i]);
      }
      // take the longest var name only
      else {
        size_t left = i + 1;
        size_t right = i + 2;
        while (right <= cmd.size() && (cmd[right] == '_' || isalnum(cmd[right]))) {
          right++;
        }
        tp = cmd.substr(left, right - left);
        string search_res = env->search_var(tp);  //search for variable
        if (search_res == "|" || search_res == ">" || search_res == "<" || search_res == "2>") {
          res += search_res;
        }
        else {
          res += "$" + tp;
        }
        i = right - 1;
      }
    }
    else {
      res.push_back(cmd[i]);
    }
  }

  return res;
}

/*==================================================================================*/

/*
 */

/* ================================ Main Function ================================ */

extern char ** environ;  // environment from system, used for initialization

int main() {
  Env env(environ);
  myShell shell(&env);
  shell.run();

  return EXIT_SUCCESS;
}
