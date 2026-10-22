/*
 * Native unit tests for the circular-pad zone math.
 *
 * Points are taken from the TM035035 datasheet (v1.2, Figure 6): the active
 * area is the ellipse inscribed in x 128..1920, y 64..1472, i.e. semi-axes
 * Rx = 896 and Ry = 704 coordinate units. A physical clock position maps to
 * (Rx * cos(a), Ry * sin(a)) with a measured from 3 o'clock, y positive down.
 */

#include <assert.h>
#include <stdint.h>

#include <zmk/cirque_zone_math.h>

int main(void)
{
	/* Axis mapping: centre -> 0, borders -> +/-100, outside clamped. */
	assert(zmk_cirque_axis_percent(1024, 128, 1920) == 0);
	assert(zmk_cirque_axis_percent(1920, 128, 1920) == 100);
	assert(zmk_cirque_axis_percent(128, 128, 1920) == -100);
	assert(zmk_cirque_axis_percent(2000, 128, 1920) == 100);
	assert(zmk_cirque_axis_percent(0, 128, 1920) == -100);
	assert(zmk_cirque_axis_percent(768, 64, 1472) == 0);
	assert(zmk_cirque_axis_percent(1472, 64, 1472) == 100);

	/* Degenerate range must not divide by zero. */
	assert(zmk_cirque_axis_percent(5, 10, 10) == 0);

	/* Rim points at 3, 4, 5 and 5:30 o'clock are all in the outer ring. */
	assert(zmk_cirque_in_outer_ring(100, 0, 70));  /* 3:00 */
	assert(zmk_cirque_in_outer_ring(87, 50, 70));  /* 4:00 */
	assert(zmk_cirque_in_outer_ring(50, 87, 70));  /* 5:00 */
	assert(zmk_cirque_in_outer_ring(26, 97, 70));  /* 5:30 */

	/* The old rectangular band required x_pct >= 52; the 5 o'clock rim has
	 * x_pct = 50 and used to be missed. */
	assert(!(50 >= 52));
	assert(zmk_cirque_in_outer_ring(50, 87, 70));

	/* Inner area stays with the pointer, not the scroll latch. */
	assert(!zmk_cirque_in_outer_ring(0, 0, 70));
	assert(!zmk_cirque_in_outer_ring(60, 0, 70));
	assert(!zmk_cirque_in_outer_ring(30, 40, 70)); /* exactly 50% radius */

	/* ring_pct = 0 disables the ring test (legacy mode). */
	assert(!zmk_cirque_in_outer_ring(100, 0, 0));
	assert(!zmk_cirque_in_outer_ring(100, 0, -1));

	/* ring_pct > 100 is clamped to the rim. */
	assert(!zmk_cirque_in_outer_ring(99, 0, 200));
	assert(zmk_cirque_in_outer_ring(100, 0, 200));

	return 0;
}
