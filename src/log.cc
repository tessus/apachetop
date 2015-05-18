#include "apachetop.h"

#include "inlines.cc"

#define RESOLVING_STRING "..."
#define NO_RESOLVED_INFO "?"

extern map *um, /* urlmap */
           *im, /* ipmap */
           *hm, /* hostmap */
           *rm, /* referrermap */
           *fm; /* filemap */

extern time_t now;
extern config cf;

extern Circle *c;

extern Queue want_host, want_ip;

#if HAVE_ADNS_H
extern adns_state adns;
#endif

/* CommonLogParser handles common and combined, despite its name */
int CommonLogParser::parse(char *logline, struct logbits *b)
{
	char *bufsp, *bufcp, *ptr;
	char *workptr;

	struct sockaddr_in addr;

	bufsp = logline;

	/* host first */
	bufcp = strchr(logline, ' ');
	if (!bufcp)
		return -1;
	
	*bufcp = (char) NULL;
	++bufcp;

	/* quickly figure out if this is an IP or a host. We do this by
	 * checking each character of it; if every character is either a
	 * digit or a dot, then it's an IP (no host can just be digits)
	*/
	for(workptr = bufsp ; *workptr ; workptr++)
	{
		if (isdigit(*workptr)) continue;
		if (*workptr == '.') continue;

		/* it's neither a digit or a dot */
		break;
	}

	ptr = bufsp;
	if (*workptr)
	{
		/* it is a hostname */

		/* insert will return existing position if it exists */
		b->host_pos = hm->insert(ptr);
		b->host_hash = TTHash(ptr);
		b->want_host = false; /* cos we have it */

#if HAVE_ADNS_H
		if (cf.do_resolving)
		{
			b->want_ip = true;
			
			dprintf("lookup %s\n", ptr);
			/* fire off a query with adns */
			b->dns_query = new adns_query;
			adns_submit(adns, ptr, adns_r_a,
			    (adns_queryflags) NULL, NULL, b->dns_query);

			b->ip_pos = im->insert(RESOLVING_STRING);
			b->ip_hash = TTHash(RESOLVING_STRING);
		}
		else
#endif /* HAVE_ADNS_H */
		{
			/* don't resolve the IP, and use -1 which means
			 * "there is nothing of interest here" */
			b->ip_pos = -1;
			b->want_ip = false;
		}
	}
	else
	{
		/* it is an IP */

		b->ip_pos = im->insert(ptr);
		b->ip_hash = TTHash(ptr);
		b->want_ip = false; /* we have the IP already */

#if HAVE_ADNS_H
		if (cf.do_resolving)
		{

			/* this is so we'll get a display like
			   ..resolving.. [212.13.201.101]
			   then once resolved:
			   clueful.shagged.org [212.13.201.101]
			*/
			b->host_pos = hm->insert(RESOLVING_STRING);
			b->host_hash = TTHash(RESOLVING_STRING);

			b->want_host = true; /* we're going to get this */

			/* construct network byte order num
			** for adns_submit_reverse
			*/
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(ptr);

			b->dns_query = new adns_query;
			adns_submit_reverse(adns, (struct sockaddr *)&addr,
			    adns_r_ptr, (adns_queryflags)adns_qf_owner,
			    NULL, b->dns_query);
		}
		else
#endif /* HAVE_ADNS_H */
		{
			/* don't resolve the host, use the IP */
			b->host_pos = hm->insert(ptr);
			b->host_hash = TTHash(ptr);
			b->want_host = false; /* we are not resolving */
		}
	}

	/* now skip to date */
	if (!(bufcp = strchr(bufcp, '[')))
		return -1;

	bufcp++;

	b->time = now; /* be lazy */

	/* find the end of the date */
	if (!(bufcp = strchr(bufcp, ']')))
		return -1;

	bufcp += 3; /* from end of date to first char of method */

	/* URL. processURL() will update bufcp to point at the end so we can
	 * continue processing from there */
	if ((ptr = this->processURL(&bufcp)) == NULL)
		return -1;

	/* get url_pos for this url; for circle_struct (c) later */
	b->url_pos = um->insert(ptr);
	b->url_hash = TTHash(ptr);

	/* return code */
	b->retcode = atoi(bufcp);
	bufcp += 4;

	/* bytecount */
	b->bytes = atoi(bufcp);


	/* this may be the end of the line if it's a common log; if
	 * it's combined then we have referrer and user agent left */
	if (!(bufsp = strchr(bufcp, '"')))
	{
		/* nothing left, its common */
		
		/* fill in a dummy value for referrer map */
		b->ref_pos = rm->insert("Unknown");
		return 0;
	}

	bufsp += 1; /* skip to first character of referrer */

	/* find the end of referrer and null it */
	if (!(bufcp = strchr(bufsp, '"')))
		return -1;
	*bufcp = (char) NULL;

	/* unless they want to keep it, skip over the protocol, ie http:// */
	if ((cf.preserve_ref_protocol == 0) && (bufcp = strstr(bufsp, "://")))
		bufsp = bufcp + 3;
	

	/* we could munge the referrer now; cut down the path elements,
	 * remove querystring, but we'll leave that for a later date */

//	b->referrer = bufsp;

	/* get ref_pos for this url; for circle_struct (c) later */
	b->ref_pos = rm->insert(bufsp);
	b->ref_hash = TTHash(bufsp);

	/* user-agent is as yet unused */

	return 0;
}


