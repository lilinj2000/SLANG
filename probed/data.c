/*
 * RECEIVE PACKET
 * Author: Anders Berggren
 *
 * Function recv_
 */

#include "probed.h"

struct packet p;

void data_recv(int flags) {
	char tmp[INET6_ADDRSTRLEN];
	struct msghdr msg; /* message */
	struct iovec entry; /* misc data, such as timestamp */
	struct sockaddr_in6 addr; /* message remote addr */
	struct {
		struct cmsghdr cm;
		char control[512];
	} control;

	/* prepare message structure */
	memset(&p.data, 0, sizeof(p.data));
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &entry;
	msg.msg_iovlen = 1;
	entry.iov_base = p.data;
	entry.iov_len = sizeof(p.data);
	msg.msg_name = (caddr_t)&addr;
	msg.msg_namelen = sizeof(addr);
	msg.msg_control = &control;
	msg.msg_controllen = sizeof(control);

	if (recvmsg(s, &msg, flags) < 0) {
		if (flags & MSG_ERRQUEUE) 
			syslog(LOG_INFO, "recvmsg: %s (ts)", strerror(errno));
		
		return;
	} else {
		tstamp_get(&msg); /* store kernel/hw rx/tx tstamp */
		if (flags & MSG_ERRQUEUE) {
			return;
		} else {
			them = addr; /* save latest peer spoken to */
			inet_ntop(AF_INET6, &(them.sin6_addr), tmp, 
					INET6_ADDRSTRLEN);
			
			syslog(LOG_DEBUG, "* RX from: %s\n", tmp); 
			syslog(LOG_DEBUG, "* RX data: %zu bytes\n", 
						sizeof(p.data));
			syslog(LOG_DEBUG, "* RX time: %010ld.%09ld\n", 
						p.ts.tv_sec, p.ts.tv_nsec);
		}
	}
}

void data_send(char *d, int size) {
	char tmp[INET6_ADDRSTRLEN];
	fd_set fs; /* select fd set for hw tstamps */
	struct timeval tv; /* select timeout for hw tstamps */

	inet_ntop(AF_INET6, &(them.sin6_addr), tmp, INET6_ADDRSTRLEN);
	syslog(LOG_DEBUG, "* TX to: %s\n", tmp);
	if (c.ts == 's') clock_gettime(CLOCK_REALTIME, &p.ts); /* get sw tx */
	if (sendto(s, d, size, 0, (struct sockaddr*)&them, slen) < 0)
		syslog(LOG_INFO, "sendto: %s", strerror(errno));
	if (c.ts != 's') { /* get kernel/hw tx */
		tv.tv_sec = 1; /* wait for nic tx tstamp during 1sec */ 
		tv.tv_usec = 0;
		FD_ZERO(&fs);
		FD_SET(s, &fs);
		if (select(s + 1, &fs, 0, 0, &tv) > 0) {
			if (FD_ISSET(s, &fs))
				data_recv(MSG_ERRQUEUE); /* get tx kernel ts */
		} else 
			syslog(LOG_ERR, "Kernel TX timestamp error.");
	}
	syslog(LOG_DEBUG, "* TX time: %010ld.%09ld\n", 
			p.ts.tv_sec, p.ts.tv_nsec);
}
