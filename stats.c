// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "stats.h"

#define to_stats(x) container_of(x, struct dmp_stats, kobj)

struct stats_attr {
	struct attribute attr;
	ssize_t (*show)(struct dmp_stats *stats, char *buf);
};
#define to_stats_attr(x) container_of(x, struct stats_attr, attr)

static ssize_t stats_attr_show(struct kobject *kobj, struct attribute *attr,
			       char *buf)
{
	struct dmp_stats *stats = to_stats(kobj);
	struct stats_attr *attribute = to_stats_attr(attr);

	if (!attribute->show) {
		return -EIO;
	}
	return attribute->show(stats, buf);
}

// store is not supported on stats attributes
static ssize_t stats_attr_store(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t len)
{
	return -EIO;
}

static const struct sysfs_ops stats_sysfs_ops = {
	.show = stats_attr_show,
	.store = stats_attr_store,
};

// attr show functions that require no locking
#define ATTR_SHOW_NO_LOCK(name, expr)                                  \
	static ssize_t name##_show(struct dmp_stats *stats, char *buf) \
	{                                                              \
		return sysfs_emit(buf, "%llu\n", expr);                \
	}

ATTR_SHOW_NO_LOCK(read_reqs, stats->read_reqs);
static struct stats_attr read_reqs_attr = __ATTR_RO(read_reqs);

ATTR_SHOW_NO_LOCK(write_reqs, stats->write_reqs);
static struct stats_attr write_reqs_attr = __ATTR_RO(write_reqs);

ATTR_SHOW_NO_LOCK(total_reqs, stats->read_reqs + stats->write_reqs);
static struct stats_attr total_reqs_attr = __ATTR_RO(total_reqs);

static u64 safe_div(u64 x, u64 y)
{
	return y == 0 ? 0 : x / y;
}

// functions below require locking to return consistent state

static ssize_t read_avg_size_show(struct dmp_stats *stats, char *buf)
{
	spin_lock(&stats->read_stats_lock);
	u64 avg = safe_div(stats->read_total_size, stats->read_reqs);
	spin_unlock(&stats->read_stats_lock);

	return sysfs_emit(buf, "%llu\n", avg);
}
static struct stats_attr read_avg_attr = __ATTR_RO(read_avg_size);

static ssize_t write_avg_size_show(struct dmp_stats *stats, char *buf)
{
	spin_lock(&stats->write_stats_lock);
	u64 avg = safe_div(stats->write_total_size, stats->write_reqs);
	spin_unlock(&stats->write_stats_lock);

	return sysfs_emit(buf, "%llu\n", avg);
}
static struct stats_attr write_avg_attr = __ATTR_RO(write_avg_size);

static ssize_t total_avg_size_show(struct dmp_stats *stats, char *buf)
{
	spin_lock(&stats->read_stats_lock);
	spin_lock(&stats->write_stats_lock);

	u64 avg = safe_div(stats->read_total_size + stats->write_total_size,
			   stats->read_reqs + stats->write_reqs);

	spin_unlock(&stats->write_stats_lock);
	spin_unlock(&stats->read_stats_lock);

	return sysfs_emit(buf, "%llu\n", avg);
}
static struct stats_attr total_avg_attr = __ATTR_RO(total_avg_size);

static struct attribute *stats_default_attrs[] = {
	&read_reqs_attr.attr,
	&write_reqs_attr.attr,
	&total_reqs_attr.attr,
	&read_avg_attr.attr,
	&write_avg_attr.attr,
	&total_avg_attr.attr,
	NULL,
};
ATTRIBUTE_GROUPS(stats_default);

static void stats_release(struct kobject *kobj)
{
	struct dmp_stats *stats = to_stats(kobj);
	kfree(stats);
}

static const struct kobj_type stats_ktype = {
	.sysfs_ops = &stats_sysfs_ops,
	.default_groups = stats_default_groups,
	.release = stats_release,
};

struct dmp_stats *dmp_create_stats(const char *name, struct kset *kset)
{
	int ret;

	struct dmp_stats *stats = kzalloc(sizeof(*stats), GFP_KERNEL);
	if (!stats)
		return NULL;

	spin_lock_init(&stats->read_stats_lock);
	spin_lock_init(&stats->write_stats_lock);

	stats->kobj.kset = kset;

	ret = kobject_init_and_add(&stats->kobj, &stats_ktype, NULL, "%s",
				   name);
	if (ret)
		goto err_kobj_put;

	ret = kobject_uevent(&stats->kobj, KOBJ_ADD);
	if (ret)
		goto err_kobj_put;

	return stats;

err_kobj_put:
	kobject_put(&stats->kobj);
	return NULL;
}

void dmp_destroy_stats(struct dmp_stats *stats)
{
	if (!stats)
		return;

	kobject_put(&stats->kobj);
}
