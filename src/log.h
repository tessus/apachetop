#ifndef _LOG_H_
#define _LOG_H_

/* types */
#define LOG_COMMON 1
#define LOG_COMBINED 2
#define LOG_ATOP 3

/* parse*() fills in this struct */
struct logbits {
	int url_pos;              /* location of url in urlmap */
	int url_hash;             /* hash of url string */

	/* host and ip. Whatever we have in the log gets set here initially,
	** then want_foo is set to false. want_otherfoo is set to true 
	** explicitly; then an entry is made into the relevant Queue
	** (want_host or want_ip). adns does the resolving, and then
	** whatever it finds is placed in otherfoo, and want_otherfoo is set
	** to false */
	bool want_host;
	int host_pos;             /* location of host in hostmap */
	int host_hash;            /* hash of host string */

	bool want_ip;
	//unsigned int ipl;         /* numerical representation of IP */
	int ip_pos;               /* location of IP in ipmap */
	int ip_hash;              /* hash of IP string */

#if HAVE_ADNS_H
	adns_query *dns_query;
	//Resolver dns_query;
#endif

	int ref_pos;              /* location of referrer in map */
	int ref_hash;             /* hash of Referrer string */

	int fileid;               /* which file descriptor we're from */
	int file_pos;             /* location of file in filemap */

	unsigned short retcode;   /* return code */
	unsigned int bytes;       /* body of result page */
	time_t time;              /* time of request, unixtime */
};

class LogParser
{
	public:
	virtual int parse(char *logline, struct logbits *b) = 0;

	char *processURL(char **buf);
	int mungeURL(char **url, int *length);
};

class CommonLogParser : public LogParser
{
	int parse(char *logline, struct logbits *b);
};
class AtopLogParser : public LogParser
{
	int parse(char *logline, struct logbits *b);
};

void collect_dns_responses();

#endif
