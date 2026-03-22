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


static int bno055_get_chip_id(struct bno055_priv *priv);
static int bno055_set_page_id(struct bno055_priv *priv, enum bno055_page_id tar_page_id);
static int bno055_set_opr_mode(struct bno055_priv *priv,enum bno055_opr_mode opr_mode);

/*Config Function*/
static int bno_axis_remap_config(struct bno055_priv *priv,enum bno055_axis_remap_config  axis_remap_config);
static int bno_axis_remap_sign(struct bno055_priv *priv,enum bno055_axis_remap_sign  axis_remap_sign);
static int bno_axis_pwr_mode(struct bno055_priv *priv,enum bno055_power_mode  power_mode);

static int bno055_init(struct bno055_priv *priv);
static int bno055_system_reset(struct bno055_priv *priv);

/* ================= MODE TABLE ================= */
static const struct bno055_mode_map bno055_modes[] = {
	{ "CONFIG", BNO055_OPR_MODE_CONFIG },
	{ "ACCONLY", BNO055_OPR_MODE_ACCONLY },
	{ "MAGONLY", BNO055_OPR_MODE_MAGONLY },
	{ "GYRONLY", BNO055_OPR_MODE_GYROONLY },
	{ "ACCMAG", BNO055_OPR_MODE_ACCMAG },
	{ "ACCGYRO", BNO055_OPR_MODE_ACCGYRO },
	{ "MAGGYRO", BNO055_OPR_MODE_MAGGYRO },
	{ "AMG", BNO055_OPR_MODE_AMG },
	{ "IMU", BNO055_OPR_MODE_IMU },
	{ "COMPASS", BNO055_OPR_MODE_COMPASS },
	{ "M4G", BNO055_OPR_MODE_M4G },
	{ "NDOF_FMC_OFF", BNO055_OPR_MODE_NDOF_FMC_OFF },
	{ "NDOF", BNO055_OPR_MODE_NDOF },
};

/*IIO channel*/

/* =========================
 * Macro define channel
 * ========================= */
//in_<type>_<axis>_<info>

#define BNO055_CHANNEL(_type, _axis, _index, _address, _sep, _sh, _avail) {	\
	.address = _address,							\
	.type = _type,								\
	.modified = 1,								\
	.channel2 = IIO_MOD_##_axis,						\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | (_sep),			\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) | (_sh),		\
	.info_mask_shared_by_type_available = _avail,				\
	.scan_index = _index,							\
	.scan_type = {								\
		.sign = 's',							\
		.realbits = 16,							\
		.storagebits = 16,						\
		.endianness = IIO_LE,						\
		.repeat = IIO_MOD_##_axis == IIO_MOD_QUATERNION ? 4 : 0,        \
	},									\
}


/* scan indexes follow DATA register order */
enum bno055_scan_axis {
	BNO055_SCAN_ACCEL_X,
	BNO055_SCAN_ACCEL_Y,
	BNO055_SCAN_ACCEL_Z,
	BNO055_SCAN_MAGN_X,
	BNO055_SCAN_MAGN_Y,
	BNO055_SCAN_MAGN_Z,
	BNO055_SCAN_GYRO_X,
	BNO055_SCAN_GYRO_Y,
	BNO055_SCAN_GYRO_Z,
	BNO055_SCAN_YAW,
	BNO055_SCAN_ROLL,
	BNO055_SCAN_PITCH,
	BNO055_SCAN_QUATERNION,
	BNO055_SCAN_LIA_X,
	BNO055_SCAN_LIA_Y,
	BNO055_SCAN_LIA_Z,
	BNO055_SCAN_GRAVITY_X,
	BNO055_SCAN_GRAVITY_Y,
	BNO055_SCAN_GRAVITY_Z,
	BNO055_SCAN_TIMESTAMP,
	_BNO055_SCAN_MAX
};
/* =========================
 * Channel definitions
 * ========================= */

