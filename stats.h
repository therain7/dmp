#ifndef DMP_STATS_H
#define DMP_STATS_H

#include <linux/kobject.h>
#include <linux/types.h>

struct dmp_stats {
	struct kobject kobj;

	atomic64_t read_reqs;
	atomic64_t read_total_size;

	atomic64_t write_reqs;
	atomic64_t write_total_size;
};

struct dmp_stats *dmp_create_stats(const char *name, struct kset *kset);
void dmp_destroy_stats(struct dmp_stats *stats);

#endif // !DMP_STATS_H
