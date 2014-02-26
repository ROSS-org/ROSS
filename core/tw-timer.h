#ifndef INC_tw_timer_h
#define INC_tw_timer_h

typedef tw_event * tw_timer;

extern tw_timer	 tw_timer_event_new(tw_lp *dst, tw_stime, tw_lp *src);
extern void	 tw_timer_event_send(tw_timer event);
extern tw_timer	 tw_timer_init(tw_lp * lp, tw_stime ts);
extern void	 tw_timer_cancel(tw_lp * lp, tw_timer * e);
extern void	 tw_timer_reset(tw_lp * lp, tw_timer * e, tw_stime ts);
#endif