static const struct iio_chan_spec bno055_channels[] = {
	/* ================= accelerometer  ================= */
	


	/* ================= ACCEL ================= */
	// BNO055_CHAN(IIO_ACCEL, IIO_MOD_X, BNO055_REG_ACC_DATA_X_LSB),
	// BNO055_CHAN(IIO_ACCEL, IIO_MOD_Y, BNO055_REG_ACC_DATA_Y_LSB),
	// BNO055_CHAN(IIO_ACCEL, IIO_MOD_Z, BNO055_REG_ACC_DATA_Z_LSB),
	// /* ================= GYRO ================= */
	// BNO055_CHAN(IIO_ANGL_VEL, IIO_MOD_X, BNO055_REG_GYR_DATA_X_LSB),
	// BNO055_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, BNO055_REG_GYR_DATA_Y_LSB),
	// BNO055_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, BNO055_REG_GYR_DATA_Z_LSB),
	// /* ================= MAG ================= */
	// BNO055_CHAN(IIO_MAGN, IIO_MOD_X, BNO055_REG_MAG_DATA_X_LSB),
	// BNO055_CHAN(IIO_MAGN, IIO_MOD_Y, BNO055_REG_MAG_DATA_Y_LSB),
	// BNO055_CHAN(IIO_MAGN, IIO_MOD_Z, BNO055_REG_MAG_DATA_Z_LSB),
};


#define BNO055_NUM_CHANNELS ARRAY_SIZE(bno055_channels)

/*SET Page ID*/
static int bno055_set_page_id(struct bno055_priv *priv,
			      enum bno055_page_id tar_page_id)
{
	int ret;
	unsigned int pageid;
	ret = regmap_read(priv->regmap, BNO055_REG_PAGE_ID, &pageid);
	if (ret) {
		//dev_err(st->dev, "Failed to read page id\n");
		goto out;
	}

	if (pageid != tar_page_id) {

		ret = regmap_write(priv->regmap, BNO055_REG_PAGE_ID,
				   tar_page_id);
		if (ret) {
			//dev_err(st->dev, "Failed to set page id\n");	
			goto out;
		}

		priv->id.page_id = tar_page_id;

		//dev_info(st->dev, "Changed to page %u\n", tar_page_id);
	}

	ret = 0;

out:
	return ret;
}
/*Read Chip ID*/
static int bno055_get_chip_id(struct bno055_priv *priv){	
	int ret;
	bno055_set_page_id(priv,PAGE_ID_0);
	ret = regmap_read(priv->regmap, BNO055_REG_SW_REV_ID_LSB, &priv->id.SW_REV_ID_LSB);
		if (ret)
			return ret;
	ret = regmap_read(priv->regmap, BNO055_REG_SW_REV_ID_MSB, &priv->id.SW_REV_ID_MSB);
		if (ret)
			return ret;
	if (priv->id.SW_REV_ID_MSB != 0x3 || priv->id.SW_REV_ID_LSB != 0x11)
		dev_warn(dev, "Untested firmware version. Anglvel scale may not work as expected\n");
	/*Read ACC_ID*/
	ret = regmap_read(priv->regmap, BNO055_REG_ACC_ID, &priv->id.ACC_ID);
		if (ret)
		return ret;
	/*Read GYR_ID*/
	ret = regmap_read(priv->regmap, BNO055_REG_GYR_ID, &priv->id.GYR_ID);
		if (ret)
		return ret;
	/*Read MAG_ID*/
	ret = regmap_read(priv->regmap, BNO055_REG_MAG_ID, &priv->id.MAG_ID);
		if (ret)
		return ret;
	/*BL_REV*/
	ret = regmap_read(priv->regmap, BNO055_REG_BL_REV_ID, &priv->id.bl_rev_id);
		if (ret)
		return ret;
	dev_info(dev, "=== BNO055 Chip Info ===\n");

	dev_info(dev, "SW Revision: 0x%02X 0x%02X\n",
		 priv->id.SW_REV_ID_MSB,
		 priv->id.SW_REV_ID_LSB);
	dev_info(dev, "CHIP ID    : 0x%02X\n", priv->id.CHIP_ID);
	dev_info(dev, "ACC ID     : 0x%02X\n", priv->id.ACC_ID);
	dev_info(dev, "GYR ID     : 0x%02X\n", priv->id.GYR_ID);
	dev_info(dev, "MAG ID     : 0x%02X\n", priv->id.MAG_ID);

	dev_info(dev, "Bootloader : 0x%02X\n", priv->id.bl_rev_id);

	dev_info(dev, "========================\n");
	return ret;
}



