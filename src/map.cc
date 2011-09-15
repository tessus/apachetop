#include "apachetop.h"

/* string to hash map, and vice versa; for quick string lookups */

extern Circle *c;
extern time_t now;

int map::create(int passed_size)
{
	size = passed_size;

	tab = (struct hash_struct *)malloc(size * sizeof(struct hash_struct));

	/* initialise map to be empty */
	this->empty(0, size);

	/* a hash for quick string lookups */
	tab_hash = new OAHash;
	tab_hash->create(size*9);
	
	return 0;
}

void map::empty(int from, int to)
{
	int x;
	struct hash_struct *ptr;

	for(ptr = &tab[from], x = from; x < to; ++ptr, ++x)
	{
		ptr->refcount = 0;
		ptr->pos = x;
		ptr->string = NULL;
		ptr->time = 0;
	}

	return;
}

int map::destroy(void)
{
	free(tab);

	tab_hash->destroy();
	delete tab_hash;

	return 0;
}

int map::resize(int newsize)
{
	int x;
	struct hash_struct *newtab;

	/* we have to resize the tab, then clear the hash, remake the hash
	 * at the newsize*5, then re-populate the hash. Remaking the hash is
	 * quite expensive, so we'd like to do this as infrequently as
	 * possible */
	newtab = (struct hash_struct *)
	    realloc(tab, newsize * sizeof(struct hash_struct));
	
	/* did it work? */
	if (!newtab)
	{
		perror("realloc map");
		exit(1);
	}

	tab = newtab;

	/* empty the new area */
	this->empty(size, newsize);

	/* remake the hash since memory addresses may have moved */
	delete tab_hash;
	tab_hash = new OAHash;
	tab_hash->create(newsize*9);
	for (x = 0 ; x < size ; ++x)
	{
		tab_hash->insert(tab[x].string, &tab[x]);
	}

//	dprintf("%p resize from %d to %d\n", this, size, newsize);

	size = newsize;

	return 0;
}

int map::insert(char *string)
{
	int x; 
	struct hash_struct *ptr;
	
	/* if this string is in the map, return existing position only */
	if ((x = this->lookup(string)) != -1)
	{
		/* we wanted to insert, but didn't, so the refcount for this
		 * particular entry is incremented */
		tab[x].refcount++;

//		dprintf("%d Found %p %d for %s\n", time(NULL), this, x, string);
		return x;
	}
	
	/* find free table slot FIXME make this more efficient */
	for (ptr = &tab[0], x = 0; x <= size; ++ptr, ++x)
	{
		if (x == size)
		{
			/* none free, make room */
			this->resize(size * 2);

			/* realloc may well move the table in
			 * memory so update the pointer */
			ptr = &tab[x];

			break;
		}

		/* if this map entry has a refcount of zero (this is new in
		 * v0.12) then nothing is pointing at it, hopefully, so it
		 * can be nuked */
		if (ptr->refcount == 0)
			break;
	}

	/* if we are re-using, free up the old data */
	if (ptr->string)
	{
		tab_hash->remove(ptr->string);
		free(ptr->string);
	}

//	dprintf("%d Using %p %d for %s\n", time(NULL), this, x, string);

	/* make entry in our table */
	ptr->time = now;
	ptr->refcount = 1;
	ptr->string = strdup(string);

	/* add into hash */
	tab_hash->insert(ptr->string, ptr);

	return x;
}

int map::remove(char *string)
{
	struct hash_struct *ptr;

	/* find string in hash */
	if ((ptr = (struct hash_struct *)tab_hash->lookup(string)))
	{
//		dprintf("%d Remove %p %d for %s\n", time(NULL), this, ptr->pos, string);

		/* remove from table */
		free(ptr->string);
		ptr->string = NULL;
		ptr->time = 0;
		ptr->refcount = 0;

		/* remove from hash */
		tab_hash->remove(string);
	}

	return 0;
}

int map::lookup(char *string)
{
	struct hash_struct *ptr;

	if ((ptr = (struct hash_struct *)tab_hash->lookup(string)))
	{
		ptr->time = now; /* touch it, so insert won't remove */
		return ptr->pos;
	}

	return -1;
}

char *map::reverse(int pos)
{
	/* return pointer to char for this string pos */
	return tab[pos].string;
}

void map::sub_ref(int pos)
{
//	dprintf("%d subref %p %d for %s\n",
//	    time(NULL), this, pos, tab[pos].string);
	
	if (tab[pos].refcount > 0)
		tab[pos].refcount--;

}
