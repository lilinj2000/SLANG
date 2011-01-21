#include <string.h>
#include <time.h>
#include <sys/queue.h>
#include "probed.h"

int res_response = 0;
int res_timeout = 0;
long long res_rtt_total = 0;
ts_t res_rtt_min, res_rtt_max;

void client_res_init(void) {
	LIST_INIT(&res_head);
	res_rtt_min.tv_sec = 99999;
	res_rtt_min.tv_nsec = 0;
	res_rtt_max.tv_sec = 0;
	res_rtt_max.tv_nsec = 0;
}

void client_res_insert(struct in6_addr *addr, data_t *data, ts_t *ts) {
	struct res *r;

	r = malloc(sizeof *r);
	memset(r, 0, sizeof *r);
	(void)clock_gettime(CLOCK_REALTIME, &r->created);
	r->state = STATE_PING;
	memcpy(&r->addr, addr, sizeof r->addr);
	r->id = data->id;
	r->seq = data->seq;
	r->ts[0] = *ts;
	LIST_INSERT_HEAD(&res_head, r, res_list);
	client_res_update(addr, data, ts);
}

void client_res_update(struct in6_addr *addr, data_t *data, ts_t *ts) {
	struct res *r, *r_tmp;
	ts_t now, diff, rtt;
	char old_state;

	(void)clock_gettime(CLOCK_REALTIME, &now);
	r = res_head.lh_first;
	while (r != NULL) {
		/* If match; update */
		old_state = r->state;
		if (r->id == data->id &&
			r->seq == data->seq &&
			memcmp(&r->addr, addr, sizeof addr) == 0) {
			if (data->type == 'o') {
				if (r->state == STATE_PING)
					r->state = STATE_GOT_PONG;
				if (r->state == STATE_GOT_TS)
					r->state = STATE_READY;
				r->ts[3] = *ts;
			} else if (data->type == 't') {
				if (r->state == STATE_PING)
					r->state = STATE_GOT_TS;
				if (r->state == STATE_GOT_PONG)
					r->state = STATE_READY;
				r->ts[1] = data->t2;
				r->ts[2] = data->t3;
			}
		}
		/* Because of Intel RX timestamp bug, wait until next TS to print */
		if (old_state == STATE_READY && data->type == 't') {
			diff_ts(&diff, &r->ts[3], &r->ts[0]);
			diff_ts(&now, &r->ts[2], &r->ts[1]);
			diff_ts(&rtt, &diff, &now);
			if (rtt.tv_sec > 0)
				syslog(LOG_INFO, "Response %d from %d in %10ld.%09ld", 
					r->seq, r->id, rtt.tv_sec, rtt.tv_nsec);
			else 
				syslog(LOG_INFO, "Response %d from %d in %ld ns", 
					r->seq, r->id, rtt.tv_nsec);
			res_response++;
			if (cmp_ts(&res_rtt_max, &rtt) == -1)
			   res_rtt_max = rtt;	
			if (cmp_ts(&res_rtt_min, &rtt) == 1)
			   res_rtt_min = rtt;	
			/* Ready; safe removal */
			r_tmp = r->res_list.le_next;
			LIST_REMOVE(r, res_list);
			free(r);
			r = r_tmp;
			continue;
		}
		diff_ts(&diff, &now, &(r->created)); 
		if (diff.tv_sec > 10) {
			syslog(LOG_INFO, "Timeout %d, %d sec", r->seq, (int)diff.tv_sec);
			res_timeout++;
			/* Timeout; safe removal */
			r_tmp = r->res_list.le_next;
			LIST_REMOVE(r, res_list);
			free(r);
			r = r_tmp;
			continue;
		}
		/* Alright, next entry */
		r = r->res_list.le_next;
	}
}

void client_res_summary(int sig) {
	float loss;

	loss = (float)res_timeout / (float)res_response;
	printf("\n%d responses, %d timeouts, %f%% packet loss\n", 
			res_response, res_timeout, loss);
	if (res_rtt_max.tv_sec > 0)
		printf("max: %ld.%09ld", res_rtt_max.tv_sec, res_rtt_max.tv_nsec);
	else 
		printf("max: %ld", res_rtt_max.tv_nsec);
	if (res_rtt_min.tv_sec > 0)
		printf(", min: %ld.%09ld\n", res_rtt_min.tv_sec, res_rtt_min.tv_nsec);
	else 
		printf(", min: %ld\n", res_rtt_min.tv_nsec);
	exit(0);
}

/*void client_res_print() {
	struct res *r;

	r = res_head.lh_first;
	while (r != NULL) {
		printf("id: %d, seq: %d\n", r->id, r->seq);
client_		r = r->res_list.le_next;
	}
}*/