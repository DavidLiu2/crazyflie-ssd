/*
 * pipeline.c
 * Responsibility: Coordinate preprocess -> inference -> decode -> transport.
 */

#include "pipeline.h"

#include <string.h>

#include "pmsis.h"

#include "app_config.h"
#include "app_log.h"
#include "follow_output.h"
#include "mem_debug.h"
#include "net_runner.h"
#include "preprocess.h"
#include "transport_if.h"

int pipeline_process_frame(
    const camera_frame_t *frame,
    pipeline_result_t *result)
{
  uint32_t t_start_us;
  uint32_t t_stage_us;
  uint8_t *net_in;
  const uint8_t *net_out;
  size_t net_out_size;

  if (result != NULL) {
    memset(result, 0, sizeof(*result));
  }

  if (frame == NULL || frame->data == NULL || result == NULL) {
    app_log_error("pipeline received an invalid frame");
    return -1;
  }

  t_start_us = pi_time_get_us();
  result->frame_id = frame->frame_id;
  result->capture_us = frame->capture_us;

  net_in = net_runner_get_input_buffer();
  if (net_in == NULL) {
    app_log_error("network input buffer not initialized");
    return -1;
  }

  t_stage_us = pi_time_get_us();
  preprocess_camera_to_net_input(
      frame->data,
      (int)frame->width,
      (int)frame->height,
      net_in);
  result->preprocess_us = pi_time_get_us() - t_stage_us;

  t_stage_us = pi_time_get_us();
  if (net_runner_run()) {
    app_log_error("network run failed");
    return -1;
  }
  result->inference_us = pi_time_get_us() - t_stage_us;

  net_out = net_runner_get_output_buffer();
  net_out_size = net_runner_get_output_size();
  result->output_checksum = mem_debug_checksum16(net_out, net_out_size);

#if APP_DEBUG && APP_LOG_OUTPUT_DUMP
  mem_debug_dump_bytes("OUTPUT_DUMP", net_out, net_out_size, APP_IO_DUMP_BYTES);
#endif

  t_stage_us = pi_time_get_us();
  if (follow_output_from_buffer(net_out, net_out_size, &result->follow)) {
    app_log_error("follow output decode failed");
    return -1;
  }
  result->postprocess_us = pi_time_get_us() - t_stage_us;

  if (transport_if_send_follow_result(frame->frame_id, &result->follow) == 0) {
    result->app_packet_sent = 1u;
  }

  result->total_us = pi_time_get_us() - t_start_us;
  return 0;
}
