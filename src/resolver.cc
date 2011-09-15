/*
	ApacheTop resolver functions

	parse() will call add_request()
		this checks to see if we have an IP -> host mapping already
		if not it fires off adns_submit_reverse 


*/

#if HAVE_ADNS_H

#include "apachetop.h"

Resolver::Resolver(void)
{
	if (adns_init(&adns, adns_if_noenv, 0) != 0)
	{
		perror("adns_init");
		exit(1);
	}
}

Resolver::~Resolver(void)
{
	adns_finish(adns);
}

int Resolver::add_request(char *request, enum resolver_action act)
{
	struct sockaddr_in addr;

	/* depending on resolver_action, this is an IP or a host to lookup
	 * the other way */
	
	/* at the moment we only do IPs */

	switch(act)
	{
		case resolver_gethost:

		/* see if this IP is in host_ip_tablei; if it is we may be
		 * able to get a host for it (or an adns is already looking
		 * it up/has looked it up and got negative reply) */


		addr.sin_family = AF_INET;
		/* add error checking */
#if HAVE_INET_ATON
		inet_aton(request, &(addr.sin_addr));
#else
		addr.sin_addr.s_addr = inet_addr(request);
#endif


//	adns_submit_reverse(adns, (struct sockaddr *)&addr, adns_r_ptr,
//	    (enum adns_queryflags)adns_qf_owner, NULL, b->dns_query);

		break;
	}
}
#endif /* HAVE_ADNS_H */
