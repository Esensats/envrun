#include <cstdlib>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <pstream.h>
#include <unordered_map>
#include <unordered_set>
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
  Command(const std::vector<std::string> &aliases) {
    this->setAliases(aliases);
  }
  virtual ~Command() = default;
  virtual std::string description() const = 0;
  virtual std::string help() const { return this->description(); }
  virtual RunResult run(const std::vector<std::string> &args) = 0;

  // Add a subcommand with a single alias
  void addSubcommand(const std::string &alias,
                     std::unique_ptr<Command> command) {
    command->setAliases({alias});
    subcommands[alias] = std::move(command);
  }

  // Add a subcommand with multiple aliases
  void addSubcommand(const std::vector<std::string> &aliases,
                     std::shared_ptr<Command> command) {
    command->setAliases(aliases);
    for (const auto &alias : aliases) {
      subcommands[alias] = command;
    }
  }

  CommandsShared &getSubcommands() { return subcommands; }
  std::vector<std::string> getAliases() const { return aliases; }

protected:
  void setAliases(const std::vector<std::string> &aliases) {
    this->aliases = aliases;
  }
  std::vector<std::string> aliases;
  CommandsShared subcommands;

  static CommandOptions readOptions(const std::vector<std::string> &args,
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

  static std::vector<std::string>
  readOptionValues(const CommandOptions &options,
                   const std::vector<std::string> &keyAliases) {
    std::cout << "a1";
    for (const auto &key : keyAliases) {
      std::cout << "a_for_" << key;
      auto it = options.find(key);
      if (it != options.end()) {
        std::cout << "a2";
        return it->second;
      }
    }
    std::cout << "a3";
    return {};
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

  std::string description() const override { return "Prints the help message"; }

  RunResult run(const std::vector<std::string> &args) override {
    (void)args;
    std::cout << "Package for running apps with environment variables\n";
    std::cout << "Available commands:\n";
    std::unordered_set<std::shared_ptr<Command>> uniqueCommands;
    for (const auto &[key, command] : commands) {
      uniqueCommands.insert(command);
    }
    for (const auto &command : uniqueCommands) {
      const auto &aliases = command->getAliases();
      std::string aliasesStr = "";
      for (const auto &alias : aliases) {
        aliasesStr += alias + (&aliases.back() == &alias ? "" : ", ");
      }
      // std::cout << std::setw(20) << std::right << aliasesStr
      //           << command->description() << "\n";
      std::cout << std::format("  {:<15} | {}\n", aliasesStr,
                               command->description());
    }
    return {""};
  }

private:
  const CommandsShared &commands;
};

class RunProcessCommand : public Command {
public:
  std::string description() const override {
    return "Runs a shell command with the configured env variables";
  }

  RunResult run(const std::vector<std::string> &args) override {
    CommandOptions options = readOptions(args, "path");

    if (!options.contains("path") || options["path"].empty() ||
        options["-e"].size() % 2 != 0) {
      std::cerr << "Usage: " << this->getAliases()[0]
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
      argsStr += arg + (&exeArgs.back() == &arg ? "" : " ");
    }

    if (env.size() > 0) {
      for (const auto &entry : env) {
        setenv(entry.first.c_str(), entry.second.c_str(), 1);
      }
    }

    const std::string shellCmd = exePath + " " + argsStr;

    if (DEBUG)
      std::cout << "shellCmd: \"" << shellCmd << "\"\n";

    // run a process and create a streambuf that reads its stdout and stderr
    redi::ipstream proc(shellCmd,
                        redi::pstreams::pstdout | redi::pstreams::pstderr);
    std::string line;
    // read child's stdout
    while (std::getline(proc.out(), line))
      std::cout << (DEBUG ? "stdout: " : "") << line << '\n';
    // if reading stdout stopped at EOF then reset the state:
    if (proc.eof() && proc.fail())
      proc.clear();
    // read child's stderr
    while (std::getline(proc.err(), line))
      std::cerr << (DEBUG ? "stderr: " : "") << line << '\n';

    return {""};
  }
};

int main(int argc, char const *argv[]) {
  RootCommand root;

  root.addSubcommand({"-c", "--command"},
                     std::make_shared<RunProcessCommand>());

  root.addSubcommand({"-v", "--version"}, std::make_shared<VersionCommand>());

  root.addSubcommand({"-h", "--help"},
                     std::make_shared<HelpCommand>(root.getSubcommands()));

  std::vector<std::string> args(argv + 1, argv + argc);
  const auto &result = root.run(args);
  if (result.error.size() > 0) {
    return 1;
  }
  return 0;
}
