#ifndef _OHTBL_H_
#define _OHTBL_H_

class OAHash
{
	public:
	int create(int p_positions);

	void destroy(void);
	void *insert(char *key, void *data);
	int remove(char *key);
	void *lookup(char *key);

	unsigned int size;

	private:
	int getNextPrime(int size);
	int cmp(const void *a, const void *b);

	typedef struct
	{
		char *key;
		void *data;
	} ohEntry;

	unsigned int positions;
	ohEntry *table;
	char vacated;
};

#endif
