#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/mutex.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/delay.h>
#include "bmp280.h"

#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/pm_runtime.h>

#define DRIVER_NAME   "bmp280dev"
#define DRIVER_AUTHOR "desmtiny nguyenhoangminh@gmail.com"
#define DRIVER_DESC   "BoshBosch BMP280 Driver"
#define DRIVER_VERS   "1.0"

static bool bmp280_is_writeable_reg(struct device *dev, unsigned int reg);
static bool bmp280_is_volatile_reg(struct device *dev, unsigned int reg);
static bool bmp280_regmap_readable(struct device *dev, unsigned int reg);

static int bmp280_chip_id(struct bmp280_priv *priv);
static int bmp280_system_reset(struct bmp280_priv *priv);
static int bmp280_chip_config(struct bmp280_priv *priv);
static int bmp280_read_calib(struct bmp280_priv *priv);
static int bmp280_init_param(struct bmp280_priv *priv);

static void bmp280_regulators_disable(void *data);



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
	return true;
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
	return true;
}

static bool bmp280_regmap_readable(struct device *dev, unsigned int reg){
    if(reg == 0xF6) return false;
    if(reg >=  BMP280_REG_STATUS && reg <= BMP280_REG_TEMP_XLSB) return true;
    if(reg == BMP280_REG_ID) return true;
    if(reg >= BMP280_REG_CALIBRATION_START && reg <= BMP280_REG_CALIBRATION_END) return true;
	return true;
}

const struct regmap_config bmp280dev_regmap_config = {
        .name = DRIVER_NAME,
        .reg_bits = 8,
        .val_bits = 8,
        .max_register = BMP280_REG_TEMP_XLSB,
        .writeable_reg = bmp280_is_writeable_reg,
        .volatile_reg = bmp280_is_volatile_reg,
        .readable_reg = bmp280_regmap_readable,
        .cache_type = REGCACHE_RBTREE,
};

EXPORT_SYMBOL_NS_GPL(bmp280dev_regmap_config, IIO_BMP280);

enum bmp280_scan {
	BMP280_PRESS,
	BMP280_TEMP,
	BME280_HUMID,
};

static const struct iio_chan_spec bmp280dev_channels[] = {
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
};

#define BMP280_NUM_CHANNELS ARRAY_SIZE(bmp280dev_channels)

static int bmp280_read_raw_impl(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	struct bmp280_priv *priv = iio_priv(indio_dev);
	int chan_value;
	int ret;
	guard(mutex)(&priv->lock);
	switch (mask) {
		case IIO_CHAN_INFO_PROCESSED:
			switch (chan->type) {
				case IIO_PRESSURE:
				case IIO_TEMP:
				default:
					return -EINVAL;
			}
		case IIO_CHAN_INFO_RAW:
			switch (chan->type) {
				case IIO_PRESSURE:
					return IIO_VAL_INT;
				case IIO_TEMP:
					return IIO_VAL_INT;
				default:
					return -EINVAL;
			}
		case IIO_CHAN_INFO_SCALE:
		case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
		case IIO_CHAN_INFO_SAMP_FREQ:
		case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
		default:
			return -EINVAL;
	}
	return ret;
}


static int bmp280_read_raw(struct iio_dev *indio_dev,
							struct iio_chan_spec const *chan,
							int *val,
							int *val2,
							long mask)
{
	struct bmp280_priv *priv = iio_priv(indio_dev);
	int ret;
	pm_runtime_get_sync(priv->dev);
	ret = bmp280_read_raw_impl(indio_dev, chan, val, val2, mask);
	pm_runtime_mark_last_busy(priv->dev);
	pm_runtime_put_autosuspend(priv->dev);
	return ret;
}

static int bmp280_read_avail(struct iio_dev *indio_dev,
			  struct iio_chan_spec const *chan,
			  const int **vals,
			  int *type,
			  int *length,
			  long mask)
{
	struct bmp280_priv *priv = iio_priv(indio_dev);
	int ret;
	switch (mask) {
		case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
			switch (chan->type) {
				case IIO_PRESSURE:
					break;
				case IIO_TEMP:
					break;
				default:
					return -EINVAL;
			}
			return IIO_AVAIL_LIST;
		case IIO_CHAN_INFO_SAMP_FREQ:
			return IIO_AVAIL_LIST;
		case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
			return IIO_AVAIL_LIST;
		default:
			return -EINVAL;
	}
	return ret;
}

