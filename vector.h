#ifndef _VECTOR2_H
#define _VECTOR2_H

typedef float vector_t;
typedef vector_t vector2_t[2];
typedef vector_t vector3_t[3];


#define v2_dotpt(x, y) ((x)[0]*(y)[0]+(x)[1]*(y)[1])
#define v2_subtract(a, b, c) ((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1])
#define v2_add(a, b, c) ((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1])
#define v2_copy(a, b) ((b)[0]=(a)[0],(b)[1]=(a)[1])
#define v2_scale(v, s, o) ((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s))
#define v2_MA(v, s, b, o) ((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s))
vector_t v2_normalize(vector2_t v);

vector_t v2_length(const vector2_t v);
void v2_invert(vector2_t v);
vector_t v2_distance(const vector2_t v1, const vector2_t v2);

#endif
