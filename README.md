# envrun

A lightweight C++ command-line tool for running applications with custom environment variables.

## Description

`envrun` allows you to execute shell commands with a custom set of environment variables without permanently modifying your system environment. This is particularly useful for:

- Running applications with specific configuration
- Testing software with different environment setups
- Managing per-project environment variables
- Isolating environment changes from your main shell session

## Features

- Run any shell command with custom environment variables
- Simple command-line interface
- Support for passing arguments to the target application
- Real-time output streaming (stdout and stderr)
- Debug mode for troubleshooting
- Cross-platform compatible (Linux, macOS, Windows)

## Requirements

- C++23 compatible compiler (g++)
- Make
- PStreams library (included in `include/`)

## Installation

### Building from Source

1. Clone or download the project
2. Navigate to the project directory
3. Build the project:

```bash
make
```

The compiled binary will be available at `bin/envrun`.

### Alternative Build Commands

```bash
# Build and run immediately
make brun

# Clean build files
make clean

# Clean everything including the binary
make distclean

# Show build information
make info
```

## Usage

### Basic Syntax

```bash
envrun <command> [-e <key> <value>]... [-- <args>...]
```

### Commands

- `-c, --command` - Run a shell command with environment variables
- `-v, --version` - Show version information
- `-h, --help` - Show help message

### Examples

#### Run a command with environment variables

```bash
# Set NODE_ENV to production and run node app
envrun -c node -e NODE_ENV production -- app.js

# Set multiple environment variables
envrun -c python -e PYTHONPATH /custom/path -e DEBUG 1 -- script.py

# Run with custom database URL
envrun -c ./myapp -e DATABASE_URL postgresql://localhost/mydb
```

#### Debug mode

Set the `DEBUG` environment variable to see detailed execution information:

```bash
DEBUG=1 envrun -c node -e NODE_ENV development -- server.js
```

This will show:
- The command being executed
- All environment variables being set
- Command arguments
- Prefixed stdout/stderr output

### Command Structure

The tool expects arguments in this order:
1. The executable/command to run (`<command>`)
2. Environment variable pairs with `-e <key> <value>`
3. Optional arguments for the command after `--`

## Examples in Practice

### Web Development

```bash
# Start a development server with custom port and environment
envrun -c npm -e NODE_ENV development -e PORT 3000 -- start

# Run tests with specific database
envrun -c npm -e DATABASE_URL sqlite://test.db -- test
```

### Python Development

```bash
# Run Python script with custom module path
envrun -c python -e PYTHONPATH ./src -- main.py

# Run with development settings
envrun -c python -e DJANGO_SETTINGS_MODULE myproject.settings.dev -- manage.py runserver
```

### General Usage

```bash
# Run any command with environment isolation
envrun -c ./my-binary -e CONFIG_FILE ./dev.conf -e LOG_LEVEL debug

# Pass complex arguments
envrun -c gcc -e CC clang -- -o output main.c -lm
```

## Development

### Project Structure

```
envrun/
├── main.cpp           # Main source file with command implementations
├── Makefile          # Build configuration
├── include/          # Header files
│   └── pstream.h     # PStreams library for process management
├── bin/              # Compiled binaries
├── build/            # Build artifacts
└── temp/             # Temporary files (PStreams source)
```

### Code Organization

The project uses a command pattern with the following key classes:

- `Command` - Base class for all commands
- `RootCommand` - Main command dispatcher
- `RunProcessCommand` - Handles process execution with environment variables
- `VersionCommand` - Shows version information
- `HelpCommand` - Displays help text

### Building for Development

```bash
# Build with debug information
make

# Run with debug output
DEBUG=1 ./bin/envrun -c echo -e TEST_VAR hello -- "world"
```

## Dependencies

- **PStreams**: C++ library for running and communicating with child processes
  - Version: 1.0.4 (included)
  - Used for process management and I/O streaming

## Version

Current version: **0.0.1**

## License

[Add your license information here]

## Contributing

[Add contribution guidelines here]

## Troubleshooting

### Common Issues

1. **Command not found**: Ensure the target command is in your PATH or use absolute paths
2. **Permission denied**: Make sure the target executable has proper permissions
3. **Environment variables not taking effect**: Check the syntax and ensure you're using `-e key value` format

### Debug Mode

Use `DEBUG=1` to see detailed execution information:

```bash
DEBUG=1 envrun -c your-command -e YOUR_VAR value
```

This will show exactly what command is being executed and with which environment variables.
