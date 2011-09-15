#ifndef _MAP_H_
#define _MAP_H_

class map
{
public:
	int create(int passed_size);
	void empty(int from, int to);
	int destroy(void);
	int resize(int newsize);
	int insert(char *string);
	int remove(char *string);
	int lookup(char *string);


	char *reverse(int pos);
	void sub_ref(int pos);

private:
	struct hash_struct
	{
		unsigned int refcount;

		int pos;
		char *string;

		time_t time;
	};

	struct hash_struct *tab;
	int size;

	OAHash *tab_hash;
};
#endif
