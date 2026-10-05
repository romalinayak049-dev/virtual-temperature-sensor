
#define pr_fmt(fmt) "vtempsensor: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define DEVICE_NAME     "vtempsensor"
#define TEMP_MIN        (-50)
#define TEMP_MAX        150
#define CRITICAL_LIMIT  60    
#define WRITE_BUF_SIZE  32

static int temperature = 25;    
static int threshold = 40;      
static DEFINE_MUTEX(sensor_lock);

static const char *sensor_state(void)
{
	if (temperature >= CRITICAL_LIMIT)
		return "CRITICAL";
	if (temperature >= threshold)
		return "WARNING";
	return "NORMAL";
}

static int vts_open(struct inode *inode, struct file *file)
{
	pr_info("device opened\n");
	return 0;
}

static int vts_release(struct inode *inode, struct file *file)
{
	pr_info("device closed\n");
	return 0;
}

static ssize_t vts_read(struct file *file, char __user *ubuf,
			size_t len, loff_t *off)
{
	char out[96];
	int n;

	mutex_lock(&sensor_lock);
	n = scnprintf(out, sizeof(out),
		      "temperature=%d C\nthreshold=%d C\nstate=%s\n",
		      temperature, threshold, sensor_state());
	mutex_unlock(&sensor_lock);

	/* Handles *off, so "cat" sees EOF after one pass. */
	return simple_read_from_buffer(ubuf, len, off, out, n);
}

static ssize_t vts_write(struct file *file, const char __user *ubuf,
			 size_t len, loff_t *off)
{
	char buf[WRITE_BUF_SIZE];
	char *arg;
	int val, ret;
	bool is_threshold;

	if (len == 0 || len >= sizeof(buf))
		return -EINVAL;
	if (copy_from_user(buf, ubuf, len))
		return -EFAULT;
	buf[len] = '\0';

	arg = strim(buf);
	is_threshold = (strncmp(arg, "threshold=", 10) == 0);
	if (is_threshold)
		arg += 10;

	ret = kstrtoint(arg, 10, &val);
	if (ret)
		return ret;
	if (val < TEMP_MIN || val > TEMP_MAX)
		return -EINVAL;

	mutex_lock(&sensor_lock);
	if (is_threshold)
		threshold = val;
	else
		temperature = val;
	mutex_unlock(&sensor_lock);

	return len;
}

static const struct file_operations vts_fops = {
	.owner   = THIS_MODULE,
	.open    = vts_open,
	.release = vts_release,
	.read    = vts_read,
	.write   = vts_write,
};

static struct miscdevice vts_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,       /* creates /dev/vtempsensor */
	.fops  = &vts_fops,
	.mode  = 0666,
};

static int __init vts_init(void)
{
	int ret = misc_register(&vts_dev);

	if (ret) {
		pr_err("failed to register device (%d)\n", ret);
		return ret;
	}
	pr_info("module loaded\n");
	pr_info("character device registered: /dev/%s\n", DEVICE_NAME);
	return 0;
}

static void __exit vts_exit(void)
{
	misc_deregister(&vts_dev);
	pr_info("module unloaded\n");
}

module_init(vts_init);
module_exit(vts_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Romali");
MODULE_DESCRIPTION("Virtual temperature sensor character device driver");
