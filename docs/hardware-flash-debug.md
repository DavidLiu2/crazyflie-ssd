# AI-deck Flash and Debug

This app now builds as a normal AI-deck GAP8 flash image and sends runtime logs over CPX to the Crazyflie console path.

## Build

Use the WSL helper from `crazyflie_ssd/`:

```bash
bash ./flash_person_follow_aideck.sh
```

That runs the Bitcraze-style Docker build flow through `aideck-gap8-examples/tools/build/make-example` and builds:

```text
crazyflie_ssd/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img
```

Direct Docker build:

```bash
docker run --rm \
  -v "${PWD}/..:/workspace" \
  -w /workspace \
  bitcraze/aideck \
  aideck-gap8-examples/tools/build/make-example \
  ../crazyflie_ssd \
  clean build image
```

Useful build overrides:

```bash
bash ./flash_person_follow_aideck.sh radio://0/80/2M/E7E7E7E7E7 APP_DEBUG=0
bash ./flash_person_follow_aideck.sh radio://0/80/2M/E7E7E7E7E7 APP_ENABLE_CPX_APP_PACKET_TX=1
```

## Flash

The helper script prints the exact flash command after a successful build:

```bash
python -m cfloader flash \
  crazyflie_ssd/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img \
  deck-bcAI:gap8-fw \
  -w radio://0/80/2M/E7E7E7E7E7
```

You can also pass the URI to the helper directly:

```bash
bash ./flash_person_follow_aideck.sh radio://0/80/2M/E7E7E7E7E7
```

## Logs

Normal flashed usage:

- Open `CFclient`
- Connect to the Crazyflie
- Watch the `Console` tab
- Look for lines prefixed with `PFOLLOW:`

Expected startup logs:

```text
CPX: GAP8: PFOLLOW: app start
CPX: GAP8: PFOLLOW: CPX init OK
CPX: GAP8: PFOLLOW: camera init OK
CPX: GAP8: PFOLLOW: model init OK
CPX: GAP8: PFOLLOW: buffers ready
CPX: GAP8: PFOLLOW: entering inference loop
```

Expected runtime summary with `APP_DEBUG=1`:

```text
CPX: GAP8: PFOLLOW: frame=10 cap=33.000ms infer=27.500ms total=28.100ms vis=1 conf=0.998 x=+0.062 scale=0.341 raw=[2048,11172,228512] drop=0 camerr=0 pkt=0 cksum=0x02a4
```

Runtime log fields:

- `frame`: processed frame number
- `cap`: camera capture time
- `infer`: network execution time
- `total`: preprocess + inference + decode time
- `vis`: thresholded visible flag
- `conf`: visibility confidence derived from the visibility head
- `x`: horizontal follow error
- `scale`: size or distance proxy
- `raw`: raw `int32` head outputs `[x, scale, visibility]`
- `drop`: dropped frames because inference was still busy
- `camerr`: camera timeout count
- `pkt`: whether a CPX app packet was sent to STM32 for that frame

With `APP_DEBUG=0`, startup and error logs remain enabled while periodic frame summaries are suppressed.

## CPX Transport Split

The project now keeps CPX console logging and CPX app-packet transport separate:

- CPX console logs go through `transport_if_console_write()` and appear in the CFclient console.
- Future STM32 follow packets go through `transport_if_send_follow_result()`.
- CPX app-packet TX is compiled in but off by default with `APP_ENABLE_CPX_APP_PACKET_TX=0`.

## JTAG or Host `printf`

For host-driven debug runs, you can still use the GAP SDK `io=host` path for `printf` output to a terminal. Example:

```bash
docker run --rm \
  -v "${PWD}/..:/workspace" \
  -w /workspace \
  bitcraze/aideck \
  aideck-gap8-examples/tools/build/make-example \
  ../crazyflie_ssd \
  io=host build
```

For normal flashed usage on real hardware, the preferred visible debug path is the CPX console output described above.

## WSL Notes

- `flash_person_follow_aideck.sh` is the recommended entrypoint when Docker is installed inside WSL.
- Docker still needs to be available inside WSL, since the actual GAP8 build runs there.
- The helper defaults to `bitcraze/aideck`. If you have a custom autotiler image, override it with `DOCKER_IMAGE=...`.
- `flash_person_follow_aideck_wsl.ps1` is still available if you want a Windows-side wrapper that jumps into WSL for you.
