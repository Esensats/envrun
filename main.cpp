#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <pstream.h>
#include <unordered_map>
#include <vector>

struct RunResult {
  std::string error;
};

using CommandOptions = std::map<std::string, std::vector<std::string>>;

class Command;

using CommandsShared =
    std::unordered_map<std::string, std::shared_ptr<Command>>;
using Commands = std::unordered_map<std::string, std::unique_ptr<Command>>;

class Command {
public:
  Command() = default;
  Command(const std::string &name) : name(name) {}
  virtual ~Command() = default;
  virtual std::string description() const = 0;
  virtual std::string help() const { return this->description(); }
  virtual RunResult run(const std::vector<std::string> &args) = 0;

  void addSubcommand(const std::string &name,
                     std::shared_ptr<Command> command) {
    command->setName(name);
    subcommands[name] = command;
  }

  CommandsShared &getSubcommands() { return subcommands; }
  std::string getName() const { return name; }

protected:
  void setName(const std::string &name) { this->name = name; }
  std::string name;
  CommandsShared subcommands;

  CommandOptions readOptions(const std::vector<std::string> &args,
                             const std::string &prefixKey) {
    int firstOptionIndex = -1;
    CommandOptions options;
    for (int i = 0; i < (int)args.size(); i++) {
      std::string arg = args[i];
      if (arg[0] == '-') {
        if (firstOptionIndex == -1) {
          firstOptionIndex = i;
        }
        if (arg == "--") {
          i++;
          options["--"] =
              std::vector<std::string>(args.begin() + i, args.end());
          break;
        }
        options[arg] = std::vector<std::string>();
        i++;
        while (i < (int)args.size() && args[i][0] != '-') {
          options[arg].push_back(args[i]);
          i++;
        }
        i--;
      }
    }
    if (firstOptionIndex != -1) {
      options[prefixKey] = std::vector<std::string>(
          args.begin(), args.begin() + firstOptionIndex);
    }
    return options;
  }
};

class RootCommand : public Command {
public:
  std::string description() const override { return ""; }

  RunResult run(const std::vector<std::string> &args) override {
    if (args.size() == 0) {
      std::cerr << "No command provided. Use --help for help.\n";
      return {"No command provided"};
    }

    auto command = subcommands.find(args[0]);
    if (command != subcommands.end()) {
      std::vector<std::string> subArgs(args.begin() + 1, args.end());
      return command->second->run(subArgs);
    } else {
      std::cerr << "Unknown command: " << args[0] << "\n";
      return {"Unknown command"};
    }
  }
};

class VersionCommand : public Command {
public:
  std::string description() const override {
    return "Prints the version of the program";
  }

  RunResult run(const std::vector<std::string> &args) override {
    (void)args;
    std::cout << "0.0.1\n";
    return {""};
  }
};

class HelpCommand : public Command {
public:
  HelpCommand(const CommandsShared &commands) : commands(commands) {}
  HelpCommand(const CommandsShared &commands, std::string name)
      : commands(commands) {
    setName(name);
  }

  std::string description() const override { return "Prints the help message"; }

  RunResult run(const std::vector<std::string> &args) override {
    (void)args;
    std::cout << "Package for running apps with environment variables\n";
    std::cout << "Available commands:\n";
    for (const auto &command : commands) {
      std::cout << command.second->getName() << ": "
                << command.second->description() << "\n";
    }
    return {""};
  }

private:
  const CommandsShared &commands;
};

class RunProcessCommand : public Command {
public:
  std::string description() const override {
    return "Runs a child process with the configured env variables";
  }

  RunResult run(const std::vector<std::string> &args) override {
    auto options = readOptions(args, "path");

    if (options["path"].size() != 1 || options["-e"].size() % 2 != 0) {
      std::cerr << "Usage: " << this->getName()
                << " <path> [-e (<key> <value>)...] [-- <args>...]\n";
      return {"Invalid arguments"};
    }

    std::string exePath = options["path"][0];
    std::vector<std::string> exeArgs = options["--"];
    std::unordered_map<std::string, std::string> env;
    for (int i = 0; i < (int)options["-e"].size(); i += 2) {
      env[options["-e"][i]] = options["-e"][i + 1];
    }

    // ...
    // Run the process
    bool DEBUG = std::getenv("DEBUG") != nullptr;
    if (DEBUG) {
      std::cout << "Running " << exePath << " with env:\n";
      for (const auto &entry : env) {
        std::cout << entry.first << "=" << entry.second << "\n";
      }
      std::cout << "Args:\n";
      for (const auto &arg : exeArgs) {
        std::cout << arg << "\n";
      }
    }

    std::string argsStr = "";

    for (const auto &arg : exeArgs) {
      argsStr += arg + " ";
    }

    std::string envStr = "(export ";

    for (const auto &entry : env) {
      envStr += entry.first + "=\"" + entry.second + "\" ";
    }

    envStr += "&& ";

    std::string shellCmd = envStr + exePath + " " + argsStr + ")";

    if (DEBUG)
      std::cout << "shellCmd: " << shellCmd << "\n";

    // run a process and create a streambuf that reads its stdout and stderr
    redi::ipstream proc(shellCmd,
                        redi::pstreams::pstdout | redi::pstreams::pstderr);
    std::string line;
    // read child's stdout
    while (std::getline(proc.out(), line))
      std::cout << "stdout: " << line << '\n';
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail())
      proc.clear();
    // read child's stderr
    while (std::getline(proc.err(), line))
      std::cerr << "stderr: " << line << '\n';
    // ...

    return {""};
  }
};

int main(int argc, char const *argv[]) {
  RootCommand root;
  root.addSubcommand("--version", std::make_unique<VersionCommand>());
  root.addSubcommand("-v", std::make_unique<VersionCommand>());
  std::shared_ptr<HelpCommand> helpCommand =
      std::make_shared<HelpCommand>(root.getSubcommands(), "help");
  root.addSubcommand("--help", helpCommand);
  root.addSubcommand("-h", helpCommand);
  root.addSubcommand("help", helpCommand);
  helpCommand->addSubcommand("run", std::make_shared<RunProcessCommand>());
  root.addSubcommand("run", std::make_unique<RunProcessCommand>());

  std::vector<std::string> args(argv + 1, argv + argc);
  root.run(args);
}
