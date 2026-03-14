#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/mutex.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/delay.h>

#include "bno055.h"


#define DRIVER_NAME   "bno055_dev"
#define DRIVER_AUTHOR "desmtiny nguyenhoangminh@gmail.com"
#define DRIVER_DESC   "BoshBosch BNO055 IMU Driver"
#define DRIVER_VERS   "1.0"

int bno055_probe(struct i2c_client *client)
{
	struct iio_dev *indio_dev;
	struct bno055_state *st;
	int chip_id;
	int ret;
    indio_dev = devm_iio_device_alloc(dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;
	st = iio_priv(indio_dev);
	st->regmap = regmap;
	mutex_init(&st->lock);
	ret = regmap_read(regmap,
			BNO055_REG_CHIP_ID,
			&chip_id);
	if (ret)
		return ret;		

}

void bno055_remove(struct i2c_client *client)
{
	dev_info(dev, "BNO055 removed\n");
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);  
MODULE_VERSION(DRIVER_VERS);