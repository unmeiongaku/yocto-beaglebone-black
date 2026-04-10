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

    regmap_write(priv->regmap, DS3231RTC_REG_MINUTES, bin2bcd(min));
    regmap_write(priv->regmap, DS3231RTC_REG_HOURS, bin2bcd(hour));
    regmap_write(priv->regmap, DS3231RTC_REG_DATE, bin2bcd(date));
    regmap_write(priv->regmap, DS3231RTC_REG_MONTH_CENTURY, bin2bcd(month));
    regmap_write(priv->regmap, DS3231RTC_REG_YEARS, bin2bcd(year));

out:
    mutex_unlock(&priv->lock);
    return ret;
}


static ssize_t time_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct ds3231rtc_priv *priv = dev_get_drvdata(dev);
    unsigned int sec, min, hour, date, month, year;

    mutex_lock(&priv->lock);

    regmap_read(priv->regmap, DS3231RTC_REG_SECONDS, &sec);
    regmap_read(priv->regmap, DS3231RTC_REG_MINUTES, &min);
    regmap_read(priv->regmap, DS3231RTC_REG_HOURS, &hour);
    regmap_read(priv->regmap, DS3231RTC_REG_DATE, &date);
    regmap_read(priv->regmap, DS3231RTC_REG_MONTH_CENTURY, &month);
    regmap_read(priv->regmap, DS3231RTC_REG_YEARS, &year);

    mutex_unlock(&priv->lock);

    return sysfs_emit(buf, "%02u:%02u:%02u %02u/%02u/20%02u\n",
        bcd2bin(hour), bcd2bin(min), bcd2bin(sec),
        bcd2bin(date), bcd2bin(month & 0x1F), bcd2bin(year));
}

static ssize_t time_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t count)
{
    struct ds3231rtc_priv *priv = dev_get_drvdata(dev);
    int sec, min, hour, date, month, year;

    if (sscanf(buf, "%d:%d:%d %d/%d/%d",
               &hour, &min, &sec,
               &date, &month, &year) != 6)
        return -EINVAL;

    mutex_lock(&priv->lock);

    regmap_write(priv->regmap, DS3231RTC_REG_SECONDS, bin2bcd(sec));
    regmap_write(priv->regmap, DS3231RTC_REG_MINUTES, bin2bcd(min));
    regmap_write(priv->regmap, DS3231RTC_REG_HOURS, bin2bcd(hour));
    regmap_write(priv->regmap, DS3231RTC_REG_DATE, bin2bcd(date));
    regmap_write(priv->regmap, DS3231RTC_REG_MONTH_CENTURY, bin2bcd(month));
    regmap_write(priv->regmap, DS3231RTC_REG_YEARS, bin2bcd(year % 100));

    mutex_unlock(&priv->lock);

    return count;
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
        .name = "DRIVER_NAME",
        .of_match_table = ds3231_of_match,
    },
    .probe = ds3231_i2c_probe,
};

module_i2c_driver(ds3231_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("desmtiny");
MODULE_DESCRIPTION("Real Time Clock driver");