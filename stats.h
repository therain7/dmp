// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DMP_STATS_H
#define DMP_STATS_H

#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/spinlock_types.h>

struct dmp_stats {
	struct kobject kobj;

	spinlock_t read_stats_lock;
	u64 read_reqs; // amount of read requests submitted
	u64 read_total_size; // total size of all read requests submitted

	spinlock_t write_stats_lock;
	u64 write_reqs; // amount of write requests submitted
	u64 write_total_size; // total size of all write requests submitted
};

struct dmp_stats *dmp_create_stats(const char *name, struct kset *kset);
void dmp_destroy_stats(struct dmp_stats *stats);

#endif // !DMP_STATS_H
