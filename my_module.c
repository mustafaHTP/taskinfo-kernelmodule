/**
 * @file my_module.c
 * @author Mustafa Hatipoğlu
 * @brief 
 * @version 0.1
 * @date 2022-11-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include "my_proc_ops.c"

#define PROCF_NAME "mytaskinfo"

const struct proc_ops my_ops = {
    .proc_read = my_read,
    .proc_write = my_write,
    .proc_open = my_open,
    .proc_release = my_release
};

static int __init my_module_init(void){
    proc_create(PROCF_NAME, 0666, NULL, &my_ops);

    printk(KERN_INFO "/proc/%s created\n", PROCF_NAME);

    return 0;
}

static void __exit my_module_exit(void){
    remove_proc_entry(PROCF_NAME, NULL);

    printk(KERN_INFO "/proc/%s removed\n", PROCF_NAME);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("My Task Info Module");
MODULE_AUTHOR("Mustafa Hatipoğlu");