static int bmp280_write_raw_impl(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan,
				 int val, int val2, long mask)
{
	struct bmp280_priv *priv = iio_priv(indio_dev);
	int ret;
	guard(mutex)(&priv->lock);
	switch (mask) {
		case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
			switch (chan->type) {
				case IIO_PRESSURE:
				case IIO_TEMP:
				default:
					return -EINVAL;
			}
		case IIO_CHAN_INFO_SAMP_FREQ:
		case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
		default:
			return -EINVAL;
	}
}

static int bmp280_write_raw(struct iio_dev *indio_dev,
			 struct iio_chan_spec const *chan,
			 int val,
			 int val2,
			 long mask)
{
	struct bmp280_priv *priv = iio_priv(indio_dev);
	int ret;

	pm_runtime_get_sync(priv->dev);
	ret = bmp280_write_raw_impl(indio_dev, chan, val, val2, mask);
	pm_runtime_mark_last_busy(priv->dev);
	pm_runtime_put_autosuspend(priv->dev);

	return ret;
}

static const struct iio_info bmp280dev_info = {
	.read_raw = bmp280_read_raw,
	.read_avail = bmp280_read_avail,
	.write_raw = bmp280_write_raw,
};

static int bmp280_chip_id(struct bmp280_priv *priv){
	int ret;
	int val;
	ret = regmap_read(priv->regmap, BMP280_REG_ID, &val);
	if(ret) return ret;
	if(val == BMP280_CHIP_ID){
		priv->chipid = BMP280_CHIP_ID;
		dev_info(priv->dev, "BMP280 Detected\n");
	}
	return ret;
}

static int bmp280_system_reset(struct bmp280_priv *priv){
    int ret;
    dev_info(priv->dev, "Reset BMP280 Device: ");
    ret = regmap_write(priv->regmap, BMP280_REG_RESET,BMP280_SYS_RESET_VALUE);
    if(ret){
        dev_err(priv->dev, "Reset Failed\n");
        return ret;
    }
    usleep_range(5000, 10000); // 5–10 ms
    /*Check systemr*/
    int val;
    ret = regmap_read(priv->regmap, BMP280_REG_RESET, &val);
    if(ret) return ret;
    if(val!=BMP280_SYS_RESET_VALUE){
        dev_info(priv->dev, "Success\n");
    }   
    return ret;
}

static const unsigned long bmp280_avail_scan_masks[] = {
	BIT(BMP280_TEMP) | BIT(BMP280_PRESS),
	0
};

static int bmp280_chip_config(struct bmp280_priv *priv)
{ 
	u8 osrs = FIELD_PREP(BMP280_OSRS_TEMP_MASK, priv->oversampling_temp + 1) |
		  FIELD_PREP(BMP280_OSRS_PRESS_MASK, priv->oversampling_press + 1);
	int ret;
	ret = regmap_write_bits(priv->regmap, BMP280_REG_CTRL_MEAS,
				BMP280_OSRS_TEMP_MASK |
				BMP280_OSRS_PRESS_MASK |
				BMP280_MODE_MASK,
				osrs | BMP280_MODE_NORMAL);
	if (ret) {
		dev_err(priv->dev, "failed to write ctrl_meas register\n");
		return ret;
	}
	ret = regmap_update_bits(priv->regmap, BMP280_REG_CONFIG,
				 BMP280_FILTER_MASK,
				 BMP280_FILTER_4X);
	if (ret) {
		dev_err(priv->dev, "failed to write config register\n");
		return ret;
	}
	return ret;
}

