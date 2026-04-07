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


static bool bno055_regmap_readable(struct device *dev, unsigned int reg);
static bool bno055_regmap_volatile(struct device *dev, unsigned int reg);
static bool bno055_regmap_writeable(struct device *dev, unsigned int reg);

static int bno055_get_chip_id(struct bno055_priv *priv);
static int bno055_set_page_id(struct bno055_priv *priv, enum bno055_page_id tar_page_id);
static int bno055_set_opr_mode(struct bno055_priv *priv,enum bno055_opr_mode opr_mode);

/*Config Function*/
static int bno_axis_remap_config(struct bno055_priv *priv,enum bno055_axis_remap_config  axis_remap_config);
static int bno_axis_remap_sign(struct bno055_priv *priv,enum bno055_axis_remap_sign  axis_remap_sign);
static int bno_axis_pwr_mode(struct bno055_priv *priv,enum bno055_power_mode  power_mode);
static int bno_acc_config(struct bno055_priv *priv,int g_range,int Bandwidth,int OPRMode );
static int bno_gyr_config(struct bno055_priv *priv,int g_range,int Bandwidth,int OPRMode );
static int bno_mag_config(struct bno055_priv *priv,int data_rate,int OPRMode,int PWRMode);
static int bno_set_unit(struct bno055_priv *priv,int acc,int angular,int euler, int temp, int  fusion_dof);
static int bno_set_temperature_src(struct bno055_priv *priv,enum bno055_temp_source temp_source);

static int bno055_init(struct bno055_priv *priv);
static int bno055_system_reset(struct bno055_priv *priv);


// /*sysfs_atrr*/
struct bno055_sysfs_attr {
	int *vals;
	int len;
	int *fusion_vals;
	int *hw_xlate;
	int type;
};
// /*Accelerometer*/
static int bno055_acc_lpf_bandwidths_vals[] = {
	7, 810000, 15, 630000, 31, 250000, 62, 500000,
	125, 0, 250, 0, 500, 0, 1000, 0,
};

static struct bno055_sysfs_attr bno055_acc_lpf_bw = {
	.vals = bno055_acc_lpf_bandwidths_vals,
	.len = ARRAY_SIZE(bno055_acc_lpf_bandwidths_vals),
	.fusion_vals = (int[]){62, 500000}, //default value in fusion mode
	.type = IIO_VAL_INT_PLUS_MICRO, //*val + (*val2 / 1,000,000)
};

static int bno055_acc_range_vals[] = {
  /* G:    2,    4,    8,    16 */
	1962, 3924, 7848, 15696
};

static struct bno055_sysfs_attr bno055_acc_range = {
	.vals = bno055_acc_range_vals,
	.len = ARRAY_SIZE(bno055_acc_range_vals),
	.fusion_vals = (int[]){3924}, /* 4G */
	.type = IIO_VAL_INT,
};

/*Gyroscope*/
/*
 * dps = hwval * (dps_range/2^15)
 * rps = hwval * (rps_range/2^15)
 *     = hwval * (dps_range/(2^15 * k))
 * where k is rad-to-deg factor
 */
static int bno055_gyr_scale_vals[] = {
	125, 1877467, 250, 1877467, 500, 1877467,
	1000, 1877467, 2000, 1877467,
};

static struct bno055_sysfs_attr bno055_gyr_scale = {
	.vals = bno055_gyr_scale_vals,
	.len = ARRAY_SIZE(bno055_gyr_scale_vals),
	.fusion_vals = (int[]){1, 900},
	.hw_xlate = (int[]){4, 3, 2, 1, 0},
	.type = IIO_VAL_FRACTIONAL,
};

static int bno055_gyr_lpf_vals_bandwidths[] = {12, 23, 32, 47, 64, 116, 230, 523};
static struct bno055_sysfs_attr bno055_gyr_lpf_bw = {
	.vals = bno055_gyr_lpf_vals_bandwidths,
	.len = ARRAY_SIZE(bno055_gyr_lpf_vals_bandwidths),
	.fusion_vals = (int[]){32}, //default
	.hw_xlate = (int[]){5, 4, 7, 3, 6, 2, 1, 0},
	.type = IIO_VAL_INT,
};

static int bno055_mag_odr_vals[] = {2, 6, 8, 10, 15, 20, 25, 30};
static struct bno055_sysfs_attr bno055_mag_odr = {
	.vals = bno055_mag_odr_vals,
	.len =  ARRAY_SIZE(bno055_mag_odr_vals),
	.fusion_vals = (int[]){20},
	.type = IIO_VAL_INT,
};

static int bno055_get_regmask(struct bno055_priv *priv, int *val, int *val2,
			      int reg, int mask, struct bno055_sysfs_attr *attr)
{
	const int shift = __ffs(mask); //return low bit position //shift = __ffs(0b00001110) = 1    //shift = __ffs(0b00000111) = 0
	int hwval, idx;
	int ret;
	int i;

	ret = regmap_read(priv->regmap, reg, &hwval); 
	if (ret)
		return ret;

	idx = (hwval & mask) >> shift;
	if (attr->hw_xlate)
		for (i = 0; i < attr->len; i++)
			if (attr->hw_xlate[i] == idx) {
				idx = i;
				break;
			}
	if (attr->type == IIO_VAL_INT) {
		*val = attr->vals[idx];
	} else { /* IIO_VAL_INT_PLUS_MICRO or IIO_VAL_FRACTIONAL */
		*val = attr->vals[idx * 2];
		*val2 = attr->vals[idx * 2 + 1];
	}

	return attr->type;
}

