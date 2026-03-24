#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regmap.h>

#include "bno055.h"

#define DRIVER_NAME   "bno055_dev"
#define DRIVER_AUTHOR "desmtiny nguyenhoangminh@gmail.com"
#define DRIVER_DESC   "BoshBosch BNO055 IIO Driver"
#define DRIVER_VERS   "1.0"


static int bno055_i2c_probe(struct i2c_client *client)
{
	struct regmap *regmap;
	regmap = devm_regmap_init_i2c(client, &bno055_regmap_config);
	return bno055_probe(&client->dev, regmap);
}

static const struct of_device_id bno055_of_match[] = {
	{ .compatible = "bosch,bno055" },
	{ }
};
MODULE_DEVICE_TABLE(of, bno055_of_match);

static const struct i2c_device_id bno055_id[] = {
	{ "bno055", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, bno055_id);

static struct i2c_driver bno055_driver = {
	.driver = {
		.name = "bno055",
		.of_match_table = bno055_of_match,
	},
	.probe = bno055_i2c_probe,
	// .remove = bno055_remove,
	.id_table = bno055_id,
};

module_i2c_driver(bno055_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);  
MODULE_VERSION(DRIVER_VERS);
