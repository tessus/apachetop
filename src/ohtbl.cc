#include "apachetop.h"

#include "inlines.cc"

#define OACMP(a, b) (strcmp((char *)a, (char *)b) == 0) ? 1 : 0
#define OA_H1(k) (TTHash(k) % positions)
#define OA_H2(k) (1 + (StringHash(k) % (positions-2)))
#define OA_HASH(k) (OA_H1(k) + (i * OA_H2(k))) % positions

static int primes[] = {101, 241, 499, 1009, 2003, 3001, 4001, 5003,
                       10007, 15013, 19381, 29131, 38011, 45589,
                       80021, 100003, 150001, 200003, 500009, 5000011, 0};

int OAHash::getNextPrime(int size)
{
	int *prime;
	for (prime = &primes[0] ; *prime ; prime++)
		if (*prime > size)
			return *prime;
	
	return size*2; /* we're out of primes so give up */
}

int OAHash::create(int p_positions)
{
	unsigned int i;

	/* use the next prime up from p_positions */
	if ((p_positions = getNextPrime(p_positions)) == -1)
		abort();
	
	if ((table = (ohEntry *)malloc(p_positions * sizeof(ohEntry))) == NULL)
		return -1;
	
	positions = p_positions;

	for(i = 0 ; i < positions ; ++i)
		table[i].key = NULL;

	size = 0;

	return 0;
}

void OAHash::destroy(void)
{
	free(table);

	return;
}

void *OAHash::insert(char *key, void *data)
{
	unsigned int p, i;
	void *d;
 
	// Do not exceed the number of positions in the table.
	if (size == positions)
		return NULL;

	// Do nothing if the data is already in the table.
	if ((d = lookup(key)))
		/* return the position in case it can be used */
		return d;

	for (i = 0; i < positions; ++i)
	{
		p = OA_HASH(key);
		if (table[p].key == NULL || table[p].key == &vacated)
		{
			// Insert the data into the table.
			table[p].key = key;
			table[p].data = data;
			size++;
			return data;
		}
	}

	return NULL;
}

int OAHash::remove(char *key)
{
	unsigned int p, i;

	for (i = 0; i < positions; ++i)
	{
		p = OA_HASH(key);
		if (table[p].key == NULL)
		{
			// Return that the data was not found.
			return -1;
		}
		else if (table[p].key == &vacated)
		{
			// Search beyond vacated positions.
			continue;
		}
		else if (OACMP(table[p].key, key))
		{
			table[p].key = &vacated;
			size--;
			return 0;
		}
	}
	return -1;
}

void *OAHash::lookup(char *key)
{
	unsigned int p, i;

	for (i = 0; i < positions; ++i)
	{
		p = OA_HASH(key);
		if (table[p].key == NULL)
		{
			return NULL;
		}

		else if (table[p].key == &vacated)
		{
			continue;
		}

		else if (OACMP(table[p].key, key))
			// return pointer to data
			return table[p].data;

	}
	return NULL;
}