static bool bno055_regmap_volatile(struct device *dev, unsigned int reg)
{
	/* data and status registers */
	if (reg >= BNO055_REG_ACC_DATA_X_LSB && reg <= BNO055_REG_SYS_ERR)
		return true;
	/* when in fusion mode, config is updated by chip */
	if (reg == BNO055_REG_MAG_CONFIG ||
	    reg == BNO055_REG_ACC_CONFIG ||
	    reg == BNO055_REG_GYR_CONFIG_0 || reg == BNO055_REG_GYR_CONFIG_1)
		return true;
	/* calibration data may be updated by the IMU */
	if (reg >= BNO055_CALDATA_START && reg <= BNO055_CALDATA_END)
		return true;
	return false;
}

static bool bno055_regmap_readable(struct device *dev, unsigned int reg)
{
	/* unnamed PG0 reserved areas */
	if((reg < BNO055_PG1(0) && reg > BNO055_CALDATA_END) 
				|| reg == 0x3C 
				|| (reg > BNO055_REG_AXIS_MAP_SIGN && reg < BNO055_CALDATA_START)) return false;		
	/* unnamed PG1 reserved areas */
	if(reg > BNO055_PG1(BNO055_REG_BNO_UNIQUE_ID_END)
				|| (reg>BNO055_PG1(BNO055_REG_GYR_AM_SET) && reg < BNO055_PG1(BNO055_REG_BNO_UNIQUE_ID_START))
				|| reg == BNO055_PG1(0x0E)
				|| (reg>=BNO055_PG1(BNO055_REG_PG1_START) && reg < BNO055_PG1(BNO055_PAGESEL_REG))) return false;
	return true;
}

static bool bno055_regmap_writeable(struct device *dev, unsigned int reg)
{
	/*
	 * Unreadable registers are indeed reserved; there are no WO regs
	 * (except for a single bit in SYS_TRIGGER register)
	 */
	if (!bno055_regmap_readable(dev, reg))
		return false;
	/* data and status registers */
	if(reg >= BNO055_REG_ACC_DATA_X_LSB && reg <= BNO055_REG_SYS_ERR) return false;
	/* ID areas */
	if(reg<BNO055_PAGESEL_REG || (reg <=BNO055_PG1(BNO055_REG_BNO_UNIQUE_ID_END) && reg >= BNO055_PG1(BNO055_REG_BNO_UNIQUE_ID_START))) return false;
	return true;
}

static const struct regmap_range_cfg bno055_regmap_ranges[] = {
	{
		.range_min = 0,
		.range_max = 0x7f * 2,
		.selector_reg = BNO055_PAGESEL_REG,
		.selector_mask = GENMASK(7, 0),
		.selector_shift = 0,
		.window_start = 0,
		.window_len = 0x80,
	},
};
const struct regmap_config bno055dev_regmap_config = {
	.name = DRIVER_NAME,
	.reg_bits = 8,
	.val_bits = 8,
	.ranges = bno055_regmap_ranges,
	.num_ranges = 1,
	.volatile_reg = bno055_regmap_volatile,
	.max_register = 0x80 * 2,
	.writeable_reg = bno055_regmap_writeable,
	.readable_reg = bno055_regmap_readable,
	.cache_type = REGCACHE_RBTREE,
};

EXPORT_SYMBOL_NS_GPL(bno055dev_regmap_config, IIO_BNO055);

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

/* ================= IIO INFO ================= */

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
	_BNO055_SCAN_MAX,
};
/* =========================
 * Channel definitions
 * ========================= */

