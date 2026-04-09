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

Run the runtime policy tests with:

```sh
make test
```

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

## Troubleshooting

- KiteKit prefers Wayland automatically when no backend is forced.
- Running GTK through X11 inside a Wayland session can make scrolling and compositing feel rougher.
- If KiteKit detects that degraded path, it shows an in-window warning bar with guidance.
- Use `G_MESSAGES_DEBUG=all ./build/kitekit-browser` to inspect the chosen backend and WebKit runtime settings.

## Manual Scroll Check

Open the bundled fixture with:

```sh
./build/kitekit-browser file://$(pwd)/tests/fixtures/scroll-stress.html
```

It includes a tall page, a sticky header, and a nested scroll container so you can compare wheel and trackpad scrolling without network variability.
