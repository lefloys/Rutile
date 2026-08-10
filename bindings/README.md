# Rutile language bindings

Each subdirectory provides Rutile headers for one host language.

| Directory | Language | Build system | Status     |
| --------- | -------- | ------------ | ---------- |
| `c/`      | C99      | CMake        | Available as `Rutile::rutile`; defines the ABI |
| `cpp/`    | C++      | CMake        | Available as `Rutile::cpp` |

The C binding is the source of truth. `rutile.h` defines the ABI and the
runtime in `runtime/` implements it. The C++ binding is a C++ interface over
that API.

Both available bindings are CMake interface targets and are installed with
Rutile.
