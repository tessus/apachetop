#ifndef _RESOLVER_H_
#define _RESOLVER_H_

enum resolver_action
{
	resolver_getip =   0,
	resolver_gethost = 1
};

class Resolver
{
	public:
	Resolver::Resolver(void);
	Resolver::~Resolver(void);
	int add_request(char *request, enum resolver_action act);



	private:
	adns_state adns;

	struct host_ip_table
	{
		unsigned long ipl;

		char *host;
	};

	OAHash *host_ip_hash;
};

#endif
