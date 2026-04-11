#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/mutex.h>
#include <linux/bcd.h>
#include "ds3231rtc.h"

#define DRIVER_NAME   "ds3231rtcdev"
#define DRIVER_AUTHOR "desmtiny nguyenhoangminh@gmail.com"
#define DRIVER_DESC   "RTC DS3231 Driver"
#define DRIVER_VERS   "1.0"

static const struct regmap_config ds3231_regmap_config = {
    .name = DRIVER_NAME,
    .reg_bits = 8,
    .val_bits = 8,
};

/* ================= READ TIME ================= */
static int ds3231rtc_read_time(struct ds3231rtc_priv *priv)
{
    unsigned int sec, min, hour, date, month, year;
    int ret;

    mutex_lock(&priv->lock);

    ret = regmap_read(priv->regmap, DS3231RTC_REG_SECONDS, &sec);
    if (ret) goto out;

    regmap_read(priv->regmap, DS3231RTC_REG_MINUTES, &min);
    regmap_read(priv->regmap, DS3231RTC_REG_HOURS, &hour);
    regmap_read(priv->regmap, DS3231RTC_REG_DATE, &date);
    regmap_read(priv->regmap, DS3231RTC_REG_MONTH_CENTURY, &month);
    regmap_read(priv->regmap, DS3231RTC_REG_YEARS, &year);

    /* BCD → binary */
    sec   = bcd2bin(sec);
    min   = bcd2bin(min);
    hour  = bcd2bin(hour);
    date  = bcd2bin(date);
    month = bcd2bin(month & 0x1F);
    year  = bcd2bin(year);

    dev_info(priv->dev,
        "Time: %02u:%02u:%02u %02u/%02u/20%02u\n",
        hour, min, sec, date, month, year);

out:
    mutex_unlock(&priv->lock);
    return ret;
}

/* ================= SET TIME ================= */
static int ds3231rtc_set_time(struct ds3231rtc_priv *priv,
                              int sec, int min, int hour,
                              int date, int month, int year)
{
    int ret;

    mutex_lock(&priv->lock);

    ret = regmap_write(priv->regmap, DS3231RTC_REG_SECONDS, bin2bcd(sec));
    if (ret) goto out;
    ret = regmap_write(priv->regmap, DS3231RTC_REG_MINUTES, bin2bcd(min));
    if(ret) goto out;
    regmap_write(priv->regmap, DS3231RTC_REG_HOURS, bin2bcd(hour));
    if(ret) goto out;
    regmap_write(priv->regmap, DS3231RTC_REG_DATE, bin2bcd(date));
    if(ret) goto out;
    regmap_write(priv->regmap, DS3231RTC_REG_MONTH_CENTURY, bin2bcd(month));
    if(ret) goto out;
    regmap_write(priv->regmap, DS3231RTC_REG_YEARS, bin2bcd(year));
    if(ret) goto out;
out:
    mutex_unlock(&priv->lock);
    return ret;
}

static ssize_t time_show(struct device *dev,
                         struct device_attribute *attr,
                         char *buf)
{
    struct ds3231rtc_priv *priv = dev_get_drvdata(dev);
    u8 data[7];
    int ret;
    int sec, min, hour, date, month, year, day;

    static const char *ds3231_day_str[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    /* Read 7 registers from 0x00 */
    ret = regmap_bulk_read(priv->regmap,
                           DS3231RTC_REG_SECONDS,
                           data, 7);
    if (ret)
        return ret;

    /* Decode */
    sec   = bcd2bin(data[0] & 0x7F);
    min   = bcd2bin(data[1] & 0x7F);
    hour  = bcd2bin(data[2] & 0x3F);
    day   = bcd2bin(data[3] & 0x07);
    date  = bcd2bin(data[4] & 0x3F);
    month = bcd2bin(data[5] & 0x1F);
    year  = bcd2bin(data[6]);

    /* Validate weekday */
    if (day < 1 || day > 7)
        return sysfs_emit(buf,
            "??? %02d:%02d:%02d %02d/%02d/20%02d\n",
            hour, min, sec, date, month, year);

    return sysfs_emit(buf,
                      "%s %02d:%02d:%02d %02d/%02d/20%02d\n",
                      ds3231_day_str[day - 1],
                      hour, min, sec,
                      date, month, year);
}


static int ds3231_calc_wday(int d, int m, int y)
{
    /* Zeller’s Congruence */
    if (m < 3) {
        m += 12;
        y--;
    }
    int k = y % 100;
    int j = y / 100;
    int h = (d + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;

    /* Convert to DS3231 format: 1=Sun ... 7=Sat */
    return ((h + 6) % 7) + 1;
}

static ssize_t time_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t len)
{
    struct ds3231rtc_priv *priv = dev_get_drvdata(dev);
    int sec, min, hour, date, month, year, day;
    u8 data[7];
    int ret;

    /* Parse input: HH:MM:SS DD/MM/YYYY */
    if (sscanf(buf, "%d:%d:%d %d/%d/%d",
               &hour, &min, &sec,
               &date, &month, &year) != 6)
        return -EINVAL;

    /* Sanity check */
    if (sec < 0 || sec > 59 ||
        min < 0 || min > 59 ||
        hour < 0 || hour > 23 ||
        date < 1 || date > 31 ||
        month < 1 || month > 12 ||
        year < 2000 || year > 2099)
        return -EINVAL;

    /* Calculate weekday automatically */
    day = ds3231_calc_wday(date, month, year);

    /* Convert to BCD + mask control bits */
    data[0] = bin2bcd(sec)  & 0x7F; /* seconds, CH=0 */
    data[1] = bin2bcd(min)  & 0x7F; /* minutes */
    data[2] = bin2bcd(hour) & 0x3F; /* hours, 24h */
    data[3] = bin2bcd(day)  & 0x07; /* weekday */
    data[4] = bin2bcd(date) & 0x3F; /* date */
    data[5] = bin2bcd(month)& 0x1F; /* month */
    data[6] = bin2bcd(year % 100);  /* year */

    /* Write atomically */
    ret = regmap_bulk_write(priv->regmap,
                            DS3231RTC_REG_SECONDS,
                            data, 7);
    if (ret)
        return ret;

    return len;
}

static DEVICE_ATTR_RW(time);

int ds3231rtc_probe(struct device *dev, struct regmap *regmap){
    struct ds3231rtc_priv *priv;
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;
    priv->dev = dev;
    priv->regmap = regmap;
    mutex_init(&priv->lock);
    dev_set_drvdata(dev, priv);
    dev_info(dev, "DS3231 RTC Ready\n");
    device_create_file(dev, &dev_attr_time);
    /*PrintTime*/
    ds3231rtc_read_time(priv);
    return 0;
}

EXPORT_SYMBOL_GPL(ds3231rtc_probe);

/* ================= I2C DRIVER ================= */

static int ds3231_i2c_probe(struct i2c_client *client)
{
    struct regmap *regmap;

    regmap = devm_regmap_init_i2c(client, &ds3231_regmap_config);
    if (IS_ERR(regmap))
        return PTR_ERR(regmap);

    return ds3231rtc_probe(&client->dev, regmap);
}

static const struct of_device_id ds3231_of_match[] = {
    { .compatible = "desmtiny,ds3231rtc" },
    { }
};
MODULE_DEVICE_TABLE(of, ds3231_of_match);

static struct i2c_driver ds3231_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = ds3231_of_match,
    },
    .probe = ds3231_i2c_probe,
};

module_i2c_driver(ds3231_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("desmtiny");
MODULE_DESCRIPTION("Real Time Clock driver");