/* ================= SET MODE ================= */
static int bno055_set_opr_mode(struct bno055_priv *priv,
			      enum bno055_opr_mode opr_mode)
{
	int ret;
	int cur_mode;
	/*check current opr mode*/
	ret = regmap_read(priv->regmap, BNO055_REG_OPR_MODE, &cur_mode);
	if(ret){
		dev_err(priv->dev, "Failed to read current Mode\n");
		return ret;
	}
	/* Already in target mode */
	if (cur_mode == opr_mode) {
        return 0;
    }
	/* Move to CONFIGMODE first if not already */
	if(cur_mode!=BNO055_OPR_MODE_CONFIG){
		int cfg_mode = BNO055_OPR_MODE_CONFIG;
		ret = regmap_write(priv->regmap, BNO055_REG_OPR_MODE,cfg_mode);
		if(ret){
			dev_err(priv->dev, "Failed to Set Config Mode\n");
			return ret;
		}
		msleep(20);
	}
	/* If target is CONFIGMODE we're done */
	if (opr_mode == BNO055_OPR_MODE_CONFIG) {
		priv->opr_mode = opr_mode;
		return 0;
	}
	/* Set target mode */
	ret = regmap_write(priv->regmap, BNO055_REG_OPR_MODE, opr_mode);
	if (ret)
		return ret;

	msleep(20);

	/* Verify mode */
	ret = regmap_read(priv->regmap, BNO055_REG_OPR_MODE, &cur_mode);
	if (ret)
		return ret;
	if (cur_mode != opr_mode)
		return -EIO;
	priv->opr_mode = opr_mode;
	return ret;
}

/* ================= SYSFS MODE ================= */
static ssize_t bno055_mode_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct bno055_priv *priv = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < ARRAY_SIZE(bno055_modes); i++)
		if (bno055_modes[i].val == priv->opr_mode)
			return sprintf(buf, "%s\n", bno055_modes[i].name);

	return sprintf(buf, "UNKNOWN\n");
}


static ssize_t bno055_mode_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct bno055_priv *priv = dev_get_drvdata(dev);
	int i, ret;

	mutex_lock(&st->lock);

	for (i = 0; i < ARRAY_SIZE(bno055_modes); i++) {
		if (sysfs_streq(buf, bno055_modes[i].name)) {
			ret = bno055_set_opr_mode(st,
						 bno055_modes[i].val);
			mutex_unlock(&st->lock);
			return ret ? ret : count;
		}
	}

	mutex_unlock(&st->lock);
	return -EINVAL;
}

static DEVICE_ATTR_RW(bno055_mode);

/* ================= READ RAW ================= */
static int bno055_read_axis(struct bno055_priv *priv,
			   u8 reg, int *val)
{
	u8 data[2];
	int ret;

	ret = regmap_bulk_read(st->regmap, reg, data, 2);
	if (ret)
		return ret;

	*val = (s16)((data[1] << 8) | data[0]);
	return 0;
}


static int bno055_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan,
			   int *val, int *val2, long mask)
{
	struct bno055_priv *priv = iio_priv(indio_dev);
	int ret;

	switch (mask) {

	case IIO_CHAN_INFO_RAW:
		mutex_lock(&st->lock);
		ret = bno055_read_axis(st, chan->address, val);
		mutex_unlock(&st->lock);
		if (ret)
			return ret;

		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {

		case IIO_ACCEL:
			*val = 0;
			*val2 = 9800; /* mg */
			return IIO_VAL_INT_PLUS_MICRO;

		case IIO_ANGL_VEL:
			*val = 0;
			*val2 = 87266; /* rad/s */
			return IIO_VAL_INT_PLUS_MICRO;

		case IIO_MAGN:
			*val = 0;
			*val2 = 1000;
			return IIO_VAL_INT_PLUS_MICRO;

		default:
			return -EINVAL;
		}
	}

	return -EINVAL;
}


static int bno055_system_reset(struct bno055_priv *priv){
	priv->id.page_id = PAGE_ID_0;
	// ret = regmap_write(priv->regmap, BNO055_REG_PAGE_ID,
	// 		priv->id.page_id);
	bno055_set_page_id(priv,PAGE_ID_0);
	dev_info(priv->dev, "Reset BNO055 Device: ");
	int tmp = BNO055_SYS_TRIGGER_RST_SYS;	
	ret = regmap_write(priv->regmap, BNO055_REG_SYS_TRIGGER,tmp);	if(ret){
	dev_err(priv->dev, "Reset Failed\n");
		return ret;
	}
	regcache_drop_region(priv->regmap, 0x0, 0xff);
	usleep_range(650000, 700000);
	//set BNO055_REG_SYS_TRIGGER TO 0x00
	tmp = BNO055_SYS_DEFAULT_SYSTEM;
	ret = regmap_write(priv->regmap, BNO055_REG_SYS_TRIGGER,tmp);
	if(ret){
		dev_err(priv->dev, "Reset Failed\n");
		return ret;
	}
	dev_info(priv->dev, "Success\n");
	usleep_range(55000, 60000);
	/*Set Operation Mode to CONFIG Mode*/
	dev_info(priv->dev, "Set Operation Mode: CONFIG_MODE");
	ret = bno055_set_opr_mode(priv,BNO055_OPR_MODE_CONFIG);
	return ret;
}

/*CONFIG FUNCTION*/
static int bno_axis_remap_config(struct bno055_priv *priv,enum bno055_axis_remap_config  axis_remap_config){
	int ret;
	bno055_set_page_id(priv,PAGE_ID_0);
	int = tmp;
	switch(axis_remap_config){
		case REMAP_CONFIG_P0_3_5_6:
			tmp = REMAP_CONFIG_P0_3_5_6;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_CONFIG,tmp);
			break;
		case REMAP_CONFIG_P1_2_4_7:
			tmp = REMAP_CONFIG_P1_2_4_7;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_CONFIG,tmp);
			break;
	}
	return ret;
}

static int bno_axis_remap_sign(struct bno055_priv *priv,enum bno055_axis_remap_sign  axis_remap_sign){
	int ret;
	bno055_set_page_id(priv,PAGE_ID_0);
	int = tmp;
	switch(axis_remap_sign){
		case BNO055_AXIS_SIGN_P0:
			tmp = BNO055_AXIS_SIGN_P0;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_SIGN,tmp);
			break;
		case BNO055_AXIS_SIGN_P1:
			tmp = BNO055_AXIS_SIGN_P1;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_SIGN,tmp);
			break;
		case BNO055_AXIS_SIGN_P2:
			tmp = BNO055_AXIS_SIGN_P2;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_SIGN,tmp);
			break;
		case BNO055_AXIS_SIGN_P3:
			tmp = BNO055_AXIS_SIGN_P3;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_SIGN,tmp);
			break;
		case BNO055_AXIS_SIGN_P4:
			tmp = BNO055_AXIS_SIGN_P4;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_SIGN,tmp);
			break;
		case BNO055_AXIS_SIGN_P5:
			tmp = BNO055_AXIS_SIGN_P5;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_SIGN,tmp);
			break;
		case BNO055_AXIS_SIGN_P6:
			tmp = BNO055_AXIS_SIGN_P6;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_SIGN,tmp);
			break;
		case BNO055_AXIS_SIGN_P7:
			tmp = BNO055_AXIS_SIGN_P7;
			ret = regmap_write(priv->regmap, BNO055_REG_AXIS_MAP_SIGN,tmp);
			break;
	}
	return ret;
}

static int bno_axis_pwr_mode(struct bno055_priv *priv,enum bno055_power_mode  power_mode){
	int ret;
	bno055_set_page_id(priv,PAGE_ID_0);
	int tmp;
	switch(power_mode){
		case BNO055_POWER_NORMAL:
			tmp = BNO055_POWER_NORMAL;
			ret = regmap_write(priv->regmap, BNO055_REG_PWR_MODE,tmp);
			break;
		case BNO055_POWER_LOW:
			tmp = BNO055_POWER_LOW;
			ret = regmap_write(priv->regmap, BNO055_REG_PWR_MODE,tmp);
			break;
		case BNO055_POWER_SUSPEND:
			tmp = BNO055_POWER_SUSPEND;
			ret = regmap_write(priv->regmap, BNO055_REG_PWR_MODE,tmp);
			break;	
	}
	return ret;
}

/*acceleration configuration*/
static int bno_acc_config(struct bno055_priv *priv,int g_range,int Bandwidth,int OPRMode ){
	int ret;
	bno055_set_page_id(priv,PAGE_ID_1);
	int tmp;
	priv->acc_gyr_mag_valuation.acc_g_range = g_range;
	priv->acc_gyr_mag_valuation.acc_bandwidth = Bandwidth;
	priv->acc_gyr_mag_valuation.acc_mode = OPRMode;
	int tmp;
	tmp = (priv->acc_gyr_mag_valuation.acc_mode | priv->acc_gyr_mag_valuation.acc_bandwidth)| priv->acc_gyr_mag_valuation.acc_g_range;
	ret = regmap_write(priv->regmap, BNO055_REG_ACC_CONFIG,tmp);
	return ret;
}	

/*gyroscope configuration*/
static int bno_gyr_config(struct bno055_priv *priv,int g_range,int Bandwidth,int OPRMode ){
	int ret;
	bno055_set_page_id(priv,PAGE_ID_1);
	priv->acc_gyr_mag_valuation.gyr_range = g_range;
	priv->acc_gyr_mag_valuation.gyr_bandwidth = Bandwidth;
	priv->acc_gyr_mag_valuation.gyr_mode = OPRMode;
	int tmp_cf0,tmp_cf1;
	tmp_cf0 =  priv->acc_gyr_mag_valuation.gyr_bandwidth|priv->acc_gyr_mag_valuation.gyr_range;
	ret = regmap_write(priv->regmap, BNO055_REG_GYR_CONFIG_0,tmp_cf0);
	tmp_cf1 = priv->acc_gyr_mag_valuation.gyr_mode;
	ret = regmap_write(priv->regmap, BNO055_REG_GYR_CONFIG_1,tmp_cf1);
	return ret;
}

static int bno_mag_config(struct bno055_priv *priv,int data_rate,int OPRMode,int PWRMode){
	int ret;
	bno055_set_page_id(priv,PAGE_ID_1);
	priv->acc_gyr_mag_valuation.mag_data_rate = data_rate;
	priv->acc_gyr_mag_valuation.mag_operation_mode = OPRMode;
	priv->acc_gyr_mag_valuation.mag_pwr_mode = PWRMode;
	int tmp;
	tmp =  (priv->acc_gyr_mag_valuation.mag_pwr_mode|priv->acc_gyr_mag_valuation.mag_operation_mode) | priv->acc_gyr_mag_valuation.mag_data_rate;
	ret = regmap_write(priv->regmap, BNO055_REG_MAG_CONFIG,tmp);
	return ret;
}

static int bno_set_unit(struct bno055_priv *priv,int acc,int angular,int euler, int temp, int  fusion_dof){
	int ret;
	int tmp;
	bno055_set_page_id(priv,PAGE_ID_1);
	priv->acc_gyr_mag_valuation.acc_linearacc_gravityvector_unit = acc;
	priv->acc_gyr_mag_valuation.angular_rate_gyr_unit = angular;
	priv->acc_gyr_mag_valuation.euler_angles_unit = euler;
	priv->acc_gyr_mag_valuation.temp_unit = temp;
	priv->acc_gyr_mag_valuation.fusion_dof = fusion_dof;
	tmp = priv->acc_gyr_mag_valuation.fusion_dof |
	      priv->acc_gyr_mag_valuation.temp_unit |
	      priv->acc_gyr_mag_valuation.euler_angles_unit |
	      priv->acc_gyr_mag_valuation.angular_rate_gyr_unit |
	      priv->acc_gyr_mag_valuation.acc_linearacc_gravityvector_unit;
	ret = regmap_write(priv->regmap, BNO055_REG_UNIT_SEL,tmp);
	/*Set Scale*/
	
}

static int bno055_init(struct bno055_priv *priv)
{
	int ret;
	/*Set Axis Remap Config*/
	ret = bno_axis_remap_config(priv,REMAP_CONFIG_P1_2_4_7);
	if(ret) dev_err(priv->dev, "Failed to axis remap configuration\n");
	/*Set Axis Remap Sign*/
	ret bno_axis_remap_sign(priv,BNO055_AXIS_SIGN_P1);
	if(ret) dev_err(priv->dev, "Failed to axis remap sign\n");
	/*Set Power Mode*/
	ret = bno_axis_pwr_mode(priv,BNO055_POWER_NORMAL);
	if(ret) dev_err(priv->dev, "Failed to Set Power Mode\n");
	/*acceleration configuration*/
	ret = bno_acc_config(priv,BNO055_ACC_RANGE_2G,BNO055_ACC_BW_62_5HZ,BNO055_ACC_OPMODE_NORMAL);
	if(ret) dev_err(priv->dev, "Failed to Set Acc Config\n");
	/*gyroscope configuration*/
	ret = bno_gyr_config(priv,BNO055_GYR_RANGE_2000DPS,BNO055_GYR_BW_32HZ,BNO055_GYR_OPMODE_NORMAL);
	if(ret) dev_err(priv->dev, "Failed to Set Gyr Config\n");
	/*mag configuration*/
	ret = bno_mag_config(priv,BNO055_MAG_ODR_20HZ,BNO055_MAG_OPMODE_ENH_REGULAR,BNO055_MAG_PWR_FORCE);
	if(ret) dev_err(priv->dev, "Failed to Set Mag Config\n");
	/*Set unit*/

	/*Set Temperature Source*/
	/*Move to page 0 for read*/
	/*Delay*/
	/*Set Operation Mode*/

	return ret;
}

/* ================= IIO INFO ================= */
static const struct iio_info bno055_info = {
	
};


int bno055_probe(struct device *dev, struct regmap *regmap)
{
	struct bno055_priv *priv;
	struct iio_dev *iio_dev;
	int ret;
	iio_dev = devm_iio_device_alloc(dev, sizeof(*priv));
	if (!iio_dev)
		return -ENOMEM;
	iio_dev->name = DRIVER_NAME;
	priv = iio_priv(iio_dev);
	mutex_init(&priv->lock);
	priv->regmap = regmap;
	priv->dev = dev;
	ret = regmap_read(priv->regmap, BNO055_REG_CHIP_ID, &priv->id.CHIP_ID);
	if (ret)
		return ret;
	if(priv->id.CHIP_ID!=BNO055_CHIP_ID){
		dev_warn(dev, "Unrecognized Chip ID 0x%x\n", priv->id.CHIP_ID);
		return -ENODEV;
	}
	else{
		dev_info(dev, "BNO055 Detected\n");
	}
	/*Reset*/
	ret = bno055_system_reset(priv);
	if(ret) return ret;
	/*Read chip ID*/
	ret = bno055_get_chip_id(priv);
	if(ret) return ret;
	/*Bno055 Init*/
	
}

void bno055_remove(struct device *dev, struct regmap *regmap)
{

}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);  
MODULE_VERSION(DRIVER_VERS);