# Code Style Guidelines

This document outlines the coding style and conventions for the KiraWavTar project.

## Formatting

- **clang-format**: Use `clang-format` for all C++ header and source files.
- **Format on Save**: Developers should enable "Format on Save" in their IDE (e.g., VS Code) to ensure consistency.

## Naming Conventions

- **Member Variables**: All non-static member variables must be prefixed with `m_` (e.g., `m_memberVariable`).
- **Static Member Variables**: All static member variables must be prefixed with `s_` (e.g., `s_staticVariable`).

## Class Member Ordering

To maintain readability, class members should be grouped by access level and split between functions and variables. The preferred order is:

1.  **`public:`** (functions/constructors/destructors)
2.  **`public slots:`**
3.  **`signals:`**
4.  **`public:`** (member variables)
5.  **`protected:`** (functions)
6.  **`protected slots:`**
7.  **`protected:`** (member variables)
8.  **`private:`** (functions)
9.  **`private slots:`**
10. **`private:`** (member variables)

Separate function blocks from variable blocks with clear access specifiers, even if the access level is the same.