static const struct iio_chan_spec bno055_channels[] = {
	/* ================= accelerometer  ================= */
	BNO055_CHANNEL(IIO_ACCEL,X,BNO055_SCAN_ACCEL_X,
					BNO055_REG_ACC_DATA_X_LSB,BIT(IIO_CHAN_INFO_OFFSET),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY)),
	BNO055_CHANNEL(IIO_ACCEL,Y,BNO055_SCAN_ACCEL_Y,
					BNO055_REG_ACC_DATA_Y_LSB,BIT(IIO_CHAN_INFO_OFFSET),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY)),	

	BNO055_CHANNEL(IIO_ACCEL,Z,BNO055_SCAN_ACCEL_Z,
					BNO055_REG_ACC_DATA_Z_LSB,BIT(IIO_CHAN_INFO_OFFSET),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY)),	
	/* ================= gyroscope ================= */
	BNO055_CHANNEL(IIO_ANGL_VEL,X,BNO055_SCAN_GYRO_X,
					BNO055_REG_GYR_DATA_X_LSB,BIT(IIO_CHAN_INFO_OFFSET),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY)),
	BNO055_CHANNEL(IIO_ANGL_VEL,Y,BNO055_SCAN_GYRO_Y,
					BNO055_REG_GYR_DATA_Y_LSB,BIT(IIO_CHAN_INFO_OFFSET),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY)),	
	BNO055_CHANNEL(IIO_ANGL_VEL,Z,BNO055_SCAN_GYRO_Z,
					BNO055_REG_GYR_DATA_Z_LSB,BIT(IIO_CHAN_INFO_OFFSET),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY),
					BIT(IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY)),	
	/* ================= magnetometer ================= */
	BNO055_CHANNEL(IIO_MAGN,X,BNO055_SCAN_MAGN_X,
					BNO055_REG_MAG_DATA_X_LSB,BIT(IIO_CHAN_INFO_OFFSET),
					BIT(IIO_CHAN_INFO_SAMP_FREQ),
					BIT(IIO_CHAN_INFO_SAMP_FREQ)),
	BNO055_CHANNEL(IIO_MAGN,Y,BNO055_SCAN_MAGN_Y,
					BNO055_REG_MAG_DATA_Y_LSB,BIT(IIO_CHAN_INFO_OFFSET),
					BIT(IIO_CHAN_INFO_SAMP_FREQ),
					BIT(IIO_CHAN_INFO_SAMP_FREQ)),	
	BNO055_CHANNEL(IIO_MAGN,Z,BNO055_SCAN_MAGN_Z,
					BNO055_REG_MAG_DATA_Z_LSB,BIT(IIO_CHAN_INFO_OFFSET),
					BIT(IIO_CHAN_INFO_SAMP_FREQ),
					BIT(IIO_CHAN_INFO_SAMP_FREQ)),	
	/* ================= euler angle ================= */
	BNO055_CHANNEL(IIO_ROT, YAW, BNO055_SCAN_YAW,
		       		BNO055_REG_EUL_HEADING_LSB, 0, 0, 0),
	BNO055_CHANNEL(IIO_ROT, ROLL, BNO055_SCAN_ROLL,
		       		BNO055_REG_EUL_ROLL_LSB, 0, 0, 0),
	BNO055_CHANNEL(IIO_ROT, PITCH, BNO055_SCAN_PITCH,
		       		BNO055_REG_EUL_PITCH_LSB, 0, 0, 0),
	/* ================= quaternion ================= */
	BNO055_CHANNEL(IIO_ROT, QUATERNION, BNO055_SCAN_QUATERNION,
		       		BNO055_REG_QUA_DATA_W_LSB, 0, 0, 0),	
	/* ================= linear acceleration ================= */
	BNO055_CHANNEL(IIO_ACCEL, LINEAR_X, BNO055_SCAN_LIA_X,
		      	 	BNO055_REG_LIA_DATA_X_LSB, 0, 0, 0),
	BNO055_CHANNEL(IIO_ACCEL, LINEAR_Y, BNO055_SCAN_LIA_X,
		       		BNO055_REG_LIA_DATA_Y_LSB, 0, 0, 0),
	BNO055_CHANNEL(IIO_ACCEL, LINEAR_Z, BNO055_SCAN_LIA_X,
		       		BNO055_REG_LIA_DATA_Z_LSB, 0, 0, 0),
	/* ================= gravity vector =================*/
	BNO055_CHANNEL(IIO_GRAVITY,X, BNO055_SCAN_GRAVITY_X,
		      	 	BNO055_REG_GRV_DATA_X_LSB, 0, 0, 0),
	BNO055_CHANNEL(IIO_GRAVITY,Y, BNO055_SCAN_GRAVITY_Y,
		       		BNO055_REG_GRV_DATA_Y_LSB, 0, 0, 0),
	BNO055_CHANNEL(IIO_GRAVITY,Z, BNO055_SCAN_GRAVITY_Z,
		       		BNO055_REG_GRV_DATA_Z_LSB, 0, 0, 0),
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
		.scan_index = -1,
	},				
	IIO_CHAN_SOFT_TIMESTAMP(BNO055_SCAN_TIMESTAMP),
};

#define BNO055_NUM_CHANNELS ARRAY_SIZE(bno055_channels)

/* ================= READ RAW DATA ================= */

