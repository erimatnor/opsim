#include <openssl/sha.h>
#include <string.h>
#include <math.h>       
#include <stdlib.h>
#include <stdio.h>

#include "bloomfilter.h"

static int bloomfilter_calculate_length(unsigned int num_keys, float error_rate, unsigned  int *lowest_m, unsigned int *best_k);

struct bloomfilter *bloomfilter_new(float error_rate, unsigned int capacity)
{
	struct bloomfilter *bf;
	unsigned int m, k, i;
	
	bloomfilter_calculate_length(capacity, error_rate, &m, &k);

	bf = (struct bloomfilter *)malloc(sizeof(struct bloomfilter) + (m / 8));

	if (!bf)
		return NULL;

	memset(bf, 0, sizeof(struct bloomfilter) + (m / 8));

	bf->m = m;
	bf->k = k;
	
	bf->salts = (unsigned int *)malloc(sizeof(unsigned int) * k);

	if (!bf->salts) {
		free(bf);
		return NULL;
	}
	       
	/* Create salts for hash functions */
	for (i = 0; i < k; i++) {
		bf->salts[i] = random();
	}
	
	return bf;
}

void bloomfilter_print(struct bloomfilter *bf)
{
	int i;

	for (i = 0; i < bf->m / 8; i++) {
		printf("%d", bf->filter[i] & 0x01 ? 1 : 0);
		printf("%d", bf->filter[i] & 0x02 ? 1 : 0);
		printf("%d", bf->filter[i] & 0x04 ? 1 : 0);
		printf("%d", bf->filter[i] & 0x08 ? 1 : 0);
		printf("%d", bf->filter[i] & 0x10 ? 1 : 0);
		printf("%d", bf->filter[i] & 0x20 ? 1 : 0);
		printf("%d", bf->filter[i] & 0x40 ? 1 : 0);
		printf("%d", bf->filter[i] & 0x80 ? 1 : 0);
	}
	printf("\n");
}

int bloomfilter_add_or_check(struct bloomfilter *bf, char *key, 
			     unsigned int len, int check)
{
	unsigned char *buf;
	int i, res = 1;

	if (!bf || !key)
		return -1;

	buf = malloc(len + sizeof(bf->salts[0]));

	if (!buf)
		return -1;

	memcpy(buf, key, len);
	
	for (i = 0; i < bf->k; i++) {
		unsigned int *md;
		unsigned int hash = 0;
		int j, index;
		
		/* Salt the input */
		memcpy(buf + len, bf->salts + i, sizeof(bf->salts[i]));

		md = (unsigned int *)SHA1(buf, len+sizeof(bf->salts[i]), NULL);
		
		for (j = 0; j < 5; j++) {
			hash = hash ^ md[j];			
		}
		
		/* Maybe there is a more efficient way to set the
		   correct bits? */
		index = hash % bf->m;

		//printf("index%d=%u\n", i, index);

		if (check) {
			if (!(bf->filter[index/8] & (1 << (index % 8)))) {
				res = 0;
				goto out;
			} /* else  */
/* 				printf("bit=%d is in set\n", index);  */
		} else			
			bf->filter[index/8] |= (1 << (index % 8));		
	}
out:
	free(buf);

	return res;
}

void bloomfilter_free(struct bloomfilter *bf)
{
	if (!bf)
		return;

	free(bf->salts);
	free(bf);	
}

/* Adapted from the perl code found at this URL:
 * http://www.perl.com/lpt/a/831 and at CPAN */
int bloomfilter_calculate_length(unsigned int num_keys, float error_rate, 
				 unsigned int *lowest_m, unsigned int *best_k)
{
	double lowest_m_tmp = -1;
	double k;
	
	*best_k = 1;

	for (k = 1; k < 101; k++) {
		double m = (-1 * k * num_keys) / log(1 - pow(error_rate, 1/k));
		
		if (lowest_m_tmp < 0 || m < lowest_m_tmp) {
			lowest_m_tmp = m;
			*best_k = k;
		}
	}

	*lowest_m = (int)lowest_m_tmp + 1;

	return 0;
}

#ifdef MAIN_DEFINED

int main(int argc, char **argv)
{
	struct bloomfilter *bf, *bf2;
	char *key = "John McEnroe";
	char *key2= "Johne";
	int res;
	
	bf = bloomfilter_new(0.01, 100);
	bf2 = bloomfilter_new(0.01, 100);
	
	if (!bf || !bf2)
		return -1;

//	printf("bloomfilter m=%u, k=%u\n", bf->m, bf->k);

	bloomfilter_add(bf, key, strlen(key));

	
	printf("bf1:\n");
	bloomfilter_print(bf);

	bloomfilter_add(bf2, key2, strlen(key2));
	
	printf("bf2:\n");
	bloomfilter_print(bf2);
	
//	res = bloomfilter_check(bf, key2, strlen(key2));
	res = bloomfilter_check(bf, key, strlen(key));

	if (res)
		printf("\"%s\" is in filter\n", key2);
	else
		printf("\"%s\" is NOT in filter\n", key2);

	bloomfilter_free(bf);
	bloomfilter_free(bf2);

	return 0;
}

#endif
