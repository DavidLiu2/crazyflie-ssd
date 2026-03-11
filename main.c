/*
 * main.c
 * Responsibility: Minimal boot entrypoint that hands control to app_main.
 */

#include "pmsis.h"
#include "app_main.h"

int main(void)
{
  pmsis_kickoff((void *)app_main_task);
  return 0;
}