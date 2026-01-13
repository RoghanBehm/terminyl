# Terminyl

## Priority
- `--help`, `--version`, `--check` `--ast`, `--ir` CLI flags
- If else, foreach, continue, break
- Markdown-style lists (ordered + unordered)
- Colour + styling built-ins
    - `#style("text", fg: "red", bg: "black", bold: true)`
    - `#style(fg: "yellow") { ... }`
- Tables


## Required
- Fenced code blocks
- Syntax highlight (probably just shell out to `bat`)
- Images via chafa
- Make variable bindings block-level rather than inline
- `#include file.termy`

## Sugar
- Standard library
- Bytecode VM
- Refactor AST shared_ptrs to raw ptrs + arena
- Incremental re-render