static int bno055_read_simple_chan(struct iio_dev *indio_dev,
				   struct iio_chan_spec const *chan,
				   int *val, int *val2, long mask)
{
	struct bno055_priv *priv = iio_priv(indio_dev);
	__le16 raw_val;
	int ret;
	switch (mask) {
		case IIO_CHAN_INFO_RAW:
			ret = regmap_bulk_read(priv->regmap, chan->address, &raw_val, sizeof(raw_val));
			//raw_val = [MSB][LSB] (little endian)
			//le16_to_cpu(raw_val) means: convert from little-endian to CPU endian
			if (ret < 0) return ret;
			*val = sign_extend32(le16_to_cpu(raw_val), 15);
			return IIO_VAL_INT;
		case IIO_CHAN_INFO_OFFSET:
			if (priv->opr_mode != BNO055_OPR_MODE_AMG) {
			*val = 0;
			} 
			else{
				ret = regmap_bulk_read(priv->regmap,chan->address+BNO055_REG_OFFSET_ADDR,&raw_val, sizeof(raw_val));
				if (ret < 0) return ret;
				*val = -sign_extend32(le16_to_cpu(raw_val), 15);
			}
			return IIO_VAL_INT;
		case IIO_CHAN_INFO_SCALE:
			*val = 1;
			switch (chan->type) {
				case IIO_GRAVITY:
				/* Table 3-35: 1 m/s^2 = 100 LSB */
				/*			   1 mg    = 1 LSB*/
				case IIO_ACCEL:
				/* Table 3-17: 1 m/s^2 = 100 LSB */
				/*			   1 mg    = 1 LSB*/
				*val2 = priv->scale.accel;
					break;
				case IIO_MAGN:
				/*
				* Table 3-19: 1 uT = 16 LSB.  But we need
				* Gauss: 1 µT = 0.01 G.
				*/
				// 1 LSB  = (1 / 16) µT
      			// 	   = (1 / 16) * 0.01 G
       			// 	   = 1 / 1600 G
				//*val2 = 1600;   //output unit Gauss
				*val2 = priv->scale.mag;
					break;
				case IIO_ANGL_VEL:
					/*
					* Table 3-22: 1 Rps = 900 LSB
					* .. but this is not exactly true. See comment at the
					* beginning of this file.
					*/
					if(priv->opr_mode != BNO055_OPR_MODE_AMG){
						*val = bno055_gyr_scale.fusion_vals[0];
						*val2 = bno055_gyr_scale.fusion_vals[1];
						return IIO_VAL_FRACTIONAL;
					}
					// *val2 = priv->scale.gyro;
					return bno055_get_regmask(priv,val,val2,BNO055_PG1(BNO055_REG_GYR_CONFIG_0),BNO055_GYR_CONFIG_RANGE_MASK,&bno055_gyr_scale);
					break;	
				case IIO_ROT:
					/* Table 3-28: 1 degree = 16 LSB */
					*val2 = 16;
					break;
				default:
					return -EINVAL;
			}
			return IIO_VAL_FRACTIONAL;
		case IIO_CHAN_INFO_SAMP_FREQ:
			if (chan->type != IIO_MAGN)
				return -EINVAL;
			return bno055_get_regmask(priv, val, val2,
						BNO055_PG1(BNO055_REG_MAG_CONFIG),
						BNO055_MAG_CONFIG_ODR_MASK,
						&bno055_mag_odr);
		case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
			switch (chan->type) {
			case IIO_ANGL_VEL:
				return bno055_get_regmask(priv, val, val2,
						  BNO055_PG1(BNO055_REG_GYR_CONFIG_0),
						  BNO055_GYR_CONFIG_LPF_BW_MASK,
						  &bno055_gyr_lpf_bw); 
			case IIO_ACCEL:
				return bno055_get_regmask(priv, val, val2,
						  BNO055_PG1(BNO055_REG_ACC_CONFIG),
						  BNO055_ACC_CONFIG_LPF_BW_MASK,
						  &bno055_acc_lpf_bw); 
			default:
				return -EINVAL;
			}
		default:
			return -EINVAL;
	}
}

static bool bno055_is_chan_readable(struct iio_dev *indio_dev,
				    struct iio_chan_spec const *chan)
{
	struct bno055_priv *priv = iio_priv(indio_dev);

	if (priv->opr_mode != BNO055_OPR_MODE_AMG)
		return true;
	/*If in AMG MODE block these read*/
	switch (chan->type) {
		//block read from IIO_GRAVITY and Quaternion
	case IIO_GRAVITY:
	case IIO_ROT:
		return false;
	case IIO_ACCEL:
		if (chan->channel2 == IIO_MOD_LINEAR_X ||
		    chan->channel2 == IIO_MOD_LINEAR_Y ||
		    chan->channel2 == IIO_MOD_LINEAR_Z)
			return false;
		return true;
	default:
		return true;
	}
}


static int bno055_read_temp_chan(struct iio_dev *indio_dev, int *val)
{
	struct bno055_priv *priv = iio_priv(indio_dev);
	unsigned int raw_val;
	int ret;

	ret = regmap_read(priv->regmap, BNO055_REG_TEMP, &raw_val);
	if (ret < 0)
		return ret;

	/*
	 * Tables 3-36 and 3-37: one byte of priv, signed, 1 LSB = 1C.
	 * ABI wants milliC.
	 */
	*val = raw_val * 1000; 

	return IIO_VAL_INT;
}

static int bno055_read_quaternion(struct iio_dev *indio_dev,
				  struct iio_chan_spec const *chan,
				  int size, int *vals, int *val_len,
				  long mask)
{
	struct bno055_priv *priv = iio_priv(indio_dev);
	__le16 raw_vals[4];
	int i, ret;
	switch (mask) {
		case IIO_CHAN_INFO_RAW:
			if (size < 4) return -EINVAL;
			ret = regmap_bulk_read(priv->regmap,BNO055_REG_QUA_DATA_W_LSB,
				       		raw_vals, sizeof(raw_vals));
			if (ret < 0) return ret;
			for (i = 0; i < 4; i++){
				vals[i] = sign_extend32(le16_to_cpu(raw_vals[i]), 15);
			}
			*val_len = 4;				
			return IIO_VAL_INT_MULTIPLE;
		case IIO_CHAN_INFO_SCALE:
		/* Table 3-31: 1 quaternion = 2^14 LSB */
			if (size < 2) return -EINVAL;
			vals[0] = 1;
			vals[1] = 14;
			return IIO_VAL_FRACTIONAL_LOG2;
		default:
			return -EINVAL;
	}

}
static int _bno055_read_raw_multi(struct iio_dev *indio_dev,
				  struct iio_chan_spec const *chan,
				  int size, int *vals, int *val_len,
				  long mask)
{
	if(!bno055_is_chan_readable(indio_dev,chan)) return -EBUSY;
	switch (chan->type) {
		case IIO_MAGN:
		case IIO_ACCEL:
		case IIO_ANGL_VEL:
		case IIO_GRAVITY:	
			if (size < 2)
				return -EINVAL;
			*val_len = 2;
			return bno055_read_simple_chan(indio_dev, chan, &vals[0], &vals[1],mask);
		case IIO_TEMP:
			*val_len = 1;
			return bno055_read_temp_chan(indio_dev, &vals[0]);		
		case IIO_ROT:
			/*
			* Rotation is exposed as either a quaternion or three
			* Euler angles.
			*/
			if (chan->channel2 == IIO_MOD_QUATERNION) 
				return bno055_read_quaternion(indio_dev, chan,
							size, vals,
							val_len, mask);
			if(size < 2) return -EINVAL;
			*val_len = 2;
				return bno055_read_simple_chan(indio_dev, chan,
							&vals[0], &vals[1],
							mask);
		default:
		return -EINVAL;	
	}
	return 0;
}

static int bno055_read_raw_multi(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan,
				 int size, int *vals, int *val_len,
				 long mask)
{
	struct bno055_priv *priv = iio_priv(indio_dev);
	int ret;

	mutex_lock(&priv->lock);
	ret = _bno055_read_raw_multi(indio_dev, chan, size,
				     vals, val_len, mask);
	mutex_unlock(&priv->lock);
	return ret;
}

static int bno055_sysfs_attr_avail(struct bno055_priv *priv, struct bno055_sysfs_attr *attr,
				   const int **vals, int *length)
{
	if (priv->opr_mode != BNO055_OPR_MODE_AMG) {
		/* locked when fusion enabled */
		*vals = attr->fusion_vals;
		if (attr->type == IIO_VAL_INT)
			*length = 1;
		else
			*length = 2; /* IIO_VAL_INT_PLUS_MICRO or IIO_VAL_FRACTIONAL*/
	} else {
		*vals = attr->vals;
		*length = attr->len;
	}

	return attr->type;
}

static int bno055_read_avail(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     const int **vals, int *type, int *length,
			     long mask)
{
	struct bno055_priv *priv = iio_priv(indio_dev);
	switch (mask) {
		case IIO_CHAN_INFO_SCALE:
			switch (chan->type) {
			case IIO_ANGL_VEL:
				*type = bno055_sysfs_attr_avail(priv, &bno055_gyr_scale,vals, length);
				return IIO_AVAIL_LIST;
			default:
				return -EINVAL;	
			}
		case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
			switch (chan->type) {
				case IIO_ANGL_VEL:
					*type = bno055_sysfs_attr_avail(priv, &bno055_gyr_lpf_bw,vals, length);
					return IIO_AVAIL_LIST;
				case IIO_ACCEL:
					*type = bno055_sysfs_attr_avail(priv, &bno055_acc_lpf_bw,vals, length);
					return IIO_AVAIL_LIST;
				default:
					return -EINVAL;
			}
			break;
		case IIO_CHAN_INFO_SAMP_FREQ:
		switch (chan->type) {
			case IIO_MAGN:
				*type = bno055_sysfs_attr_avail(priv, &bno055_mag_odr,vals, length);
				return IIO_AVAIL_LIST;
			default:
				return -EINVAL;
		}
		default:
			return -EINVAL;
	}
}

static int bno055_set_regmask(struct bno055_priv *priv, int val, int val2,
			      int reg, int mask, struct bno055_sysfs_attr *attr)
{
	const int shift = __ffs(mask);
	int best_delta;
	int req_val;
	int tbl_val;
	bool first;
	int delta;
	int hwval;
	int ret;
	int len;
	int i;

	/*
	 * The closest value the HW supports is only one in fusion mode,
	 * and it is autoselected, so don't do anything, just return OK,
	 * as the closest possible value has been (virtually) selected
	 */
	if (priv->opr_mode != BNO055_OPR_MODE_AMG)
		return 0;

	len = attr->len;

	/*
	 * We always get a request in INT_PLUS_MICRO, but we
	 * take care of the micro part only when we really have
	 * non-integer tables. This prevents 32-bit overflow with
	 * larger integers contained in integer tables.
	 */
	req_val = val;
	if (attr->type != IIO_VAL_INT) {
		len /= 2;
		req_val = min(val, 2147) * 1000000 + val2;
	}

	first = true;
	for (i = 0; i < len; i++) {
		switch (attr->type) {
		case IIO_VAL_INT:
			tbl_val = attr->vals[i];
			break;
		case IIO_VAL_INT_PLUS_MICRO:
			WARN_ON(attr->vals[i * 2] > 2147);
			tbl_val = attr->vals[i * 2] * 1000000 +
				attr->vals[i * 2 + 1];
			break;
		case IIO_VAL_FRACTIONAL:
			WARN_ON(attr->vals[i * 2] > 4294);
			tbl_val = attr->vals[i * 2] * 1000000 /
				attr->vals[i * 2 + 1];
			break;
		default:
			return -EINVAL;
		}
		delta = abs(tbl_val - req_val);
		if (first || delta < best_delta) {
			best_delta = delta;
			hwval = i;
			first = false;
		}
	}

	if (attr->hw_xlate)
		hwval = attr->hw_xlate[hwval];

	ret = bno055_set_opr_mode(priv, BNO055_OPR_MODE_CONFIG);
	if (ret)
		return ret;

	ret = regmap_update_bits(priv->regmap, reg, mask, hwval << shift);
	if (ret)
		return ret;

	return bno055_set_opr_mode(priv, BNO055_OPR_MODE_AMG);
}

static int _bno055_write_raw(struct iio_dev *iio_dev,
			     struct iio_chan_spec const *chan,
			     int val, int val2, long mask)
{
	struct bno055_priv *priv = iio_priv(iio_dev);
	switch (chan->type) {
	case IIO_MAGN:
		switch (mask) {
			case IIO_CHAN_INFO_SAMP_FREQ:
				return bno055_set_regmask(priv, val, val2,
						  BNO055_PG1(BNO055_REG_MAG_CONFIG),
						  BNO055_MAG_CONFIG_ODR_MASK,
						  &bno055_mag_odr);
			default:
				return -EINVAL;
		}
	case IIO_ACCEL:
		switch (mask) {
			case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
				return bno055_set_regmask(priv, val, val2,
						  BNO055_PG1(BNO055_REG_ACC_CONFIG),
						  BNO055_ACC_CONFIG_LPF_BW_MASK,
						  &bno055_acc_lpf_bw);
			default:
				return -EINVAL;
		}
	case IIO_ANGL_VEL:
		switch (mask) {
			case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
				return bno055_set_regmask(priv, val, val2,
							BNO055_PG1(BNO055_REG_GYR_CONFIG_0),
							BNO055_GYR_CONFIG_LPF_BW_MASK,
							&bno055_gyr_lpf_bw);
			case IIO_CHAN_INFO_SCALE:
				return bno055_set_regmask(priv, val, val2,
						  	BNO055_PG1(BNO055_REG_GYR_CONFIG_0),
						  	BNO055_GYR_CONFIG_RANGE_MASK,
						  	&bno055_gyr_scale);
			default:
				return -EINVAL;
		}
	default:
		return -EINVAL;
	}
}

static int bno055_write_raw(struct iio_dev *iio_dev,
			    struct iio_chan_spec const *chan,
			    int val, int val2, long mask)
{
	struct bno055_priv *priv = iio_priv(iio_dev);
	int ret;

	mutex_lock(&priv->lock);
	ret = _bno055_write_raw(iio_dev, chan, val, val2, mask);
	mutex_unlock(&priv->lock);

	return ret;
}

/*binary sysfs*/ 
static const char *bno055_mode_str[] = {
	"CONFIG",
	"ACCONLY",
	"MAGONLY",
	"GYRONLY",
	"ACCMAG",
	"ACCGYRO",
	"MAGGYRO",
	"AMG",
	"IMU",
	"COMPASS",
	"M4G",
	"NDOF_FMC_OFF",
	"NDOF",
};

static ssize_t operation_mode_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct bno055_priv *priv = iio_priv(dev_to_iio_dev(dev));

	if (priv->opr_mode < ARRAY_SIZE(bno055_mode_str))
		return sysfs_emit(buf, "%d %s\n",
				 priv->opr_mode,
				 bno055_mode_str[priv->opr_mode]);

	return sysfs_emit(buf, "UNKNOWN\n");
}

static ssize_t operation_mode_store(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct bno055_priv *priv = iio_priv(indio_dev);

}


static IIO_DEVICE_ATTR_RW(operation_mode, 0);
static IIO_DEVICE_ATTR_RW(in_magn_calibration_fast_enable, 0);
static IIO_DEVICE_ATTR_RW(in_accel_range_raw, 0);

static IIO_DEVICE_ATTR_RO(in_accel_range_raw_available, 0);
static IIO_DEVICE_ATTR_RO(sys_calibration_auto_status, 0);
static IIO_DEVICE_ATTR_RO(in_accel_calibration_auto_status, 0);
static IIO_DEVICE_ATTR_RO(in_gyro_calibration_auto_status, 0);
static IIO_DEVICE_ATTR_RO(in_magn_calibration_auto_status, 0);
static IIO_DEVICE_ATTR_RO(serialnumber, 0);

static struct attribute *bno055_attrs[] = {
	&iio_dev_attr_in_accel_range_raw_available.dev_attr.attr,
	&iio_dev_attr_in_accel_range_raw.dev_attr.attr,
	&iio_dev_attr_operation_mode.dev_attr.attr, //ok
	&iio_dev_attr_in_magn_calibration_fast_enable.dev_attr.attr,
	&iio_dev_attr_sys_calibration_auto_status.dev_attr.attr,
	&iio_dev_attr_in_accel_calibration_auto_status.dev_attr.attr,
	&iio_dev_attr_in_gyro_calibration_auto_status.dev_attr.attr,
	&iio_dev_attr_in_magn_calibration_auto_status.dev_attr.attr,
	&iio_dev_attr_serialnumber.dev_attr.attr,
	NULL
};

static BIN_ATTR_RO(calibration_data, BNO055_CALDATA_LEN);

static struct bin_attribute *bno055_bin_attrs[] = {
	&bin_attr_calibration_data,
	NULL
};

static const struct attribute_group bno055_attrs_group = {
	.attrs = bno055_attrs,
	.bin_attrs = bno055_bin_attrs,
};

static const struct iio_info bno055dev_info = {
	.read_raw_multi = bno055_read_raw_multi,
	.read_avail = bno055_read_avail,
	.write_raw  = bno055_write_raw,
	.attrs = &bno055_attrs_group,
};



/* ================= END IIO INFO ================= */

