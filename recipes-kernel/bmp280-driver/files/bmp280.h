#ifndef _BMP280_H_
#define _BMP280_H_

#define BMP280_I2C_ADDR				    0x76
#define BMP280_ID                       0xD0
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
#define BMP280_REG_STATUS                   0xF3
#define BMP280_REG_CTRL_MEAS                0xF4
#define BMP280_REG_CONFIG                   0xF5


#define BMP280_REG_COMP_TEMP_START	    0x88
#define BMP280_COMP_TEMP_REG_COUNT	    6

#define BMP280_REG_COMP_PRESS_START	    0x8E
#define BMP280_COMP_PRESS_REG_COUNT	    18

#define BMP280_CONTIGUOUS_CALIB_REGS	(BMP280_COMP_TEMP_REG_COUNT + \
					 BMP280_COMP_PRESS_REG_COUNT)

                     
#define BMP280_FILTER_MASK		        GENMASK(4, 2)
#define BMP280_FILTER_OFF		        0
#define BMP280_FILTER_2X		        1
#define BMP280_FILTER_4X		        2
#define BMP280_FILTER_8X		        3
#define BMP280_FILTER_16X		        4

#define BMP280_OSRS_TEMP_MASK		    GENMASK(7, 5)
#define BMP280_OSRS_TEMP_SKIP		    0
#define BMP280_OSRS_TEMP_1X		        1
#define BMP280_OSRS_TEMP_2X		        2
#define BMP280_OSRS_TEMP_4X		        3
#define BMP280_OSRS_TEMP_8X		        4
#define BMP280_OSRS_TEMP_16X		    5

#define BMP280_OSRS_PRESS_MASK		    GENMASK(4, 2)
#define BMP280_OSRS_PRESS_SKIP		    0
#define BMP280_OSRS_PRESS_1X		    1
#define BMP280_OSRS_PRESS_2X		    2
#define BMP280_OSRS_PRESS_4X		    3
#define BMP280_OSRS_PRESS_8X		    4
#define BMP280_OSRS_PRESS_16X		    5

#define BMP280_MODE_MASK		        GENMASK(1, 0)
#define BMP280_MODE_SLEEP		        0
#define BMP280_MODE_FORCED		        1
#define BMP280_MODE_NORMAL		        3

/* Number of bytes for each value */
#define BMP280_NUM_PRESS_BYTES		3
#define BMP280_NUM_TEMP_BYTES		3
#define BMP280_BURST_READ_BYTES		(BMP280_NUM_PRESS_BYTES + \
					 BMP280_NUM_TEMP_BYTES)

static const char *const bmp280_supply_names[] = {
	"vddd", "vdda"
};

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
	u8  H1;
	s16 H2;
	u8  H3;
	s16 H4;
	s16 H5;
	s8  H6;
};

#endif //_BMP280_H_