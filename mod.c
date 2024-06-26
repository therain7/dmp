// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/init.h>
#include <linux/spinlock.h>

#include <linux/device-mapper.h>
#include <linux/bio.h>

#include "stats.h"

struct dmp_c {
	struct dm_dev *dev; // underlying device
	struct dmp_stats *stats; // per device stats
};

// aggregate stats for all devices
static struct dmp_stats *global_stats;

static struct kset *stats_kset;

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	int ret;

	if (argc != 1) {
		ti->error = "Invalid argument count";
		return -EINVAL;
	}

	struct dmp_c *pc = kzalloc(sizeof(*pc), GFP_KERNEL);
	if (!pc) {
		ti->error = "Cannot allocate proxy context";
		return -ENOMEM;
	}

	ret = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table),
			    &pc->dev);
	if (ret) {
		ti->error = "Device lookup failed";
		goto err_free_pc;
	}

	pc->stats =
		dmp_create_stats(dm_table_device_name(ti->table), stats_kset);
	if (!pc->stats) {
		ret = -ENOMEM;
		goto err_put_device;
	}

	ti->private = pc;
	return 0;

err_put_device:
	dm_put_device(ti, pc->dev);
err_free_pc:
	kfree(pc);
	return ret;
}

static void dmp_dtr(struct dm_target *ti)
{
	struct dmp_c *pc = ti->private;

	dmp_destroy_stats(pc->stats);
	dm_put_device(ti, pc->dev);
	kfree(pc);
}

static void update_stats(struct bio *bio, struct dmp_stats *stats)
{
	u32 size = bio->bi_iter.bi_size;
	switch (bio_op(bio)) {
	case REQ_OP_READ:
		spin_lock(&stats->read_stats_lock);

		stats->read_reqs++;
		stats->read_total_size += size;

		spin_unlock(&stats->read_stats_lock);
		break;
	case REQ_OP_WRITE:
		spin_lock(&stats->write_stats_lock);

		stats->write_reqs++;
		stats->write_total_size += size;

		spin_unlock(&stats->write_stats_lock);
		break;
	default:
		break;
	}
}

static int dmp_map(struct dm_target *ti, struct bio *bio)
{
	struct dmp_c *pc = ti->private;

	bio_set_dev(bio, pc->dev->bdev);
	update_stats(bio, pc->stats);
	update_stats(bio, global_stats);

	return DM_MAPIO_REMAPPED;
}

static struct target_type dmp_target = {
	.name = "dmp",
	.version = { 1, 0, 0 },
	.module = THIS_MODULE,
	.ctr = dmp_ctr,
	.dtr = dmp_dtr,
	.map = dmp_map,
};

static int __init dmp_init(void)
{
	int ret;

	stats_kset =
		kset_create_and_add("stat", NULL, &THIS_MODULE->mkobj.kobj);
	if (!stats_kset) {
		return -ENOMEM;
	}

	global_stats = dmp_create_stats("all", stats_kset);
	if (!global_stats) {
		ret = -ENOMEM;
		goto err_kset_unregister;
	}

	ret = dm_register_target(&dmp_target);
	if (ret) {
		goto err_destroy_stats;
	}

	return 0;

err_destroy_stats:
	dmp_destroy_stats(global_stats);
err_kset_unregister:
	kset_unregister(stats_kset);
	return ret;
}

static void __exit dmp_exit(void)
{
	dm_unregister_target(&dmp_target);
	dmp_destroy_stats(global_stats);
	kset_unregister(stats_kset);
}

module_init(dmp_init);
module_exit(dmp_exit);

MODULE_AUTHOR("Andrei <therain.i@yahoo.com>");
MODULE_DESCRIPTION(DM_NAME " dmp target");
MODULE_LICENSE("GPL");
