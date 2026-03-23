# crazyflie_ssd

This folder is the deployment/runtime side of the project.
It is a GAP8 / AI-deck application that wraps the generated network from
`../pytorch_ssd` with camera capture, preprocessing, inference execution,
and a small amount of debug transport code.

It is not yet a full end-to-end on-device detector. The runtime shell is in
place, but the current postprocess path is still a stub and recent GVSOC runs
show the app hanging before useful network progress.

## Runtime Data Flow

1. `camera_if.c`
   Captures a `324 x 244` Himax grayscale frame into L2 memory.
2. `preprocess.c`
   Center-crops to a square and resizes to the network input size using a
   simple nearest-neighbor style mapping.
3. `net_runner.c`
   Allocates the L2 arena and output buffer, calls `mem_init()`,
   `network_initialize()`, and then `network_run(...)`.
4. `pipeline.c`
   Orchestrates preprocess -> inference -> postprocess -> transport.
5. `ssd_postprocess.c`
   Currently a stub. It does not decode SSD boxes/classes yet.
6. `transport_if.c`
   Sends a compact UART packet with only:
   - a magic byte
   - frame id
   - checksum
   - the first 16 bytes of raw network output

## What Is Hand-Written vs Generated

Hand-written application code:

- `main.c`
- `app_config.h`
- `src/app_main.c`
- `src/camera_if.c`
- `src/preprocess.c`
- `src/net_runner.c`
- `src/pipeline.c`
- `src/ssd_postprocess.c`
- `src/transport_if.c`
- headers in `inc/`

Machine-generated or model-derived artifacts:

- `generated/`
  DORY-generated network sources, headers, and copied support files.
- `hex/`
  Model weights and input files flashed/read through readfs.
- `vars.mk`
  Enumerates the weight files that the GAP SDK app expects to mount.

If the model changes, assume `generated/`, `hex/`, and likely `vars.mk` need
to be refreshed rather than hand-edited.

## Current Compile-Time Contract

The main runtime assumptions live in `app_config.h`:

- `APP_NET_INPUT_W = 128`
- `APP_NET_INPUT_H = 128`
- `APP_NET_INPUT_C = 1`
- `APP_NET_INPUT_BYTES = 128 * 128 * 1`
- `APP_NET_OUTPUT_BYTES = 12`
- `APP_NET_ARENA_BYTES = 412000`
- camera frame = `324 x 244`
- UART baud = `115200`

If you redesign the model, these values are likely to change and the runtime
must be updated with them.

## Current State Of The Runtime

- `app_main.c` wires the basic async loop: capture frame, preprocess, run
  network, send compact debug output.
- `cpx_init`, tracing, and streamer setup are currently stubbed out.
- `ssd_postprocess.c` returns zero detections and does not perform SSD decode,
  score thresholding, or NMS.
- The UART transport currently sends raw debug bytes, not decoded detections.
- `BUILD/` and `docker_gvsoc_logs_*` are diagnostics and archived runs, not
  the source of truth for the app logic.

## Recent GVSOC Behavior

Recent logs under `docker_gvsoc_logs_20260316_122754/` suggest the runtime is
not reaching useful inference progress:

- frame count stays at `0`
- layer progress stays at `-1`
- the watcher remains at `CACHE_ALLOC_BEGIN`

That points more toward a memory allocation / generated-runtime initialization
problem than a postprocess bug.

## Relationship To `../pytorch_ssd`

This folder depends on artifacts produced by `../pytorch_ssd`.

If you redesign the model there, you will usually need to update:

- `app_config.h`
- `src/preprocess.c`
- `src/net_runner.c`
- `src/ssd_postprocess.c`
- `src/transport_if.c`
- regenerated files in `generated/`
- regenerated weights in `hex/`
- possibly `vars.mk`

The most important coupling points are:

- input width / height / channels
- output tensor size and layout
- memory footprint
- whether detections are decoded on-device or off-device

## Good Starting Points For A Redesign

If the goal is to simplify the system, decide these first:

- Do you still want SSD, or do you want a different detector/head?
- Do you want 1-channel grayscale end-to-end, or a 3-channel model?
- Do you want decoded boxes on GAP8, or only raw tensors sent out?
- Do you want the device app to be a real product path, or mainly a debug
  harness for GVSOC bring-up?

Once those decisions are made, this folder becomes much easier to reshape.
