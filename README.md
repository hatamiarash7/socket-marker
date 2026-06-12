# Socket Marker

A lightweight `LD_PRELOAD` library that automatically applies a Linux `SO_MARK`
to every socket created by an application — without modifying the application.

Typical use-cases:

- Policy-based routing (`ip rule fwmark`)
- Multi-WAN / VPN traffic segregation
- Per-process network policies
- Testing routing rules without touching application code

## How It Works

The library intercepts the `socket()` libc call via `LD_PRELOAD` and immediately
calls:

```c
setsockopt(fd, SOL_SOCKET, SO_MARK, &mark, sizeof(mark));
```

The mark value is read once at library load time from the `SO_MARK` environment
variable, falling back to the compiled-in default (`1234`) when the variable is
absent or invalid.

## Requirements

- Linux kernel ≥ 2.6.25 (`SO_MARK` support)
- glibc (or a compatible `LD_PRELOAD`-capable libc)
- `CAP_NET_ADMIN` to set `SO_MARK` (typically `sudo`)

## Building

```bash
make
```

Or without Make:

```bash
gcc -O2 -Wall -Wextra \
    -shared -fPIC \
    -o setmark.so setmark.c \
    -ldl
```

To override the compiler or flags:

```bash
make CC=clang CFLAGS="-O3 -Wall"
```

## Usage

### Default mark (1234)

```bash
sudo LD_PRELOAD=$PWD/setmark.so curl https://example.com
```

### Custom mark via environment variable

```bash
sudo SO_MARK=100 LD_PRELOAD=$PWD/setmark.so curl https://example.com
```

```bash
sudo SO_MARK=4611 LD_PRELOAD=$PWD/setmark.so ping 1.1.1.1
```

The mark can be any value in the range `0`–`4294967295` (`UINT32_MAX`).
An invalid value (non-numeric, negative, out of range) is rejected and the
default mark is used instead; a warning is printed to stderr.

## Policy Routing Example

The following routes all traffic from a process through a dedicated gateway.

**1. Add a named routing table:**

```bash
echo "100 marked" >> /etc/iproute2/rt_tables
```

**2. Add a default route for that table:**

```bash
ip route add default via 192.168.1.1 table marked
```

**3. Map fwmark 100 to the table:**

```bash
ip rule add fwmark 100 table marked
```

**4. Run the application:**

```bash
sudo SO_MARK=100 LD_PRELOAD=$PWD/setmark.so curl https://example.com
```

All sockets created by the process will carry mark `100` and follow the
`marked` routing table.

## Installation

```bash
sudo make install          # installs to /usr/local/lib/setmark.so
sudo make install PREFIX=/usr
sudo make uninstall
```

## Testing

```bash
make test
```

Tests require `CAP_NET_ADMIN` to verify that `SO_MARK` was actually applied.
Without it, the mark-value test is automatically skipped; the remaining tests
(socket creation, errno preservation) run without elevated privileges.

```bash
sudo make test             # full test suite including SO_MARK verification
```

## Verification

Check that a socket carries the expected mark:

```bash
ss -tupne | grep -i mark
```

Inspect policy routing rules:

```bash
ip rule show
ip route show table marked
```

Monitor packets:

```bash
tcpdump -i any -n
```

## Limitations

| Limitation | Reason |
|---|---|
| Statically linked binaries | `LD_PRELOAD` is a dynamic-linker feature |
| Setuid / setgid binaries | Linux drops `LD_PRELOAD` for security |
| Sockets created before library load | Only new `socket()` calls are intercepted |
| No `CAP_NET_ADMIN` | `setsockopt(SO_MARK)` fails with `EPERM`; the socket is still returned to the caller |

A dynamically linked binary can be checked with:

```bash
ldd <binary>
# "not a dynamic executable" → LD_PRELOAD will not work
```

## Troubleshooting

**Verify the library is loaded:**

```bash
LD_PRELOAD=$PWD/setmark.so env | grep LD_PRELOAD
```

**Check initialization output:**

The library always prints a line to stderr on load, e.g.:

```
[Socket Marker] Initialized (mark=100)
```

If this line does not appear, the library was not loaded.

**Marking fails silently:**

`setsockopt(SO_MARK)` requires `CAP_NET_ADMIN`. Without it, the library logs
a warning per socket but the application continues to run normally:

```
[Socket Marker] setsockopt(fd=3, mark=100) failed: Operation not permitted
```

Run with `sudo` or grant the binary `CAP_NET_ADMIN`:

```bash
sudo setcap cap_net_admin+ep <binary>
```

**Check current capabilities:**

```bash
capsh --print
```
