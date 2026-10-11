# Socket Marker — Agent Instructions

## Project Overview

A single-file Linux `LD_PRELOAD` shared library (`setmark.c`) that intercepts `socket()` via `dlsym(RTLD_NEXT)` and applies `SO_MARK` to every created socket. See [README.md](README.md) for usage and policy routing examples.

## Build

```bash
gcc -O2 -Wall -Wextra \
    -shared -fPIC \
    -o setmark.so setmark.c \
    -ldl
```

No build system, Makefile, or external dependencies — just `gcc` and `libdl`.

## Key Conventions

- **`_GNU_SOURCE`** must be defined first (required for `RTLD_NEXT` and glibc extensions).
- Initialization happens in a `__attribute__((constructor))` function; keep it side-effect-free beyond reading `SO_MARK` and resolving the real `socket` symbol.
- `SO_MARK` env var is parsed with `strtoul` with strict validation (`errno`, `end` pointer, `UINT32_MAX` bound check); maintain that pattern for any new env var parsing.
- Errors go to `stderr` with the `[Socket Marker]` prefix — keep that prefix consistent.
- The library is **Linux-only** (`SOL_SOCKET`/`SO_MARK` are Linux-specific). Do not introduce portability abstractions.

## Pitfalls

- `SO_MARK` requires `CAP_NET_ADMIN` (typically `sudo`). Tests or examples that skip this will silently fail.
- `real_socket` can be `NULL` if `dlsym` fails; always guard before calling it.
- Do not call `socket()` inside the constructor — it would recurse before `real_socket` is resolved.
