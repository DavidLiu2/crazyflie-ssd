/*
 * pipeline.c
 * Responsibility: Coordinate preprocess -> inference -> postprocess -> transport.
 */

#include "pipeline.h"

#include <stdio.h>

#include "app_config.h"
#include "mem_debug.h"
#include "net_runner.h"
#include "preprocess.h"
#include "ssd_postprocess.h"
#include "transport_if.h"

void pipeline_process_frame(const camera_frame_t *frame)
{
  person_det_t dets[4];
  uint8_t *net_in;
  const uint8_t *net_out;
  size_t net_out_size;

  if (frame == NULL || frame->data == NULL) {
    printf("ERROR: invalid camera frame\n");
    return;
  }

  net_in = net_runner_get_input_buffer();
  if (net_in == NULL) {
    printf("ERROR: network input buffer not initialized\n");
    return;
  }

  preprocess_camera_to_net_input(
      frame->data,
      (int)frame->width,
      (int)frame->height,
      net_in);
  printf("preprocess done\n");

  if (net_runner_run()) {
    printf("ERROR: network run failed\n");
    return;
  }

  net_out = net_runner_get_output_buffer();
  net_out_size = net_runner_get_output_size();

  mem_debug_dump_bytes("OUTPUT_DUMP", net_out, net_out_size, APP_IO_DUMP_BYTES);
  printf("output bytes dumped\n");

  (void)ssd_postprocess_run(net_out, net_out_size, dets, sizeof(dets) / sizeof(dets[0]));

  /* Preserve compact raw-output UART debug transport. */
  transport_if_send_compact_packet(frame->frame_id, net_out, net_out_size);
}