#include <linux/module.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>

#include <linux/dma-direct.h>
#include <linux/mm.h>

#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tom Humbertclaude");
MODULE_DESCRIPTION("Export allocated DMA coherent memory adress for external audio use");

#define DEVICE_NAME                         "my_dma_allocator"

static dev_t dev_num;
static struct class *my_class;
static struct device *my_device;

struct my_buffer_s{
    void* addr_virt;
    dma_addr_t addr_dma;
    size_t dma_size;
};

static struct my_buffer_s my_buffer;

static void my_dma_free(void) {
    if (!my_buffer.addr_virt)
        return;
    dma_free_coherent(my_device, my_buffer.dma_size, my_buffer.addr_virt, my_buffer.addr_dma);
    memset(&my_buffer, 0, sizeof(my_buffer));
    pr_info("Freed DMA region\n");
}

static long my_dma_alloc(size_t size) {
    dma_addr_t _addr_dma;
    
    if (my_buffer.addr_virt) {
        my_dma_free();
    }

    my_buffer.addr_virt = dma_alloc_coherent( my_device,
                                                   PAGE_ALIGN(size), 
                                                   &_addr_dma, 
                                                   GFP_KERNEL);
    if (!my_buffer.addr_virt) {
        pr_err("dma_alloc_coherent failed\n");
        return -ENOMEM;
    }
    my_buffer.dma_size = PAGE_ALIGN(size);
    my_buffer.addr_dma = _addr_dma;
    pr_info("DMA virt addr: %px\nDMA phys addr: %llx\nSize: %ld\n", my_buffer.addr_virt, dma_to_phys(my_device, my_buffer.addr_dma), my_buffer.dma_size);
    return 0;
}

static ssize_t addr_phys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%llx\n", dma_to_phys(my_device, my_buffer.addr_dma));
}

static ssize_t dma_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%ld\n", my_buffer.dma_size);
}

static ssize_t dma_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    size_t size = 0;
    if (kstrtoul(buf, 10, &size))
        return -1;
    if (size) {
        my_dma_alloc(size);
    }
    else {
        my_dma_free();
    }
    return count;
}

static struct kobj_attribute addr_phys_attr = __ATTR_RO(addr_phys);
static struct kobj_attribute dma_size_attr = __ATTR_RW(dma_size);

static struct kobject *my_kobj;

static int __init my_dma_allocator_init(void) {
    long ret;

    my_kobj = kobject_create_and_add(DEVICE_NAME, kernel_kobj);
    if (IS_ERR(my_kobj))
        return PTR_ERR(my_kobj);

    ret = sysfs_create_file(my_kobj, &addr_phys_attr.attr);
    if (ret)
        goto rm_kobj;
    ret = sysfs_create_file(my_kobj, &dma_size_attr.attr);
    if (ret)
        goto rm_first_sysfs;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret)
        goto rm_all_sysfs;
    my_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        goto unreg_dev_region;
    }
    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        ret = PTR_ERR(my_device);
        goto destroy_class;
    }
    ret = dma_set_coherent_mask(my_device, dma_get_required_mask(my_device));
    if (ret)
        goto destroy_device;
        
    memset(&my_buffer, 0, sizeof(my_buffer));

    return 0;

destroy_device:
    device_destroy(my_class, dev_num);
destroy_class:
    class_destroy(my_class);
unreg_dev_region:
    unregister_chrdev_region(dev_num, 1);
rm_all_sysfs:
    sysfs_remove_file(my_kobj, &dma_size_attr.attr);
rm_first_sysfs:
    sysfs_remove_file(my_kobj, &addr_phys_attr.attr);
rm_kobj:
    kobject_put(my_kobj);

    return ret;
}

static void __exit my_dma_allocator_exit(void) {
    if (my_buffer.addr_virt)
        my_dma_free();
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    unregister_chrdev_region(dev_num, 1);
    sysfs_remove_file(my_kobj, &dma_size_attr.attr);
    sysfs_remove_file(my_kobj, &addr_phys_attr.attr);
    kobject_put(my_kobj);
}

module_init(my_dma_allocator_init);
module_exit(my_dma_allocator_exit);