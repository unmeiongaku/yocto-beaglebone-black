// SPDX-License-Identifier: GPL-2.0-only
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/mod_devicetable.h>

#include "bmp280.h"


#define DRIVER_NAME "bmp280dev"

static int bmp280_i2c_probe(struct i2c_client *client)
{
    struct regmap *regmap;
	regmap = devm_regmap_init_i2c(client, &bmp280dev_regmap_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);
	return bmp280_probe(&client->dev, regmap);
}

static const struct of_device_id bmp280_of_match[] = {
	{ .compatible = "desmtiny,bmp280" },
	{ }
};

MODULE_DEVICE_TABLE(of, bmp280_of_match);


static const struct i2c_device_id bmp280_id[] = {
    { "bmp280", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bmp280_id);

static struct i2c_driver bnp280_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = bmp280_of_match,
	},
	.probe = bmp280_i2c_probe,
	.id_table = bmp280_id,
};

module_i2c_driver(bnp280_driver);

MODULE_AUTHOR("desmtiny <nguyenhoangminhdo@gmail.com>");
MODULE_DESCRIPTION("Driver for Bosch Sensortec BMP180/BMP280 pressure and temperature sensor");
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(IIO_BMP280);