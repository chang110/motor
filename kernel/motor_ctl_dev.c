#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <wiringPi.h>


#define MOTOR_GPIO_MAX_PIN      40
#define DEVICE_NAME             "motor_ctl"
#define DEFAULT_SETP_LONG       1.8
#define DEFAULT_SUBDIVIDE       1
#define DEFAULT_ROTATE_SPEED    0.5

static DEFINE_MUTEX(motor_ctl_lock);

static int motor_ctl_open (struct inode *indoe, struct file *filp);
static int motor_ctl_release (struct inode *inode, struct file *filp);

struct motor_ctl_drv_info{
    unsigned int major;
    struct cdev cdev;
    struct class *drv_class;
};

struct motor_data{
    double step_long;
    unsigned short subdivide;
    unsigned short turn_to;
    double rotate_speed;
};

static struct motor_ctl_drv_info motor_ctl_drv;
static struct file_operations motor_ctl_drv_fops = {
    .owner = THIS_MODULE,
    .open = motor_ctl_open,
    .release = motor_ctl_release
};

static void motor_chr_drv_destroy(void)
{
    device_destroy(motor_ctl_drv.drv_class, MKDEV(motor_ctl_drv.major, 0));
    cdev_del(&motor_ctl_drv.cdev);
    class_destroy(motor_ctl_drv.drv_class);
    unregister_chrdev_region(MKDEV(motor_ctl_drv.major, 0), 1);
}

static int motor_chr_drv_create(void)
{
    dev_t dev_id;
    struct device *dev_device;
    
    if (alloc_chrdev_region(&dev_id, 0, 1, DEVICE_NAME)){
        printk("QAT: unable to allocate chrdev region\n");
        return -EFAULT;
    }
    motor_ctl_drv.drv_class = class_create(DEVICE_NAME);
    if (IS_ERR(motor_ctl_drv.drv_class)){
        printk("QAT: class_create failed for motor_ctl\n");
        goto err_chrdev_unreg;
    }
    motor_ctl_drv.major = MAJOR(dev_id);
    cdev_init(&motor_ctl_drv.cdev, &motor_ctl_drv_fops);
    if (cdev_add(&motor_ctl_drv.cdev, dev_id, 1)){
        printk("QAT: cdev_add failed for motor_ctl\n");
        goto err_class_destr;
    }

    dev_device = device_create(motor_ctl_drv.drv_class, NULL, 
        MKDEV(motor_ctl_drv.major,0), NULL, DEVICE_NAME);
    if (IS_ERR(dev_device)){
        printk("QAT: device_create failed for motor_ctl\n");
        goto err_cdev_del;
    }
    return 0;
err_cdev_del:
    cdev_del(&motor_ctl_drv.cdev);
err_class_destr:
    class_destroy(motor_ctl_drv.drv_class);
err_chrdev_unreg:
    unregister_chrdev_region(dev_id, 1);
    return -EFAULT;
}

static int motor_ctl_open (struct inode *indoe, struct file *filp)
{

    printk("motor_ctl_open\n");

    if (wiringPiSetup() < 0){
        printk("QAT: wiringPiSetup failed for motor_ctl\n");
        return -EFAULT;
    };

    struct motor_data *prv_md = kmalloc((sizeof(struct motor_data), GFP_KERNEL);
    if (!prv_md){
        printk("QAT: kmalloc failed for motor_ctl\n");
        return -ENOMEM;
    }
	memset(&uart, 0, sizeof(uart));

    prv_md->step_long = DEFAULT_SETP_LONG;
    prv_md->subdivide = DEFAULT_SUBDIVIDE;
    prv_md->turn_to = 0;
    prv_md->rotate_speed = DEFAULT_ROTATE_SPEED;

    filp->private_data = prv_md;
    return 0;
}
static int motor_ctl_release (struct inode *inode, struct file *filp)
{
    kfree(filp->private_data);
    printk("motor_ctl_release\n");
    return 0;
}

static int __init motor_register_ctl_device_driver(void)
{
    if(motor_chr_drv_create())
        goto err_chr_dev;
    
    return 0;

err_chr_dev:
    mutex_destroy(&motor_ctl_lock);
    return -EFAULT;
}

static void __exit motor_unregister_ctl_device_driver(void)
{
    motor_chr_drv_destroy();
    mutex_destroy(&motor_ctl_lock);
}

module_init(motor_register_ctl_device_driver);
module_exit(motor_unregister_ctl_device_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("chang110 <bqq_110@163.com>");
MODULE_DESCRIPTION("Motor control device driver");