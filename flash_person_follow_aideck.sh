#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
WORKSPACE_ROOT=$(cd "${SCRIPT_DIR}/.." && pwd)

APP_DIR_NAME="crazyflie_ssd"
APP_DIR_ARG="../crazyflie_ssd"
MAKE_EXAMPLE_REL="${MAKE_EXAMPLE_REL:-aideck-gap8-examples/tools/build/make-example}"
DOCKER_IMAGE="${DOCKER_IMAGE:-bitcraze/aideck}"
DEFAULT_URI="${CRAZYFLIE_URI:-radio://0/80/2M/E7E7E7E7E7}"
FLASH_IMAGE_PATH="${WORKSPACE_ROOT}/${APP_DIR_NAME}/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img"

uri="${DEFAULT_URI}"
if [[ $# -gt 0 && "$1" == *"://"* ]]; then
  uri="$1"
  shift
fi

build_args=(clean build image "$@")

if [[ ! -f "${WORKSPACE_ROOT}/${MAKE_EXAMPLE_REL}" ]]; then
  echo "Missing build helper: ${WORKSPACE_ROOT}/${MAKE_EXAMPLE_REL}" >&2
  echo "Set MAKE_EXAMPLE_REL if your aideck example repo lives somewhere else." >&2
  exit 1
fi

echo "Building ${APP_DIR_NAME} with Docker image ${DOCKER_IMAGE}"
docker run --rm \
  -v "${WORKSPACE_ROOT}:/workspace" \
  -w /workspace \
  "${DOCKER_IMAGE}" \
  "${MAKE_EXAMPLE_REL}" \
  "${APP_DIR_ARG}" \
  "${build_args[@]}"

echo
echo "Flash image:"
echo "  ${FLASH_IMAGE_PATH}"
echo
echo "Flash command:"
echo "  python -m cfloader flash \"${FLASH_IMAGE_PATH}\" deck-bcAI:gap8-fw -w \"${uri}\""

if [[ ! -f "${FLASH_IMAGE_PATH}" ]]; then
  echo
  echo "Build finished but the flash image was not found at the expected path." >&2
  exit 2
fi
