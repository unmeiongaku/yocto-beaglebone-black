#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/mutex.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/delay.h>

#include "bno055.h"
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>


#define DRIVER_NAME   "bmp280dev"
#define DRIVER_AUTHOR "desmtiny nguyenhoangminh@gmail.com"
#define DRIVER_DESC   "BoshBosch BMP280 Driver"
#define DRIVER_VERS   "1.0"


static bool bmp280_is_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case BMP280_REG_CONFIG:
	case BMP280_REG_CTRL_MEAS:
	case BMP280_REG_RESET:
		return true;
	default:
		return false;
	}
}

static bool bmp280_is_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case BMP280_REG_TEMP_XLSB:
	case BMP280_REG_TEMP_LSB:
	case BMP280_REG_TEMP_MSB:
	case BMP280_REG_PRESS_XLSB:
	case BMP280_REG_PRESS_LSB:
	case BMP280_REG_PRESS_MSB:
	case BMP280_REG_STATUS:
		return true;
	default:
		return false;
	}
}

static bool bmp280_regmap_readable(struct device *dev, unsigned int reg){
    if(reg == 0xF6) return false;
    if(reg > =  BMP280_REG_STATUS && reg <= BMP280_REG_TEMP_XLSB) return true;
    if(reg == BMP280_ID) return true;
    if(reg >= BMP280_REG_CALIBRATION_START && reg <= BMP280_REG_CALIBRATION_END) return true;
}

const struct regmap_config bmp280dev_regmap_config(){
    {
        .name = DRIVER_NAME,
        .reg_bits = 8,
        .val_bits = 8,
        .max_register = BMP280_TEMP_XLSB,
        .writeable_reg = bmp280_is_writeable_reg,
        .volatile_reg = bmp280_is_volatile_reg,
        .readable_reg = bmp280_regmap_readable,
        .cache_type = REGCACHE_RBTREE,
    }
};


enum bmp280_scan {
	BMP280_PRESS,
	BMP280_TEMP,
	BME280_HUMID,
};

static const struct iio_chan_spec bmp280_channels[] = {
    {
        .type = IIO_PRESSURE,
        .info_mask_separate =   BIT(IIO_CHAN_INFO_PROCESSED) |
				        BIT(IIO_CHAN_INFO_RAW) |
				        BIT(IIO_CHAN_INFO_SCALE) |
				        BIT(IIO_CHAN_INFO_OVERSAMPLING_RATIO),
		.scan_index = BMP280_PRESS,
		.scan_type = {
			.sign = 'u',
			.realbits = 32,
			.storagebits = 32,
			.endianness = IIO_CPU,
	    },
    },
    {
        .type = IIO_TEMP,
        .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED) |
				        BIT(IIO_CHAN_INFO_RAW) |
				        BIT(IIO_CHAN_INFO_SCALE) |
				        BIT(IIO_CHAN_INFO_OVERSAMPLING_RATIO),
        .scan_index = BMP280_TEMP,
        .scan_type = {
			.sign = 's',
			.realbits = 32,
			.storagebits = 32,
			.endianness = IIO_CPU,
		},
    },
    IIO_CHAN_SOFT_TIMESTAMP(2),
}

int bno055_probe(struct device *dev, struct regmap *regmap)
{

    dev_info(dev, "BMP280 ready\n");
    return 0;
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);  
MODULE_VERSION(DRIVER_VERS);