static int bmp280_read_calib(struct bmp280_priv *priv)
{
	struct bmp280_calib *calib = &priv->calib.bmp280;
	int ret;
	/* Read temperature and pressure calibration values. */
	ret = regmap_bulk_read(priv->regmap, BMP280_REG_COMP_TEMP_START,
			       priv->bmp280_cal_buf,
			       sizeof(priv->bmp280_cal_buf));
	if (ret) {
		dev_err(priv->dev,
			"failed to read calibration parameters\n");
		return ret;
	}
	/* Toss calibration data into the entropy pool */
	add_device_randomness(priv->bmp280_cal_buf,
			      sizeof(priv->bmp280_cal_buf));
	/* Parse temperature calibration values. */
	calib->T1 = le16_to_cpu(priv->bmp280_cal_buf[0]);
	calib->T2 = le16_to_cpu(priv->bmp280_cal_buf[1]);
	calib->T3 = le16_to_cpu(priv->bmp280_cal_buf[2]);
	/* Parse pressure calibration values. */
	calib->P1 = le16_to_cpu(priv->bmp280_cal_buf[3]);
	calib->P2 = le16_to_cpu(priv->bmp280_cal_buf[4]);
	calib->P3 = le16_to_cpu(priv->bmp280_cal_buf[5]);
	calib->P4 = le16_to_cpu(priv->bmp280_cal_buf[6]);
	calib->P5 = le16_to_cpu(priv->bmp280_cal_buf[7]);
	calib->P6 = le16_to_cpu(priv->bmp280_cal_buf[8]);
	calib->P7 = le16_to_cpu(priv->bmp280_cal_buf[9]);
	calib->P8 = le16_to_cpu(priv->bmp280_cal_buf[10]);
	calib->P9 = le16_to_cpu(priv->bmp280_cal_buf[11]);
	return 0;
}

///Table 5
static const int bmp280_oversampling_avail[] = { 1, 2, 4, 8, 16 };
static const int bmp280_temp_coeffs[] = { 10, 1 };
static const int bmp280_press_coeffs[] = { 1, 256000 };

static const struct bmp280_chip_info bmp280_chip_info = {
    .start_up_time = 2000,
    .avail_scan_masks = bmp280_avail_scan_masks,

    .oversampling_temp_avail = bmp280_oversampling_avail,
    .num_oversampling_temp_avail = ARRAY_SIZE(bmp280_oversampling_avail),
    .oversampling_temp_default = BMP280_OSRS_TEMP_2X - 1,

    .oversampling_press_avail = bmp280_oversampling_avail,
    .num_oversampling_press_avail = ARRAY_SIZE(bmp280_oversampling_avail),
    .oversampling_press_default = BMP280_OSRS_PRESS_16X - 1,

    .temp_coeffs = bmp280_temp_coeffs,
    .temp_coeffs_type = IIO_VAL_FRACTIONAL,

    .press_coeffs = bmp280_press_coeffs,
    .press_coeffs_type = IIO_VAL_FRACTIONAL,
};


int bmp280_probe(struct device *dev, struct regmap *regmap)
{
    struct iio_dev *iio_dev;
    struct bmp280_priv *priv;
    unsigned int chip_id;
	unsigned int i;  
    int ret;
    iio_dev = devm_iio_device_alloc(dev, sizeof(*priv));  
    if (!iio_dev)
		return -ENOMEM;
    priv = iio_priv(iio_dev);
    mutex_init(&priv->lock);
    iio_dev->name = DRIVER_NAME;
    iio_dev->dev.parent = dev;
	priv->regmap = regmap;
	priv->dev = dev;

	/*iio*/
	iio_dev->channels = bmp280dev_channels;
	iio_dev->num_channels = BMP280_NUM_CHANNELS;
	iio_dev->info = &bmp280dev_info;
    iio_dev->modes = INDIO_DIRECT_MODE;

	if(ret) return ret;
	/*Init*/
	priv->start_up_time = bmp280_chip_info.start_up_time;
	priv->oversampling_press = bmp280_chip_info.oversampling_press_default;
	priv->oversampling_temp = bmp280_chip_info.oversampling_temp_default;
	/* Wait to make sure we started up properly */
	usleep_range(priv->start_up_time,priv->start_up_time + 100);

	/*Check Chip ID*/
	ret = bmp280_chip_id(priv);
	if(ret) return ret;
	/*Retset Device*/
	ret = bmp280_system_reset(priv);
	if(ret) return ret;
	ret = bmp280_chip_config(priv);
	if (ret)
		return ret;
	dev_set_drvdata(dev, iio_dev);	
	/*
	 * Some chips have calibration parameters "programmed into the devices'
	 * non-volatile memory during production". Let's read them out at probe
	 * time once. They will not change.
	 */
	ret = bmp280_read_calib(priv);
	if(ret) return dev_err_probe(priv->dev, ret,
					     "failed to read calibration coefficients\n");
	ret = 	devm_iio_device_register(dev,iio_dev);	
	if(ret) return ret;			 
    return 0;
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);  
MODULE_VERSION(DRIVER_VERS);