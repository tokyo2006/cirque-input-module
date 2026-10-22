/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Geometry helpers for circular Cirque sensors (e.g. the TM035035, which has a
 * 31 mm circular sensing area inside a 35 mm module).
 *
 * The Pinnacle reports absolute coordinates on a 2048 x 1536 grid. On a round
 * pad the reachable area is the ellipse inscribed in the clipped active range
 * (TM035035 datasheet v1.2, Figure 6), so rectangular edge zones do not follow
 * the rim: at 4-6 o'clock the rim curves inward in X while still sitting at the
 * outer radius, and a vertical "x >= width - zone" band misses it.
 *
 * These helpers work in percent-of-semi-axis space: -100..100 on each axis,
 * with 0 at the ellipse centre and +/-100 on the active-range border.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Map a coordinate in [min, max] to [-100, 100] around the centre. Values
 * outside the range are clamped so electrical noise near the border cannot
 * overflow the ring test. */
static inline int32_t zmk_cirque_axis_percent(int32_t coord, int32_t min, int32_t max)
{
	int32_t half;

	if (max <= min) {
		return 0;
	}

	half = (max - min) / 2;
	coord = coord < min ? min : (coord > max ? max : coord);

	return (coord - min - half) * 100 / half;
}

/* True when (x_pct, y_pct) is in the outer ring that starts at ring_pct of the
 * radius (100 = the rim itself). ring_pct <= 0 disables the test, so callers
 * can keep the legacy rectangular behaviour. */
static inline bool zmk_cirque_in_outer_ring(int32_t x_pct, int32_t y_pct, int32_t ring_pct)
{
	int64_t distance_sq;
	int64_t ring;

	if (ring_pct <= 0) {
		return false;
	}

	ring = ring_pct > 100 ? 100 : ring_pct;
	distance_sq = (int64_t)x_pct * x_pct + (int64_t)y_pct * y_pct;

	return distance_sq >= ring * ring;
}
