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

static int bno055_set_page_id(struct bno055_state_t *st, enum bno055_page_id_t tar_page_id);
static int bno055_set_opr_mode(struct bno055_state_t *st,enum bno055_opr_mode_t opr_mode);
static int bno055_chip_init(struct bno055_state_t *st);
static int bno_read_chip_id(struct bno055_state_t *st);

static int bno055_set_page_id(struct bno055_state_t *st,
			      enum bno055_page_id_t tar_page_id)
{
	int ret;
	unsigned int pageid;
	ret = regmap_read(st->regmap, BNO055_REG_PAGE_ID, &pageid);
	if (ret) {
		dev_err(st->dev, "Failed to read page id\n");
		goto out;
	}

	if (pageid != tar_page_id) {

		ret = regmap_write(st->regmap, BNO055_REG_PAGE_ID,
				   tar_page_id);
		if (ret) {
			dev_err(st->dev, "Failed to set page id\n");
			goto out;
		}

		st->id.page_id = tar_page_id;

		dev_info(st->dev, "Changed to page %u\n", tar_page_id);
	}

	ret = 0;

out:
	return ret;
}

static int bno055_set_opr_mode(struct bno055_state_t *st,
			      enum bno055_opr_mode_t opr_mode)
{
	int ret;
	int cur_mode;
	/*check current opr mode*/
	ret = regmap_read(st->regmap, BNO055_REG_OPR_MODE, &cur_mode);
	if(ret){
		dev_err(st->dev, "Failed to read current Mode\n");
		return ret;
	}
	/* Already in target mode */
	if (cur_mode == opr_mode) {
        return 0;
    }
	/* Move to CONFIGMODE first if not already */
	if(cur_mode!=BNO055_OPR_MODE_CONFIG){
		int cfg_mode = BNO055_OPR_MODE_CONFIG;
		ret = regmap_write(st->regmap, BNO055_REG_OPR_MODE,cfg_mode);
		if(ret){
			dev_err(st->dev, "Failed to Set Config Mode\n");
			return ret;
		}
		msleep(20);
	}
	/* If target is CONFIGMODE we're done */
	if (opr_mode == BNO055_OPR_MODE_CONFIG) {
		st->opr_mode = opr_mode;
		return 0;
	}
	/* Set target mode */
	ret = regmap_write(st->regmap, BNO055_REG_OPR_MODE, opr_mode);
	if (ret)
		return ret;

	msleep(20);

	/* Verify mode */
	ret = regmap_read(st->regmap, BNO055_REG_OPR_MODE, &cur_mode);
	if (ret)
		return ret;
	if (cur_mode != opr_mode)
		return -EIO;
	st->opr_mode = opr_mode;
	return 0;
}

static int bno055_chip_init(struct bno055_state_t *st)
{
	int ret;
	unsigned int val;

	ret = regmap_read(st->regmap, BNO055_CHIP_ID, &val);
	if (ret)
		return ret;

	if (val != BNO055_CHIP_ID_VALUE) {
		dev_err(st->dev, "Invalid chip id 0x%x\n", val);
		return -ENODEV;
	}

	dev_info(st->dev, "BNO055 detected\n");

	st->id.page_id = PAGE_ID_0;

	ret = regmap_write(st->regmap, BNO055_REG_PAGE_ID,
			   st->id.page_id);
	if (ret) {
		dev_err(st->dev, "Failed to set default page\n");
		return ret;
	}

	/*Reset CHip*/
	if(st->id.page_id != PAGE_ID_0) bno055_set_page_id(st,PAGE_ID_0);
	int tmp = BNO055_SYS_TRIGGER_RST_SYS;
	dev_info(st->dev, "Reset BNO055 Device: ");
	ret = regmap_write(st->regmap, BNO055_REG_SYS_TRIGGER,tmp);
	if(ret){
		dev_err(st->dev, "Reset Failed\n");
		return ret;
	}
	msleep(600);

	//set BNO055_REG_SYS_TRIGGER TO 0x00
	tmp = BNO055_SYS_DEFAULT_SYSTEM;
	ret = regmap_write(st->regmap, BNO055_REG_SYS_TRIGGER,tmp);
	if(ret){
		dev_err(st->dev, "Reset Failed\n");
		return ret;
	}
	dev_info(st->dev, "Success\n");
	msleep(50);

	/*Set Operation Mode to CONFIG Mode*/
	dev_info(st->dev, "Set Operation Mode: CONFIG_MODE");
	if(bno055_set_opr_mode(st,BNO055_OPERATION_CONFIG_MODE)!=0){
		dev_err(st->dev, "Failed\n");
		return -1;
	}
	dev_info(st->dev, "Success\n");

	


	dev_dbg(st->dev, "BNO055 default initialization done\n");
	return 0;
}

int bno055_probe(struct i2c_client *client)
{
	struct iio_dev *indio_dev;
	struct bno055_state_t *st;
	int chip_id;
	int ret;
    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;
	st = iio_priv(indio_dev);
	mutex_init(&st->lock);
	st->regmap = devm_regmap_init_i2c(client, NULL);
	if (IS_ERR(st->regmap))
	return PTR_ERR(st->regmap);
	ret = bno055_chip_init(st);
	if (ret)
		return ret;
	

}

void bno055_remove(struct i2c_client *client)
{
	dev_info(dev, "BNO055 removed\n");
	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);  
MODULE_VERSION(DRIVER_VERS);