/******************************************************************************
 *
 *   Copyright (C) 2011  Intel Corporation. All rights reserved.
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *****************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>       /* Define alloc_chrdev_region(), register_chrdev_region() */
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DRIVER_AUTHOR "desmtiny nguyenhoangminhdo@gmail.com"
#define DRIVER_DESC   "Hello world kernel module"
#define DRIVER_VERS   "1.0"
#define DEVICE_NAME "m_cdev"

static struct m_foo_dev {
    dev_t dev_num;
    struct cdev m_cdev;
    struct class *m_class;
    struct device *m_device;
} mdev;

/* ================= FILE OPERATIONS ================= */
static int m_open(struct inode *inode, struct file *file)
{
    pr_info("Device opened\n");
    return 0;
}

static int m_release(struct inode *inode, struct file *file)
{
    pr_info("Device closed\n");
    return 0;
}

static ssize_t m_read(struct file *file, char __user *buf,
                      size_t len, loff_t *offset)
{
    char msg[] = "Hello from kernel space!\n";
    size_t msg_len = sizeof(msg);

    if (*offset >= msg_len)
        return 0;

    if (copy_to_user(buf, msg, msg_len))
        return -EFAULT;

    *offset += msg_len;
    return msg_len;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open  = m_open,
    .release = m_release,
    .read  = m_read,
};

static int __init chdev_init(void)
{
    int ret;
    /* 1.0 Dynamic allocating device number (cat /proc/devices) */
    ret = alloc_chrdev_region(&mdev.dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
	    pr_err("Failed to alloc chrdev region\n");
	    return ret;
    }
    // dev_t dev = MKDEV(173, 0);
    // register_chrdev_region(&mdev.dev_num, 1, "m-cdev")
    pr_info("Major = %d Minor = %d\n", MAJOR(mdev.dev_num), MINOR(mdev.dev_num));
    /* 2. Init cdev */
    cdev_init(&mdev.m_cdev, &fops);
    mdev.m_cdev.owner = THIS_MODULE;
    /* 3. Add cdev to kernel */
    ret = cdev_add(&mdev.m_cdev,mdev.dev_num, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(mdev.dev_num, 1);
        return ret;
    }
    /* 4. Create class */
    mdev.m_class = class_create(DEVICE_NAME);
    if (IS_ERR(mdev.m_class)) {
        pr_err("Failed to create class\n");
        cdev_del(&mdev.m_cdev);
        unregister_chrdev_region(mdev.dev_num, 1);
        return PTR_ERR(mdev.m_class);
    }
    /* 5. Create device node in /dev */
    mdev.m_device = device_create(mdev.m_class, NULL, mdev.dev_num, NULL, DEVICE_NAME);
        if (IS_ERR(mdev.m_device)) {
        pr_err("Failed to create device\n");
        class_destroy(mdev.m_class);
        cdev_del(&mdev.m_cdev);
        unregister_chrdev_region(mdev.dev_num, 1);
        return PTR_ERR(mdev.m_device);
    }
    pr_info("Driver loaded successfully\n");
    return 0;
}

static void __exit chdev_exit(void)
{
    device_destroy(mdev.m_class, mdev.dev_num);
    class_destroy(mdev.m_class);
    cdev_del(&mdev.m_cdev);
    unregister_chrdev_region(mdev.dev_num, 1);              /* 1.0 */
    pr_info("Driver unloaded\n\n");
}

module_init(chdev_init);
module_exit(chdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERS);