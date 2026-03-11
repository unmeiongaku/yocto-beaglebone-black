#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/regmap.h>
#include <linux/delay.h>

#include "bno055_registers.h"


#define DRIVER_NAME   "bno055_dev"
#define DRIVER_AUTHOR "desmtiny nguyenhoangminh@gmail.com"
#define DRIVER_DESC   "BoshBosch BNO055 IMU Driver"
#define DRIVER_VERS   "1.0"

int bno055_core_probe(struct device *dev, struct regmap *regmap)
{


}
