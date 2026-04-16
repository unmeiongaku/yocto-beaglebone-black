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


static int bmp280_set_mode(struct bmp280_priv *priv, enum bmp280_opr_mode mode){
	int tmp,currentmode;
	int ret;
	/*Check Current Mode*/
	ret = regmap_read(priv->regmap, BMP280_REG_CTRL_MEAS, &tmp);
	if(ret) return ret;
	currentmode = tmp & 0x03;
	if(currentmode == mode) return 0;
	if(currentmode != mode){
		ret = regmap_write(priv->regmap, BMP280_REG_CTRL_MEAS,mode);
		if(ret) return ret;
	}
	usleep_range(1000, 2000);
	/**/	
	/*Recheck*/
	ret = regmap_read(priv->regmap, BMP280_REG_CTRL_MEAS, &tmp);
	if(ret) return ret;
	if(tmp == mode){
		priv->mode = mode;
	}
	return 0;
}

static int bmp280_set_osrs(struct bmp280_priv *priv,enum bmp280_osrs_type type,u8 osrs){
	int ret;
	int tmp;
	switch(type){
		case OSRS_TEMP:
			switch(osrs){
				case BMP280_OSRS_TEMP_SKIP: priv->config.osrst = 0; break;
				case BMP280_OSRS_TEMP_1X: priv->config.osrst = 1; break;
				case BMP280_OSRS_TEMP_2X: priv->config.osrst = 2; break;
				case BMP280_OSRS_TEMP_4X: priv->config.osrst = 4; break;
				case BMP280_OSRS_TEMP_8X: priv->config.osrst = 8; break;
				case BMP280_OSRS_TEMP_16X: priv->config.osrst = 16; break;
			}
			tmp = osrs << 5;
			ret = regmap_write(priv->regmap, BMP280_REG_CTRL_MEAS,tmp);
			if(ret) return 0;
		break;
		case OSRS_PRESS:
			switch(osrs){
				case BMP280_OSRS_PRESS_SKIP: priv->config.osrsp = 0; break;
				case BMP280_OSRS_PRESS_1X: priv->config.osrsp = 1; break;
				case BMP280_OSRS_PRESS_2X: priv->config.osrsp = 2; break;
				case BMP280_OSRS_PRESS_4X: priv->config.osrsp = 4; break;
				case BMP280_OSRS_PRESS_8X: priv->config.osrsp = 8; break;
				case BMP280_OSRS_PRESS_16X: priv->config.osrsp = 16; break;
			}
			tmp = (osrs & 0x03) << 2;
			ret = regmap_write(priv->regmap, BMP280_REG_CTRL_MEAS,tmp);
			if(ret) return 0;
		break;
		default:
        	return -EINVAL;
	}
	return 0;
}

static int bmp280_set_config(struct bmp280_priv *priv,
                            enum bmp280_t_sb_standby enum_t_sb,
                            enum bmp280_filter_iir enum_filter,
                            u8 spi3w_en)
{
    int ret;
    u8 config;
    /* Set sleep mode */
    ret = bmp280_set_mode(priv, SLEEP_MODE);
    if (ret)
        return ret;

	if(spi3w_en!=0) spi3w_en = 1;

    /* Store config */
    priv->config.enum_t_sb = enum_t_sb;
    priv->config.enum_filter = enum_filter;
    priv->config.spi3w_en = spi3w_en;

    /* Convert standby → us */
    switch (priv->config.enum_t_sb) {
    case BMP280_TSB_0_5:   priv->config.t_sb_us = 500; break;
    case BMP280_TSB_62_5:  priv->config.t_sb_us = 62500; break;
    case BMP280_TSB_125:   priv->config.t_sb_us = 125000; break;
    case BMP280_TSB_250:   priv->config.t_sb_us = 250000; break;
    case BMP280_TSB_500:   priv->config.t_sb_us = 500000; break;
    case BMP280_TSB_1000:  priv->config.t_sb_us = 1000000; break;
    case BMP280_TSB_2000:  priv->config.t_sb_us = 2000000; break;
    case BMP280_TSB_4000:  priv->config.t_sb_us = 4000000; break;
    default:
        return -EINVAL;
    }

    /* Convert filter → delay samples */
    switch (priv->config.enum_filter) {
    case BMP280_FILTER_OFF: priv->config.filter_delay_samples = 1; break;
    case BMP280_FILTER_2X:   priv->config.filter_delay_samples = 2; break;
    case BMP280_FILTER_4X:   priv->config.filter_delay_samples = 5; break;
    case BMP280_FILTER_8X:   priv->config.filter_delay_samples = 11; break;
    case BMP280_FILTER_16X:  priv->config.filter_delay_samples = 22; break;
    default:
        return -EINVAL;
    }

    /* Build config register (mask để an toàn) */
    config = ((priv->config.enum_t_sb & 0x07) << 5) |
             ((priv->config.enum_filter & 0x07) << 2) |
             (priv->config.spi3w_en);

    ret = regmap_write(priv->regmap, BMP280_REG_CONFIG, config);
    if (ret)
        return ret;
    return 0;
}

