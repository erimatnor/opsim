#include <stdlib.h>
#include <stdio.h>

#include "point.h"

void p2_random(point2_t p)
{
	
}

#define SAME_SIGNS( a, b ) ((a < 0 && b < 0) || (a > 0 && b > 0) || (a == b))

/* Based on Mukesh Prasad's xlines.c */
int point_line_intersect(const point2_t p1, const point2_t p2, 
			 const point2_t p3, const point2_t p4, 
			 point2_t intersect)
{
    float a1, a2, b1, b2, c1, c2; /* Coefficients of line eqns. */
    float r1, r2, r3, r4;         /* 'Sign' values */
    float denom, offset, num;     /* Intermediate values */

    /* Compute a1, b1, c1, where line joining points 1 and 2
     * is "a1 x  +  b1 y  +  c1  =  0".
     */

    a1 = p2[1] - p1[1];
    b1 = p1[0] - p2[0];
    c1 = p2[0] * p1[1] - p1[0] * p2[1];

    /* Compute r3 and r4.
     */

    r3 = a1 * p3[0] + b1 * p3[1] + c1;
    r4 = a1 * p4[0] + b1 * p4[1] + c1;

    /* Check signs of r3 and r4.  If both point 3 and point 4 lie on
     * same side of line 1, the line segments do not intersect.
     */

    if (r3 != 0 && r4 != 0 && SAME_SIGNS(r3, r4))
	    return 0;

    /* Compute a2, b2, c2 */

    a2 = p4[1] - p3[1];
    b2 = p3[0] - p4[0];
    c2 = p4[0] * p3[1] - p3[0] * p4[1];

    /* Compute r1 and r2 */

    r1 = a2 * p1[0] + b2 * p1[1] + c2;
    r2 = a2 * p2[0] + b2 * p2[1] + c2;

    /* Check signs of r1 and r2.  If both point 1 and point 2 lie
     * on same side of second line segment, the line segments do
     * not intersect.
     */

    if (r1 != 0 && r2 != 0 && SAME_SIGNS(r1, r2))
	    return 0;

    /* Line segments intersect: compute intersection point. 
     */

    denom = a1 * b2 - a2 * b1;

    if (denom == 0)
	    return -1;

    offset = denom < 0 ? - denom / 2 : denom / 2;

    /* The denom/2 is to get rounding instead of truncating.  It
     * is added or subtracted to the numerator, depending upon the
     * sign of the numerator.
     */

    num = b1 * c2 - b2 * c1;
    intersect[0] = ( num < 0 ? num - offset : num + offset ) / denom;

    num = a2 * c1 - a1 * c2;
    intersect[1] = ( num < 0 ? num - offset : num + offset ) / denom;

    return 1;
	
}

#ifdef MAIN_DEFINED

int main(int argc, char **argv) 
{
	point2_t pA1, pA2, pB1, pB2, pRes;
	int res;

	pA1[0] = 1.0;
	pA1[1] = 2.3;
	pA2[0] = 5.7;
	pA2[1] = 5.4;
	
	
	pB1[0] = 4.2;
	pB1[1] = 0.3;
	pB2[0] = 2.4;
	pB2[1] = 6.7;

	res = point_line_intersect(pA1, pA2, pB1, pB2, pRes);

	if (res) {
		printf("Intersection point: (%f,%f)\n", pRes[0], pRes[1]); 
	} else {
		printf("No intersection\n");
	}
	
	return res;
}

#endif
