# Socket Marker

A lightweight `LD_PRELOAD` library that automatically applies a Linux `SO_MARK` to every socket created by an application.

This is useful for:

- Policy-based routing (`ip rule fwmark`)
- Traffic segregation
- Multi-WAN routing
- VPN routing
- Per-process network policies
- Testing routing rules without modifying application code

## How It Works

The library intercepts the libc `socket()` function using `LD_PRELOAD`.

Whenever an application creates a new socket, the library automatically executes:

```c
setsockopt(fd, SOL_SOCKET, SO_MARK, &mark, sizeof(mark));
```

The mark value is read from the `SO_MARK` environment variable.

## Requirements

- Linux
- glibc-compatible `LD_PRELOAD`
- Root privileges or `CAP_NET_ADMIN`

`SO_MARK` requires elevated privileges.

## Building

```bash
gcc -O2 -Wall -Wextra \
    -shared -fPIC \
    -o setmark.so setmark.c \
    -ldl
```

## Usage

### Default Mark

The library uses the default mark configured in the source code when `SO_MARK` is not specified.

```bash
sudo LD_PRELOAD=$PWD/setmark.so curl https://example.com
```

### Custom Mark

Set the mark dynamically through an environment variable:

```bash
sudo SO_MARK=100 \
     LD_PRELOAD=$PWD/setmark.so \
     curl https://example.com
```

Another example:

```bash
sudo SO_MARK=4611 \
     LD_PRELOAD=$PWD/setmark.so \
     ping 1.1.1.1
```

## Policy Routing Example

Create a routing table:

```bash
echo "100 marked" >> /etc/iproute2/rt_tables
```

Add a default route:

```bash
ip route add default via 192.168.1.1 table marked
```

Create a rule for packets marked with `100`:

```bash
ip rule add fwmark 100 table marked
```

Run an application:

```bash
sudo SO_MARK=100 \
     LD_PRELOAD=$PWD/setmark.so \
     curl https://example.com
```

All traffic generated through sockets created by the process will follow the `marked` routing table.

## Verification

View sockets:

```bash
ss -tupne
```

Inspect policy routing rules:

```bash
ip rule show
```

Inspect routes:

```bash
ip route show table marked
```

Monitor packets:

```bash
tcpdump -i any
```

## Limitations

### Statically Linked Applications

`LD_PRELOAD` only works with dynamically linked binaries.

This will not work:

```bash
ldd <binary>
```

If the output contains:

```text
not a dynamic executable
```

### Setuid Binaries

Linux ignores `LD_PRELOAD` for setuid/setgid binaries for security reasons.

### Existing Sockets

Only sockets created after the library is loaded are marked.

### Privileges

Setting `SO_MARK` requires:

- root privileges, or
- `CAP_NET_ADMIN`

Without sufficient permissions, the application continues to run but socket marking will fail.

## Troubleshooting

### Verify the library is loaded

```bash
LD_PRELOAD=$PWD/setmark.so env | grep LD_PRELOAD
```

### Enable debug messages

The library prints initialization and socket-marking errors to stderr.

Example:

```text
[Socket Marker] Initialized (mark=100)
```

### Check capabilities

```bash
capsh --print
```

Verify that `CAP_NET_ADMIN` is available.
