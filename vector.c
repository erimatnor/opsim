#include <math.h>

#include "vector.h"

vector_t v2_length(const vector2_t v)
{
	return (vector_t)sqrt(v[0]*v[0] + v[1]*v[1]);
}

void v2_invert(vector2_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
}

vector_t v2_distance(const vector2_t v1, const vector2_t v2)
{
	vector2_t v;

	v2_subtract(v2, v1, v);

	return v2_length(v);
}

vector_t v2_normalize(vector2_t v) 
{
        float   length, ilength;

        length = v[0]*v[0] + v[1]*v[1];
        length = sqrt (length);

        if ( length ) {
                ilength = 1/length;
                v[0] *= ilength;
                v[1] *= ilength;
        }
                
        return length;
}

/* vector2_t v2_add(vector2_t v1, vector2_t v2) */
/* { */
/* 	return v1; */
/* } */
