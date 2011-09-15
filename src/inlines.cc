#include "apachetop.h"

#define THREE_QUARTERS  24
#define ONE_EIGHTH      4
#define HIGH_BITS       (~((unsigned int)(~0) >> ONE_EIGHTH))

inline unsigned int StringHash(register const char *str)
{
	register unsigned int val;
	register unsigned int i;

	for (val = 0; *str; str++)
	{
		val = (val << ONE_EIGHTH) + *str;

		if ((i = val & HIGH_BITS) != 0)
			val = (val ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
	}
	return val;
}

inline unsigned int QuickHash(register const char *str)
{
	register unsigned int val, tmp;

	for(val = 0 ; *str ; str++)
	{
		val = (val << 4) + *str;
		if ((tmp = (val & 0xf0000000)))
			val = (val ^ (tmp >> 24)) ^ tmp;
	}
	return val;
}

inline unsigned long TTHash(register const char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}
