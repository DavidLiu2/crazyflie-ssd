/*
 * Minimal compatibility shim for DORY-generated sources in the AI-deck
 * FreeRTOS build. The generated kernels include "pulp.h", but on this build
 * the required vector types, cluster helpers, and architecture constants come
 * from PMSIS.
 */

#ifndef __PULP_H__
#define __PULP_H__

#include "pmsis.h"

#endif