/*SET Page ID*/
static int bno055_set_page_id(struct bno055_priv *priv,enum bno055_page_id tar_page_id)
{
	int ret;
	unsigned int pageid;
	ret = regmap_read(priv->regmap, BNO055_PAGESEL_REG, &pageid);
	if (ret) {
		//dev_err(st->dev, "Failed to read page id\n");
		goto out;
	}

	if (pageid != tar_page_id) {

		ret = regmap_write(priv->regmap, BNO055_PAGESEL_REG,
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
static int bno055_get_chip_id(struct bno055_priv *priv)
{
	struct device *dev = priv->dev;   // ✅ FIX
	int ret;

	bno055_set_page_id(priv, PAGE_ID_0);

	ret = regmap_read(priv->regmap, BNO055_REG_SW_REV_ID_LSB,
			  &priv->id.SW_REV_ID_LSB);
	if (ret)
		return ret;

	ret = regmap_read(priv->regmap, BNO055_REG_SW_REV_ID_MSB,
			  &priv->id.SW_REV_ID_MSB);
	if (ret)
		return ret;

	if (priv->id.SW_REV_ID_MSB != 0x3 ||
	    priv->id.SW_REV_ID_LSB != 0x11)
		dev_warn(dev,
			 "Untested firmware version. Anglvel scale may not work as expected\n");

	ret = regmap_read(priv->regmap, BNO055_REG_ACC_ID, &priv->id.ACC_ID);
	if (ret)
		return ret;

	ret = regmap_read(priv->regmap, BNO055_REG_GYR_ID, &priv->id.GYR_ID);
	if (ret)
		return ret;

	ret = regmap_read(priv->regmap, BNO055_REG_MAG_ID, &priv->id.MAG_ID);
	if (ret)
		return ret;

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

	return 0;
}



/* ================= SET MODE ================= */
static int bno055_set_opr_mode(struct bno055_priv *priv, enum bno055_opr_mode opr_mode){
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
// static ssize_t bno055_mode_show(struct device *dev,
// 				struct device_attribute *attr,
// 				char *buf)
// {
// 	struct bno055_priv *priv = dev_get_drvdata(dev);
// 	int i;

// 	for (i = 0; i < ARRAY_SIZE(bno055_modes); i++)
// 		if (bno055_modes[i].val == priv->opr_mode)
// 			return sprintf(buf, "%s\n", bno055_modes[i].name);

// 	return sprintf(buf, "UNKNOWN\n");
// }


// static ssize_t bno055_mode_store(struct device *dev,	
// 				 struct device_attribute *attr,
// 				 const char *buf, size_t count)
// {
// 	struct bno055_priv *priv = dev_get_drvdata(dev);
// 	int i, ret;

// 	mutex_lock(&priv->lock);

// 	for (i = 0; i < ARRAY_SIZE(bno055_modes); i++) {
// 		if (sysfs_streq(buf, bno055_modes[i].name)) {
// 			ret = bno055_set_opr_mode(st,
// 						 bno055_modes[i].val);
// 			mutex_unlock(&st->lock);
// 			return ret ? ret : count;
// 		}
// 	}

// 	mutex_unlock(&st->lock);
// 	return -EINVAL;
// }

// static DEVICE_ATTR_RW(bno055_mode);


static int bno055_system_reset(struct bno055_priv *priv){
	int ret;
	priv->id.page_id = PAGE_ID_0;
	// ret = regmap_write(priv->regmap, BNO055_PAGESEL_REG,
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
	//bno055_set_page_id(priv,PAGE_ID_0);
	int tmp;
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
	//bno055_set_page_id(priv,PAGE_ID_0);
	int tmp;
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
	//bno055_set_page_id(priv,PAGE_ID_0);
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
	//bno055_set_page_id(priv,PAGE_ID_1);
	int tmp;
	priv->acc_gyr_mag_valuation.acc_g_range = g_range;
	priv->acc_gyr_mag_valuation.acc_bandwidth = Bandwidth;
	priv->acc_gyr_mag_valuation.acc_mode = OPRMode;
	tmp = (priv->acc_gyr_mag_valuation.acc_mode | priv->acc_gyr_mag_valuation.acc_bandwidth)| priv->acc_gyr_mag_valuation.acc_g_range;
	ret = regmap_write(priv->regmap, BNO055_PG1(BNO055_REG_ACC_CONFIG),tmp);
	return ret;
}	

/*gyroscope configuration*/
static int bno_gyr_config(struct bno055_priv *priv,int g_range,int Bandwidth,int OPRMode ){
	int ret;
	//bno055_set_page_id(priv,PAGE_ID_1);
	priv->acc_gyr_mag_valuation.gyr_range = g_range;
	priv->acc_gyr_mag_valuation.gyr_bandwidth = Bandwidth;
	priv->acc_gyr_mag_valuation.gyr_mode = OPRMode;
	int tmp_cf0, tmp_cf1;
	tmp_cf0 =  priv->acc_gyr_mag_valuation.gyr_bandwidth|priv->acc_gyr_mag_valuation.gyr_range;
	ret = regmap_write(priv->regmap, BNO055_PG1(BNO055_REG_GYR_CONFIG_0),tmp_cf0);
	tmp_cf1 = priv->acc_gyr_mag_valuation.gyr_mode;
	ret = regmap_write(priv->regmap, BNO055_PG1(BNO055_REG_GYR_CONFIG_1),tmp_cf1);
	return ret;
}

static int bno_mag_config(struct bno055_priv *priv,int data_rate,int OPRMode,int PWRMode){
	int ret;
	int tmp;
	//bno055_set_page_id(priv,PAGE_ID_1);
	priv->acc_gyr_mag_valuation.mag_data_rate = data_rate;
	priv->acc_gyr_mag_valuation.mag_operation_mode = OPRMode;
	priv->acc_gyr_mag_valuation.mag_pwr_mode = PWRMode;
	tmp =  (priv->acc_gyr_mag_valuation.mag_pwr_mode|priv->acc_gyr_mag_valuation.mag_operation_mode) | priv->acc_gyr_mag_valuation.mag_data_rate;
	ret = regmap_write(priv->regmap, BNO055_PG1(BNO055_REG_MAG_CONFIG),tmp);
	return ret;
}

static int bno_set_unit(struct bno055_priv *priv,int acc,int angular,int euler, int temp, int  fusion_dof){
	int ret;
	int tmp;
	//bno055_set_page_id(priv,PAGE_ID_1);
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
	priv->scale.accel = (acc == BNO055_UNIT_ACC_MS2) ? 100 : 1;
	priv->scale.gyro = (angular == BNO055_UNIT_GYR_DPS) ? 16 : 900;
	priv->scale.mag = 16;
	priv->scale.euler = (euler == BNO055_UNIT_EUL_DEG) ? 16 : 900;
	priv->scale.qua = 16384;
	priv->scale.temp = (temp == BNO055_UNIT_TEMP_C) ? 1 : 2;
	return ret;
}

static int bno_set_temperature_src(struct bno055_priv *priv,enum bno055_temp_source temp_source){
	int tmp,ret;
	//bno055_set_page_id(priv,PAGE_ID_1);
	if(temp_source == BNO055_TEMP_SRC_ACCEL){
		priv->temp_source = BNO055_TEMP_SRC_ACCEL;
		tmp = BNO055_TEMP_SRC_ACCEL;
	}
	else if(temp_source == BNO055_TEMP_SRC_GYRO){
		priv->temp_source = BNO055_TEMP_SRC_GYRO;
		tmp = BNO055_TEMP_SRC_GYRO;
	}
	ret = regmap_write(priv->regmap, BNO055_REG_TEMP_SOURCE,tmp);
	return ret;
}

static int bno055_init(struct bno055_priv *priv){
	int ret;
	/*Set Axis Remap Config*/
	ret = bno_axis_remap_config(priv,REMAP_CONFIG_P1_2_4_7);
	if(ret) dev_err(priv->dev, "Failed to axis remap configuration\n");
	/*Set Axis Remap Sign*/
	ret = bno_axis_remap_sign(priv,BNO055_AXIS_SIGN_P1);
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
	ret = bno_set_unit(priv,BNO055_UNIT_ACC_MS2,BNO055_UNIT_GYR_DPS,BNO055_UNIT_EUL_DEG,BNO055_UNIT_TEMP_C,BNO055_UNIT_ANDROID_FORMAT);
	if(ret) dev_err(priv->dev, "Failed to Set Unit\n");
	/*Set Temperature Source*/
	ret = bno_set_temperature_src(priv,BNO055_TEMP_SRC_ACCEL);
	if(ret) dev_err(priv->dev, "Failed to Set Temperature Source\n");
	/*Move to page 0 for read*/
	bno055_set_page_id(priv,PAGE_ID_0);
	/*Delay*/
	msleep(100);
	/*Set Operation Mode*/
	ret = bno055_set_opr_mode(priv,BNO055_OPR_MODE_NDOF);
	msleep(50);
	return ret;
}



int bno055_probe(struct device *dev, struct regmap *regmap)
{
	struct bno055_priv *priv;
	struct iio_dev *iio_dev;
	int ret;
	unsigned int val;

	iio_dev = devm_iio_device_alloc(dev, sizeof(*priv));
	if (!iio_dev)
		return -ENOMEM;

	iio_dev->name = DRIVER_NAME;
	iio_dev->dev.parent = dev;

	priv = iio_priv(iio_dev);
	mutex_init(&priv->lock);

	priv->regmap = regmap;
	priv->dev = dev;

	ret = regmap_read(priv->regmap, BNO055_REG_CHIP_ID, &val);
	if (ret)
		return ret;

	priv->id.CHIP_ID = val;

	if (priv->id.CHIP_ID != BNO055_CHIP_ID) {
		dev_warn(dev, "Unrecognized Chip ID 0x%x\n", priv->id.CHIP_ID);
		return -ENODEV;
	}

    msleep(50);

	dev_info(dev, "BNO055 Detected\n");

	ret = bno055_system_reset(priv);
	if (ret)
		return ret;

	ret = bno055_get_chip_id(priv);
	if (ret)
		return ret;

	ret = bno055_init(priv);
	if (ret) {
		dev_err(dev, "Failed to Init\n");
		return ret;
	}

	iio_dev->channels = bno055_channels;
	iio_dev->num_channels = BNO055_NUM_CHANNELS;
	iio_dev->info = &bno055dev_info;
	iio_dev->modes = INDIO_DIRECT_MODE;

	ret = devm_iio_device_register(dev, iio_dev);
	if (ret)
		return ret;

	dev_info(dev, "BNO055 ready\n");

	return 0;
}

// void bno055_remove(struct device *dev, struct regmap *regmap)
// {
	
// }

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);  
MODULE_VERSION(DRIVER_VERS);