#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define DRIVER_NAME "m_cdev"
#define DRIVER_AUTHOR "desmtiny nguyenhoangminh@gmail.com"
#define DRIVER_DESC   "Character Device"
#define DRIVER_VERS   "1.0"
/* ring buffer config */

#define RING_SIZE 16
#define MSG_SIZE 128

#define M_IOCTL_CLEAR_BUFFER _IO('m', 0)
#define M_IOCTL_GET_SIZE     _IOR('m', 1, int)

/* ================= RING BUFFER ================= */

struct ring_buffer {

    char data[RING_SIZE][MSG_SIZE];

    int head;
    int tail;
    int count;
};

/* ================= DEVICE ================= */

struct m_device {

    dev_t dev_num;

    struct cdev cdev;

    struct class *class;

    struct device *device;

    struct ring_buffer rb;

    struct mutex lock;

    wait_queue_head_t read_queue;
};

static struct m_device mdev;

/* ================= RING BUFFER OPS ================= */

static int rb_push(struct ring_buffer *rb, const char *msg)
{
    if (rb->count == RING_SIZE)
        return -1;

    strncpy(rb->data[rb->head], msg, MSG_SIZE-1);

    rb->head = (rb->head + 1) % RING_SIZE;

    rb->count++;

    return 0;
}

static int rb_pop(struct ring_buffer *rb, char *msg)
{
    if (rb->count == 0)
        return -1;

    strncpy(msg, rb->data[rb->tail], MSG_SIZE);

    rb->tail = (rb->tail + 1) % RING_SIZE;

    rb->count--;

    return strlen(msg);
}

/* ================= FILE OPS ================= */

static int m_open(struct inode *inode, struct file *file)
{
    struct m_device *dev;

    dev = container_of(inode->i_cdev, struct m_device, cdev);

    file->private_data = dev;

    pr_info("Device opened\n");

    return 0;
}

static int m_release(struct inode *inode, struct file *file)
{
    pr_info("Device closed\n");

    return 0;
}

/* ================= WRITE ================= */

static ssize_t m_write(struct file *file,
                       const char __user *buf,
                       size_t len,
                       loff_t *offset)
{
    struct m_device *dev = file->private_data;

    char kbuf[MSG_SIZE];

    if (len > MSG_SIZE-1)
        len = MSG_SIZE-1;

    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    kbuf[len] = '\0';

    mutex_lock(&dev->lock);

    if (rb_push(&dev->rb, kbuf)) {

        mutex_unlock(&dev->lock);

        return -ENOMEM;
    }

    mutex_unlock(&dev->lock);

    wake_up_interruptible(&dev->read_queue);

    pr_info("Push message: %s\n", kbuf);

    return len;
}

/* ================= READ ================= */

static ssize_t m_read(struct file *file,
                      char __user *buf,
                      size_t len,
                      loff_t *offset)
{
    struct m_device *dev = file->private_data;

    char kbuf[MSG_SIZE];

    int ret;

    wait_event_interruptible(dev->read_queue,
                             dev->rb.count > 0);

    mutex_lock(&dev->lock);

    ret = rb_pop(&dev->rb, kbuf);

    mutex_unlock(&dev->lock);

    if (ret < 0)
        return 0;

    if (copy_to_user(buf, kbuf, ret))
        return -EFAULT;

    return ret;
}

/* ================= IOCTL ================= */

static long m_ioctl(struct file *file,
                    unsigned int cmd,
                    unsigned long arg)
{
    struct m_device *dev = file->private_data;

    int size;

    switch(cmd)
    {

        case M_IOCTL_CLEAR_BUFFER:

            mutex_lock(&dev->lock);

            dev->rb.head = 0;
            dev->rb.tail = 0;
            dev->rb.count = 0;

            mutex_unlock(&dev->lock);

            break;

        case M_IOCTL_GET_SIZE:

            size = dev->rb.count;

            if (copy_to_user((int __user *)arg,
                             &size,
                             sizeof(int)))
                return -EFAULT;

            break;

        default:

            return -EINVAL;
    }

    return 0;
}

/* ================= POLL ================= */

static __poll_t m_poll(struct file *file,
                       poll_table *wait)
{
    struct m_device *dev = file->private_data;

    poll_wait(file, &dev->read_queue, wait);

    if (dev->rb.count > 0)
        return POLLIN | POLLRDNORM;

    return 0;
}

/* ================= FOPS ================= */

static const struct file_operations fops = {

    .owner = THIS_MODULE,

    .open = m_open,

    .release = m_release,

    .read = m_read,

    .write = m_write,

    .unlocked_ioctl = m_ioctl,

    .poll = m_poll,
};

/* ================= INIT ================= */

static int __init m_init(void)
{
    pr_info("Character Device Driver\n");

    int ret;

    ret = alloc_chrdev_region(&mdev.dev_num,
                              0,
                              1,
                              DRIVER_NAME);

    if (ret)
        return ret;

    cdev_init(&mdev.cdev, &fops);

    ret = cdev_add(&mdev.cdev,
                   mdev.dev_num,
                   1);

    if (ret)
        goto err;

    mdev.class = class_create(DRIVER_NAME);

    mdev.device = device_create(
                    mdev.class,
                    NULL,
                    mdev.dev_num,
                    NULL,
                    DRIVER_NAME);

    mutex_init(&mdev.lock);

    init_waitqueue_head(&mdev.read_queue);

    mdev.rb.head = 0;
    mdev.rb.tail = 0;
    mdev.rb.count = 0;

    pr_info("ring buffer driver loaded\n");

    return 0;

err:

    unregister_chrdev_region(mdev.dev_num, 1);

    return ret;
}

/* ================= EXIT ================= */

static void __exit m_exit(void)
{
    device_destroy(mdev.class, mdev.dev_num);

    class_destroy(mdev.class);

    cdev_del(&mdev.cdev);

    unregister_chrdev_region(mdev.dev_num, 1);

    pr_info("driver unloaded\n");
}

module_init(m_init);
module_exit(m_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);  
MODULE_VERSION(DRIVER_VERS);