static int bmp280_wait_data_ready(struct bmp280_priv *priv)
{
    int ret, val;
    int timeout = 20; // tùy theo oversampling

    do {
        ret = regmap_read(priv->regmap, BMP280_REG_STATUS, &val);
        if (ret)
            return ret;

        /* measuring = 0 → data ready */
        if (!(val & BMP280_STATUS_MEASURING))
            return 0;

        usleep_range(2000, 3000);
    } while (--timeout);

    dev_err(priv->dev, "Data not ready (timeout)\n");
    return -ETIMEDOUT;
}

static int bmp280_system_reset(struct bmp280_priv *priv)
{
    int ret, val;
    int timeout = 10;

    dev_info(priv->dev, "Reset BMP280 Device\n");

    ret = regmap_write(priv->regmap,
                       BMP280_REG_RESET,
                       BMP280_SYS_RESET_VALUE);
    if (ret) {
        dev_err(priv->dev, "Reset failed\n");
        return ret;
    }

	priv->mode = SLEEP_MODE;

    usleep_range(2000, 4000);

    do {
        ret = regmap_read(priv->regmap, BMP280_REG_STATUS, &val);
        if (ret)
            return ret;

        if (!(val & BMP280_STATUS_IM_UPDATE)) {
            dev_dbg(priv->dev, "BMP280 Device Ready\n");
            return 0;
        }

        usleep_range(2000, 3000);
    } while (--timeout);

    dev_err(priv->dev, "Reset timeout\n");
    return -ETIMEDOUT;
}

static const unsigned long bmp280_avail_scan_masks[] = {
	BIT(BMP280_TEMP) | BIT(BMP280_PRESS),
	0
};


static int bmp280_set_use_case_normal_config(struct bmp280_priv *priv, enum bmp280_use_case_normal ucase){
	int ret;
	switch(ucase){
		case HANDHELD_DEVICE_LOW_POWER:
			ret = bmp280_set_config(priv,BMP280_TSB_62_5,BMP280_FILTER_4X,0);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_PRESS,BMP280_OSRS_PRESS_16X);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_TEMP,BMP280_OSRS_TEMP_2X);
			if(ret) return ret;
			break;
		case HANDHELD_DEVICEC_DYNAMIC:
			ret = bmp280_set_config(priv,BMP280_TSB_0_5,BMP280_OSRS_PRESS_16X,0);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_PRESS,BMP280_OSRS_PRESS_4X);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_TEMP,BMP280_OSRS_TEMP_1X);
			if(ret) return ret;
			break;
		case ELEVATOR_FLOOR_CHANGE_DETECTION:
			ret = bmp280_set_config(priv,BMP280_TSB_125,BMP280_FILTER_4X,0);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_PRESS,BMP280_OSRS_PRESS_4X);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_TEMP,BMP280_OSRS_TEMP_1X);
			if(ret) return ret;
			break;
		case DROP_DETECTION:
			ret = bmp280_set_config(priv,BMP280_TSB_0_5,BMP280_FILTER_OFF,0);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_PRESS,BMP280_OSRS_PRESS_2X);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_TEMP,BMP280_OSRS_TEMP_1X);
			if(ret) return ret;
			break;
		case INDOOR_NAVIGATION:
			ret = bmp280_set_config(priv,BMP280_TSB_0_5,BMP280_FILTER_16X,0);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_PRESS,BMP280_OSRS_PRESS_16X);
			if(ret) return ret;
			ret = bmp280_set_osrs(priv,OSRS_TEMP,BMP280_OSRS_TEMP_2X);
			if(ret) return ret;
			break;
	}
	ret = bmp280_set_mode(priv,NORMAL_MODE);
	if(ret) return ret;
	return 0;
}

static void bmp280_dump_calib(struct bmp280_priv *priv)
{
    dev_info(priv->dev, "=== BMP280 Calibration Data ===\n");

    dev_info(priv->dev, "T1 = %u\n", priv->calib.T1);
    dev_info(priv->dev, "T2 = %d\n", priv->calib.T2);
    dev_info(priv->dev, "T3 = %d\n", priv->calib.T3);

    dev_info(priv->dev, "P1 = %u\n", priv->calib.P1);
    dev_info(priv->dev, "P2 = %d\n", priv->calib.P2);
    dev_info(priv->dev, "P3 = %d\n", priv->calib.P3);
    dev_info(priv->dev, "P4 = %d\n", priv->calib.P4);
    dev_info(priv->dev, "P5 = %d\n", priv->calib.P5);
    dev_info(priv->dev, "P6 = %d\n", priv->calib.P6);
    dev_info(priv->dev, "P7 = %d\n", priv->calib.P7);
    dev_info(priv->dev, "P8 = %d\n", priv->calib.P8);
    dev_info(priv->dev, "P9 = %d\n", priv->calib.P9);

    dev_info(priv->dev, "===============================\n");
}

static int bmp280_read_calib(struct bmp280_priv *priv)
{
	int ret;
	/* Read temperature and pressure calibration values. */
	ret = regmap_bulk_read(priv->regmap, BMP280_REG_COMP_TEMP_START,
			       priv->calib.bmp280_cal_buf,
			       sizeof(priv->calib.bmp280_cal_buf));
	if (ret) {
		dev_err(priv->dev,
			"failed to read calibration parameters\n");
		return ret;
	}
	/* Toss calibration data into the entropy pool */
	add_device_randomness(priv->calib.bmp280_cal_buf,
			      sizeof(priv->calib.bmp280_cal_buf));
	/* Parse temperature calibration values. */
	priv->calib.T1 = le16_to_cpu(priv->calib.bmp280_cal_buf[0]);
	priv->calib.T2 = le16_to_cpu(priv->calib.bmp280_cal_buf[1]);
	priv->calib.T3 = le16_to_cpu(priv->calib.bmp280_cal_buf[2]);
	/* Parse pressure calibration values. */
	priv->calib.P1 = le16_to_cpu(priv->calib.bmp280_cal_buf[3]);
	priv->calib.P2 = le16_to_cpu(priv->calib.bmp280_cal_buf[4]);
	priv->calib.P3 = le16_to_cpu(priv->calib.bmp280_cal_buf[5]);
	priv->calib.P4 = le16_to_cpu(priv->calib.bmp280_cal_buf[6]);
	priv->calib.P5 = le16_to_cpu(priv->calib.bmp280_cal_buf[7]);
	priv->calib.P6 = le16_to_cpu(priv->calib.bmp280_cal_buf[8]);
	priv->calib.P7 = le16_to_cpu(priv->calib.bmp280_cal_buf[9]);
	priv->calib.P8 = le16_to_cpu(priv->calib.bmp280_cal_buf[10]);
	priv->calib.P9 = le16_to_cpu(priv->calib.bmp280_cal_buf[11]);
	dev_info(priv->dev, "BMP280 Calibrated\n");
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
	if (ret)
		return ret;
	/*Read Calibration Data*/
	ret = bmp280_read_calib(priv);
	if(ret) return dev_err_probe(priv->dev, ret,
					     "failed to read calibration coefficients\n");
	bmp280_dump_calib(priv);					 
	/*Set Device Config*/
	ret = bmp280_set_use_case_normal_config(priv,INDOOR_NAVIGATION);
	if(ret) return 0;
	dev_info(priv->dev, "BMP280 Success\n");
	dev_set_drvdata(dev, iio_dev);	
	ret = 	devm_iio_device_register(dev,iio_dev);	
	if(ret) return ret;			 
    return 0;
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);  
MODULE_VERSION(DRIVER_VERS);