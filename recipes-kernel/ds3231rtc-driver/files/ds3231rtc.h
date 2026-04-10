#ifndef _DS3231RTC_H_
#define _DS3231RTC_H_

#define DS3231RTC_I2C_ADDR   0x68

#define DS3231RTC_REG_SECONDS           0X00
#define DS3231RTC_REG_MINUTES           0X01
#define DS3231RTC_REG_HOURS             0X02
#define DS3231RTC_REG_DAY               0X03
#define DS3231RTC_REG_DATE              0X04
#define DS3231RTC_REG_MONTH_CENTURY     0x05
#define DS3231RTC_REG_YEARS             0x06

#define DS3231RTC_REG_ALARM1_SECONDS    0x07
#define DS3231RTC_REG_ALARM1_MINUTES    0x08
#define DS3231RTC_REG_ALARM1_HOURS      0x09
#define DS3231RTC_REG_ALARM1_DAY_DATE   0x0A

#define DS3231RTC_REG_ALARM2_MINUTES    0x0B
#define DS3231RTC_REG_ALARM2_HOURS      0x0C
#define DS3231RTC_REG_ALARM2_DAY_DATE   0x0D

#define DS3231RTC_REG_CONTROL           0x0E
#define DS3231RTC_REG_CONTROL_STATUS    0x0F
#define DS3231RTC_REG_AGING_OFFSET      0x10
#define DS3231RTC_REG_MSB_TEMP          0x11
#define DS3231RTC_REG_LSB_TEMP          0x12

struct ds3231rtc_priv{
    struct regmap *regmap;
	struct device *dev;
	struct mutex lock;
};

int ds3231rtc_probe(struct device *dev, struct regmap *regmap);

#endif /* _DS3231RTC_H_ */



