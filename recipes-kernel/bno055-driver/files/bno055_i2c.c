#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mod_devicetable.h>
#include <linux/regmap.h>

#include "bno055.h"

#define DRIVER_NAME   "bno055"

static int bno055_i2c_probe(struct i2c_client *client)
{
	struct regmap *regmap;

	regmap = devm_regmap_init_i2c(client, &bno055dev_regmap_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

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
		.name = DRIVER_NAME,
		.of_match_table = bno055_of_match,
	},
	.probe = bno055_i2c_probe,
	.id_table = bno055_id,
};

module_i2c_driver(bno055_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("desmtiny");
MODULE_DESCRIPTION("Bosch BNO055 IIO driver");