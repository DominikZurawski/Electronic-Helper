# Docker (Fedora 43) — build/test

## Build image

```bash
docker build -t ppe-dev .
```

## Run container (CLI)

```bash
docker run --rm -it ppe-dev
```

## Run GUI (Linux, X11)

```bash
docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  ppe-dev
```

Then inside container:

```bash
./build/ppe_gui
```
