/* This struct and the macros handle time wrapping. */

struct delay {
	unsigned long start;
	unsigned long duration;
};

static inline void set_delay(unsigned long now, struct delay *delay,
				unsigned long duration)
{
	delay->start = now;
	delay->duration = duration;
}
#define DELAY_SET(d, duration) set_delay(pdx->tics, d, duration)
#define DELAY_CLR(d) ((d)->duration = 0)
#define DELAY_ISSET(d) ((d)->duration)

static inline int delay_up(unsigned long now, struct delay *delay)
{
	if (delay->duration)
		return now - delay->start > delay->duration;
	else
		return 0;
}
#define DELAY_UP(d) delay_up(pdx->tics, d)