int AtopLogParser::parse(char *logline, struct logbits *b)
{
	return 0;
}


/* generic parser helper functions */

char *LogParser::processURL(char **buf) /* {{{ */
{
	char *bufcp, *realstart, *endptr;
	int length;

	bufcp = *buf;

	/* this skips past the method */
	if (!(bufcp = strchr(bufcp, ' ')) )
		return NULL;
	++bufcp; // skip space

	realstart = bufcp;

	/* find the end of url; locate a protocol, out of the following list */
	if (
	    !(endptr = strstr(bufcp, " HTTP/"))
#if WITH_REAL_PROTOCOLS
	    /* v0.12: RealServer logs are very similar to Apache's,
	     * so we can support those too! Cool! */
	    && !(endptr = strstr(bufcp, " RTSP/")) /* RealStreaming UDP */
	    && !(endptr = strstr(bufcp, " RTSPT/")) /* RealStreaming TCP */
	    && !(endptr = strstr(bufcp, " RTSPH/")) /* RealStreaming HTTP */
#endif
	   )
		return NULL;

	/* null the space in front of it */
	*endptr = (char) NULL;

	/* TODO maybe we can use the protocol someday.. */


	/* this is all mungeURL is interested in */
	length = endptr - realstart;

	/* now find the finishing ", so parse* can deal with rest of line */
	if (!(endptr = strstr(endptr+1, "\" ")))
		return NULL;

	mungeURL(&realstart, &length);
	
	/* feed back where the end of the URL is */
	*buf = endptr+2;

	return realstart;
} /* }}} */

/* munge the url passed in *url inplace;
 * *length is the original length, and we update it once we're done */
int LogParser::mungeURL(char **url, int *length) /* {{{ */
{
	int skipped = 0;
	char *bufcp, *endptr, *workptr;

	endptr = *url + *length;
	*endptr = (char) NULL;

	/* do we want to keep the query string? */
	if (!cf.keep_querystring)
	{
		/* null the first ? or & - anything after
		 * it is unrequired; it's the querystring */
		if ((workptr = strchr(*url, '?')) ||
		    (workptr = strchr(*url, '&')) )
		{
			/* we might have overrun the end of the real URL and
			 * gone into referrer or something. Check that. */
			if (workptr < endptr)
			{
				/* we're ok */
				*workptr = (char) NULL;
				bufcp = workptr+1;
			}
		}
	}

	/* how many path segments of the url are we keeping? */
	if (cf.keep_segments > 0)
	{
		/* given a path of /foo/bar/moo/ and a keep_segments of 2,
		 * we want the / after the second element */

		bufcp = workptr = *url + 1; /* skip leading / */

		//dprintf("workptr is %s\n", workptr);

		/* now skip the next keep_segments slashes */
		while (skipped < cf.keep_segments && workptr < endptr)
		{
			workptr++;

			if (*workptr == '/')
			{
				/* discovered a slash */
				skipped++;

				/* bufcp becomes the char after / */
				bufcp = workptr+1;
			}

			/* if we hit the end before finding the right number
			 * of slashes, we just keep it all */
			if (workptr == endptr)
				bufcp = workptr;
		}
		*bufcp = (char) NULL;
	}


	/* do we want to lowercase it all? */
	if (cf.lowercase_urls)
	{
		workptr = *url;
		while(workptr < endptr)
		{
			*workptr = tolower(*workptr);
			workptr++;
		}
	}

	/* fin */

	return 0;
} /* }}} */

#if HAVE_ADNS_H
/* adns; check to see if any queries have returned, and populate the circle
 * as required. Be careful of any circle entries that have expired since
 * the query was started. */
void collect_dns_responses()
{
	int err;
	struct logbits *lb;
	adns_answer *answer;
	int got_host = false, got_ip = false;

	/* check every circle entry that has want_host or want_ip */

	while(c->walk(&lb) != -1)
	{
		if (lb->want_host == false && lb->want_ip == false)
			continue;

//		dprintf("adns_check for %p\n", lb);
		/* this circle slot has an outstanding query */
		err = adns_check(adns, lb->dns_query, &answer, NULL);

		if (err == EAGAIN)
		{
			/* still waiting */
			continue;
		}

		/* some form of reply. Be it success or error, this query is
		 * now done. */

		got_host = lb->want_host;
		got_ip = lb->want_ip;

		lb->want_host = false;
		lb->want_ip = false;
		delete lb->dns_query;

		if (answer->status == adns_s_ok)
		{
			/* we have a reply */
	//		dprintf("got a reply\n");
			if (got_host)
			{
				/* we'll have this new host in the hostmap ta */
				lb->host_pos = hm->insert(*answer->rrs.str);
				lb->host_hash = TTHash(*answer->rrs.str);
			}
			else if (got_ip)
			{
				/* put the IP into the ipmap */
				lb->ip_pos =
				    im->insert(inet_ntoa(*answer->rrs.inaddr));
				lb->ip_hash =
				    TTHash(inet_ntoa(*answer->rrs.inaddr));
			}

			free(answer);
			continue;
		}

		/* assume this IP has no reverse info; so we'll put the IP
		 * into Host as well; this is so that the Host list will be
		 * maintained properly (if we just put ? into Host, then
		 * they bunch up together)
		*/

		lb->host_pos = hm->insert(im->reverse(lb->ip_pos));
		lb->host_hash = TTHash(im->reverse(lb->ip_pos));
		free(answer);
		continue;
	}
}
#endif /* HAVE_ADNS_H */
