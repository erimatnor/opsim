#ifndef _BLOOMFILTER_H
#define _BLOOMFILTER_H


struct bloomfilter {
	unsigned int k; /* Number of hash functions */
	unsigned int m; /* Length of filter in bits */
	unsigned int *salts; /* k salts used to salt the sha1 hash
				function */
	char filter[0];
};

struct bloomfilter *bloomfilter_new(float error_rate, unsigned int capacity);
int bloomfilter_add_or_check(struct bloomfilter *bf, char *key, unsigned int len, int check);
void bloomfilter_free(struct bloomfilter *bf);

static inline int bloomfilter_check(struct bloomfilter *bf, char *key, unsigned int len)
{
	return bloomfilter_add_or_check(bf, key, len, 1);
}
static inline int bloomfilter_add(struct bloomfilter *bf, char *key, unsigned int len)
{
	return bloomfilter_add_or_check(bf, key, len, 0);
}
#endif /*_BLOOMFILTER_H */
