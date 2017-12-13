#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>

#define FIRST_MINOR 0
#define MINOR_CNT 1

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

static char c = 'A';
static struct semaphore my_sem;

static int my_open(struct inode *i, struct file *f)
{
	printk("%s %d: Got here\n", __func__, __LINE__);
	return 0;
}

static int my_close(struct inode *i, struct file *f)
{
	printk("%s %d: Got here\n", __func__, __LINE__);
	return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len,
		loff_t *off)
{
    /* Acquire the Semaphore */
    if (down_interruptible(&my_sem)) {
        printk("Unable to acquire Semaphore\n");
        return -1;
    }

    return 0;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len,
        loff_t *off)
{
    /* Release the semaphore */
    up(&my_sem);
    if (copy_from_user(&c, buf + len - 1, 1)) {
        return -EFAULT;
    }

    return len;
}

static struct file_operations driver_fops =
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.read = my_read,
	.write = my_write
};

static int __init sem_init_module(void)
{
	int ret;
	struct device *dev_ret;

	if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR,
					MINOR_CNT, "my_sem")) < 0) {
		return ret;
	}

	cdev_init(&c_dev, &driver_fops);

	if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0) {
		unregister_chrdev_region(dev, MINOR_CNT);
		return ret;
	}

	if (IS_ERR(cl = class_create(THIS_MODULE, "char"))) {
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(cl);
	}

	if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL,
					"mysem%d", FIRST_MINOR))) {
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(dev_ret);
	}

	/* Init binary semaphore */
	sema_init(&my_sem, 1);

	printk(KERN_INFO "Hello World\n");
	return 0;
}

static void __exit sem_exit_module(void) {
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
	printk(KERN_INFO "Bye\n");
}

module_init(sem_init_module);
module_exit(sem_exit_module);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Binary semaphore demo module");
