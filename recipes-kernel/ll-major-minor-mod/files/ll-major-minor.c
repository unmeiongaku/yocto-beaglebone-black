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
#include <linux/mutex.h>

#define DRIVER_AUTHOR "desmtiny nguyenhoangminhdo@gmail.com"
#define DRIVER_DESC   "Hello world kernel module"
#define DRIVER_VERS   "1.0"
#define DEVICE_NAME "m_cdev"

#define BUFFER_SIZE 1028

static struct m_foo_dev {
    dev_t dev_num;
    struct cdev m_cdev;
    struct class *m_class;
    struct device *m_device;
    char buffer[BUFFER_SIZE];
    size_t size;
    struct mutex lock;
};

static struct m_foo_dev mdev;

/* ================= FILE OPERATIONS ================= */
static int m_open(struct inode *inode, struct file *file)
{
    pr_info("Message From Kernel\n");
    pr_info("Device opened\n");
    struct m_foo_dev *dev;
    dev = container_of(inode->i_cdev, struct m_foo_dev, m_cdev);
    file->private_data = dev;
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
    /*Write Text To m_cdev file and userspace can read text from /dev/m_cdev "Hello from kernel space!*/
    // char msg[] = "Hello from kernel space!\n";
    // size_t msg_len = sizeof(msg);

    // if (*offset >= msg_len)
    //     return 0;

    // if (copy_to_user(buf, m_classsg, msg_len))
    //     return -EFAULT;

    // *offset += msg_len;
    // return msg_len;
    /*Userspace write data to Kernel space m_cdev file then read file dev/m_cdev*/
    
    struct m_foo_dev *dev = file->private_data;
    ssize_t ret;

    if (*offset >= dev->size)
        return 0;

    if (len > dev->size - *offset)
        len = dev->size - *offset;

    mutex_lock(&dev->lock);

    if (copy_to_user(buf, dev->buffer + *offset, len)) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }

    *offset += len;
    ret = len;

    mutex_unlock(&dev->lock);

    return ret;
}

static ssize_t m_write(struct file *file, const char __user *buf,
                       size_t len, loff_t *offset)
{
    struct m_foo_dev *dev = file->private_data;

    if (len > BUFFER_SIZE)
        len = BUFFER_SIZE;

    mutex_lock(&dev->lock);

    if (copy_from_user(dev->buffer, buf, len)) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }

    dev->size = len;

    mutex_unlock(&dev->lock);

    pr_info("Received %zu bytes from user\n", len);

    return len;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open  = m_open,
    .release = m_release,
    .read  = m_read,
    .write = m_write,
};

static int __init chdev_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&mdev.dev_num, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&mdev.m_cdev, &fops);
    mdev.m_cdev.owner = THIS_MODULE;

    ret = cdev_add(&mdev.m_cdev, mdev.dev_num, 1);
    if (ret)
        goto err_unregister;

    mdev.m_class = class_create(DEVICE_NAME);
    if (IS_ERR(mdev.m_class)) {
        ret = PTR_ERR(mdev.m_class);
        goto err_cdev;
    }

    mdev.m_device = device_create(mdev.m_class, NULL, mdev.dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(mdev.m_device)) {
        ret = PTR_ERR(mdev.m_device);
        goto err_class;
    }

    mutex_init(&mdev.lock);

    strcpy(mdev.buffer, "Hello from kernel space\n");
    mdev.size = strlen(mdev.buffer);

    pr_info("Driver loaded successfully\n");
    return 0;

err_class:
    class_destroy(mdev.m_class);

err_cdev:
    cdev_del(&mdev.m_cdev);

err_unregister:
    unregister_chrdev_region(mdev.dev_num, 1);

    return ret;
}

static void __exit chdev_exit(void)
{
    device_destroy(mdev.m_class, mdev.dev_num);
    class_destroy(mdev.m_class);
    cdev_del(&mdev.m_cdev);
    unregister_chrdev_region(mdev.dev_num, 1);              /* 1.0 */
    pr_info("Driver unloaded\n");
}

module_init(chdev_init);
module_exit(chdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERS);