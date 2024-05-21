// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/init.h>
#include <linux/module.h>

#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/atomic/atomic-instrumented.h>

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

#define ATTR_SHOW(name, expr)                                          \
	static ssize_t name##_show(struct dmp_stats *stats, char *buf) \
	{                                                              \
		return sysfs_emit(buf, "%lld\n", expr);                \
	}

#define STATS_READ(field) atomic64_read(&stats->field)

ATTR_SHOW(read_reqs, STATS_READ(read_reqs))
static struct stats_attr read_reqs_attr = __ATTR_RO(read_reqs);

ATTR_SHOW(write_reqs, STATS_READ(write_reqs))
static struct stats_attr write_reqs_attr = __ATTR_RO(write_reqs);

ATTR_SHOW(read_avg_size, STATS_READ(read_total_size) / STATS_READ(read_reqs))
static struct stats_attr read_avg_size_attr = __ATTR_RO(read_avg_size);

ATTR_SHOW(write_avg_size, STATS_READ(write_total_size) / STATS_READ(write_reqs))
static struct stats_attr write_avg_size_attr = __ATTR_RO(write_avg_size);

ATTR_SHOW(total_reqs, STATS_READ(read_reqs) + STATS_READ(write_reqs))
static struct stats_attr total_reqs_attr = __ATTR_RO(total_reqs);

ATTR_SHOW(total_avg_size,
	  (STATS_READ(read_total_size) + STATS_READ(write_total_size)) /
		  (STATS_READ(read_reqs) + STATS_READ(write_reqs)))
static struct stats_attr total_avg_size_attr = __ATTR_RO(total_avg_size);

static struct attribute *stats_default_attrs[] = {
	&read_reqs_attr.attr,
	&write_reqs_attr.attr,
	&read_avg_size_attr.attr,
	&write_avg_size_attr.attr,
	&total_reqs_attr.attr,
	&total_avg_size_attr.attr,
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
	kobject_put(&stats->kobj);
}
