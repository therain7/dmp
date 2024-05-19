// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/device-mapper.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>

struct dmp_c {
	struct dm_dev *dev; // underlying device
};

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	if (argc != 1) {
		ti->error = "Invalid argument count";
		return -EINVAL;
	}

	struct dmp_c *pc = kmalloc(sizeof(*pc), GFP_KERNEL);
	if (pc == NULL) {
		ti->error = "Cannot allocate proxy context";
		return -ENOMEM;
	}

	int ret = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table),
				&pc->dev);
	if (ret) {
		ti->error = "Device lookup failed";
		kfree(pc);
		return ret;
	}

	ti->private = pc;

	return 0;
}

static void dmp_dtr(struct dm_target *ti)
{
	struct dmp_c *pc = ti->private;

	dm_put_device(ti, pc->dev);
	kfree(pc);
}

static int dmp_map(struct dm_target *ti, struct bio *bio)
{
	struct dmp_c *pc = ti->private;

	bio_set_dev(bio, pc->dev->bdev);
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
module_dm(dmp);

MODULE_AUTHOR("Andrei <therain.i@yahoo.com>");
MODULE_DESCRIPTION(DM_NAME " dmp target");
MODULE_LICENSE("GPL");
