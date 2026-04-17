#ifndef _BMP280_H_
#define _BMP280_H_

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>

#include <linux/iio/iio.h>


#define BMP280_I2C_ADDR				    0x76
#define BMP280_REG_ID                   0xD0
#define BMP280_CHIP_ID			        0x58

#define BMP280_REG_CALIBRATION_START        0x88
#define BMP280_REG_CALIBRATION_END        0xA1  

#define BMP280_REG_PRESS_MSB                0xF7
#define BMP280_REG_PRESS_LSB                0xF8
#define BMP280_REG_PRESS_XLSB               0xF9
#define BMP280_REG_TEMP_MSB                 0xFA
#define BMP280_REG_TEMP_LSB                 0xFB
#define BMP280_REG_TEMP_XLSB                0xFC

/* Helper mask to truncate excess 4 bits on pressure and temp readings */
#define BMP280_MEAS_TRIM_MASK		    GENMASK(24, 4)

#define BMP280_REG_RESET                    0xE0
#define BMP280_SYS_RESET_VALUE              0x68
#define BMP280_REG_STATUS                   0xF3
#define BMP280_STATUS_IM_UPDATE   			BIT(0)
#define BMP280_STATUS_MEASURING   			BIT(3)
#define BMP280_REG_CTRL_MEAS                0xF4
#define BMP280_REG_CONFIG                   0xF5


#define BMP280_REG_COMP_TEMP_START	    0x88
#define BMP280_COMP_TEMP_REG_COUNT	    6

#define BMP280_REG_COMP_PRESS_START	    0x8E
#define BMP280_COMP_PRESS_REG_COUNT	    18

#define BMP280_CONTIGUOUS_CALIB_REGS	(BMP280_COMP_TEMP_REG_COUNT + \
					 BMP280_COMP_PRESS_REG_COUNT)

                     
#define BMP280_FILTER_MASK		        GENMASK(4, 2)
// #define BMP280_FILTER_OFF		        0
// #define BMP280_FILTER_2X		        1
// #define BMP280_FILTER_4X		        2
// #define BMP280_FILTER_8X		        3
// #define BMP280_FILTER_16X		        4

#define BMP280_OSRS_TEMP_MASK		    GENMASK(7, 5)
#define BMP280_OSRS_TEMP_SKIP		    0x00
#define BMP280_OSRS_TEMP_1X		        0x01
#define BMP280_OSRS_TEMP_2X		        0x02
#define BMP280_OSRS_TEMP_4X		        0x03
#define BMP280_OSRS_TEMP_8X		        0x04
#define BMP280_OSRS_TEMP_16X		    0x05

#define BMP280_OSRS_PRESS_MASK		    GENMASK(4, 2)
#define BMP280_OSRS_PRESS_SKIP		    0x00
#define BMP280_OSRS_PRESS_1X		    0x01
#define BMP280_OSRS_PRESS_2X		    0x02
#define BMP280_OSRS_PRESS_4X		    0x03
#define BMP280_OSRS_PRESS_8X		    0x04
#define BMP280_OSRS_PRESS_16X		    0x05

#define BMP280_MODE_MASK		        GENMASK(1, 0)
#define BMP280_MODE_SLEEP		        0
#define BMP280_MODE_FORCED		        1
#define BMP280_MODE_NORMAL		        3

/* BMP280 register skipped special values */
#define BMP280_TEMP_SKIPPED			0x80000
#define BMP280_PRESS_SKIPPED		0x80000

/* Number of bytes for each value */
#define BMP280_NUM_PRESS_BYTES		3
#define BMP280_NUM_TEMP_BYTES		3
#define BMP280_BURST_READ_BYTES		(BMP280_NUM_PRESS_BYTES + \
					 BMP280_NUM_TEMP_BYTES)

#define BMP280_STANDBY_MASK      GENMASK(7, 5)
#define BMP280_FILTER_MASK       GENMASK(4, 2)
#define BMP280_SPI3W_MASK        BIT(0)

/* See datasheet Section 4.2.2. */
struct bmp280_calib {
	u16 T1;
	s16 T2;
	s16 T3;
	u16 P1;
	s16 P2;
	s16 P3;
	s16 P4;
	s16 P5;
	s16 P6;
	s16 P7;
	s16 P8;
	s16 P9;
	__le16 bmp280_cal_buf[BMP280_CONTIGUOUS_CALIB_REGS / 2];
};


struct bmp280_chip_info{
	unsigned int start_up_time; /* in microseconds */
	/* log of base 2 of oversampling rate */
	const unsigned long *avail_scan_masks;

	const int *oversampling_temp_avail;
	int num_oversampling_temp_avail;
	int oversampling_temp_default;

	const int *oversampling_press_avail;
	int num_oversampling_press_avail;
	int oversampling_press_default;

	const int *temp_coeffs;
	const int temp_coeffs_type;
	const int *press_coeffs;
	const int press_coeffs_type;
};

enum bmp280_opr_mode{
	SLEEP_MODE =  0x00,
	FORCE_MODE =  0x01,
	NORMAL_MODE = 0x03,
};

enum bmp280_t_sb_standby{
	BMP280_TSB_0_5  = 0x00,
	BMP280_TSB_62_5 = 0x01,
	BMP280_TSB_125  = 0x02,
	BMP280_TSB_250  = 0x03,
	BMP280_TSB_500  = 0x04,
	BMP280_TSB_1000 = 0x05,
	BMP280_TSB_2000 = 0x06,
	BMP280_TSB_4000 = 0x07,
};

/*Table 6: filter settings*/
enum bmp280_filter_iir{
	BMP280_FILTER_OFF			= 0x00,
	BMP280_FILTER_2X			= 0x01,
	BMP280_FILTER_4X			= 0x02,
	BMP280_FILTER_8X			= 0x03,
	BMP280_FILTER_16X			= 0x04,
};

enum bmp280_osrs_type{
	OSRS_TEMP = 0,
	OSRS_PRESS = 1,
};

enum bmp280_use_case_normal{
	HANDHELD_DEVICE_LOW_POWER,
	HANDHELD_DEVICEC_DYNAMIC,
	ELEVATOR_FLOOR_CHANGE_DETECTION,
	DROP_DETECTION,
	INDOOR_NAVIGATION,
};

struct bmp280_config{
	int t_sb_us;	//in us 
	int filter_delay_samples;
	u8 spi3w_en;
	enum bmp280_t_sb_standby enum_t_sb;
	enum bmp280_filter_iir enum_filter;
	int osrst;
	int osrsp;
	u8 ctrl_meas_osrsp;
	u8 ctrl_meas_osrst;
	u8 ctrl_meas_mode;
};




struct bmp280_priv{
    struct regmap *regmap;
	struct device *dev;
	struct mutex lock;
	unsigned int start_up_time; 
	u8 iir_filter_coeff;
	int chipid;
	struct bmp280_calib calib;
	const struct bmp280_chip_info *chip_info;
	enum bmp280_opr_mode mode;
	struct bmp280_config config;
	enum bmp280_use_case_normal ucase;

	union {
		/* Sensor data buffer */
		u8 buft[BMP280_NUM_TEMP_BYTES];
		u8 bufp[BMP280_NUM_PRESS_BYTES];
		__le16 le16;
		__be16 be16;
	} __aligned(IIO_DMA_MINALIGN);
};


int bmp280_probe(struct device *dev, struct regmap *regmap);

extern const struct regmap_config bmp280dev_regmap_config;

#endif //_BMP280_H_