# KiteKit Browser

An intentionally tiny WebKitGTK browser for Linux.

## Requirements

- `gcc`
- `make`
- `pkg-config`
- WebKitGTK 4.1 development headers and GTK 3 development headers

On Debian or Ubuntu, that typically means installing:

```sh
sudo apt install build-essential pkg-config libwebkit2gtk-4.1-dev
```

## Build

```sh
make
```

The binary is written to `build/kitekit-browser`.

## Run

Launch with the default start page:

```sh
./build/kitekit-browser
```

Launch with a specific page:

```sh
./build/kitekit-browser https://example.com
```

You can also use:

```sh
make run
make run URL=https://example.com
```

## Behavior

- One window, one tab, one WebKit view
- Toolbar with Back, Forward, Reload, and a URL entry
- URLs without a scheme are loaded as `https://...`
- Invalid or unreachable pages fall back to WebKit's built-in error handling
