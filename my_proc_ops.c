/**
 * @file my_proc_ops.c
 * @author Mustafa Hatipoğlu
 * @brief 
 * @version 0.1
 * @date 2022-11-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <asm/uaccess.h>
#include <linux/init.h>   /* Needed for the macros */
#include <linux/kernel.h> /* Needed for pr_info() */
#include <linux/proc_fs.h> /*proc_ops, proc)create, proc_remove, remove_proc_entry...*/
#include <linux/slab.h> /*for kmalloc()*/

#define MYBUF_SIZE 256
#define MYDATA_SIZE 256
struct my_data {
    int size;
    char *buf; /* my data starts here */
};

unsigned long int number_of_process;

/**
 * TODO: taskinfoya gore ayarlamaniz gerekiyor
 * @brief
 *
 * @param inode
 * @param file
 * @return int
 */
int my_open(struct inode *inode, struct file *file) {
    struct my_data *my_data =
        kmalloc(sizeof(struct my_data) * MYBUF_SIZE, GFP_KERNEL);
    my_data->buf = kmalloc(sizeof(char) * MYBUF_SIZE, GFP_KERNEL);
    my_data->size = MYBUF_SIZE;

    /* validate access to data
    Not: diger fonksiyonlarda,
    private_datayi farkli adrese point ettirmeyin,
    yada once my_datayi kfree ile free edin*/
    file->private_data = my_data;

    return 0;
}
/**
 * TODO: taskinfoya gore ayarlamaniz gerekiyor
 *
 * @param inode
 * @param file
 * @return int
 */
int my_release(struct inode *inode, struct file *file) {
    /*free all memories*/
    struct my_data *my_data = file->private_data;
    kfree(my_data->buf);
    kfree(my_data);

    return 0;
}

/**
 * TODO: taskinfoya gore ayarlamaniz gerekiyor
 * @brief copy data from mydata->buffer to user_buf,
 * file: opened file data
 * usr_buf: usr buffer
 * offset: the cursor position on the mydata->buf from the last call
 * file:
 */
ssize_t my_read(struct file *file, char __user *usr_buf, size_t size,
                loff_t *offset) {

    struct my_data *my_data = (struct my_data *)file->private_data;
    
    char process_info[MYBUF_SIZE] = {'\0'};
    
    /* if number_of_process equals 1, then find process has biggest totaltime */
    struct task_struct *task_max;
    struct task_struct *task_cursor;
    u64 max_total_time = 0;
    u64 total_time = 0;
    u64 utime = 0;
    u64 stime = 0;

    for_each_process(task_cursor) {
        utime = task_cursor->utime;
        stime = task_cursor->stime;
        total_time = utime + stime;

        if (total_time > max_total_time) {
            max_total_time = total_time;
            task_max = task_cursor;
        }
    }

    pid_t task_max_pid;
    u64 utime_inseconds;
    u64 stime_inseconds;
    u64 totaltime_inseconds;
    u64 vruntime;

    task_max_pid = task_max->pid;
    utime_inseconds = nsecs_to_jiffies64(task_max->utime) / HZ;
    stime_inseconds = nsecs_to_jiffies64(task_max->stime) / HZ;
    totaltime_inseconds = utime_inseconds + stime_inseconds;
    vruntime = task_max->se.vruntime;
    
    int total_bytes_written = 0;
    ssize_t bytes_to_be_written =
        sprintf(process_info,
                "process running times\npid = %d, utime = %llu, stime = %llu, "
                "utime+stime = %llu, vruntime = %llu\n",
                task_max_pid, utime_inseconds, stime_inseconds,
                totaltime_inseconds, vruntime);

    // check buffer overflow
    int bytes_written =
        min((int)(my_data->size - *offset), (int)bytes_to_be_written);
    if (bytes_written <= 0) return total_bytes_written;

    strcpy(my_data->buf + *offset, process_info);
    if (copy_to_user(usr_buf, my_data->buf + *offset, bytes_written)) return -EFAULT;

    // change offset value
    *offset += bytes_written;
    total_bytes_written += bytes_written;

    /**
     * @brief if the number of process is bigger than 1,
     * find the number of processes have biggest total value
     * in given number(number_of_process)
     *
     */
    if (number_of_process > 1) {
        u64 max_total_time = task_max->utime + task_max->stime;
        u64 local_max_total_time = 0;
        int i;
        for (i = 0; i < number_of_process - 1; i++) {
            for_each_process(task_cursor) {
                utime = task_cursor->utime;
                stime = task_cursor->stime;
                total_time = utime + stime;

                if (total_time < max_total_time &&
                    total_time > local_max_total_time) {
                    local_max_total_time = total_time;
                    task_max = task_cursor;
                }
            }
            max_total_time = task_max->stime + task_max->utime;
            local_max_total_time = 0;

            task_max_pid = task_max->pid;
            utime_inseconds = nsecs_to_jiffies64(task_max->utime) / HZ;
            stime_inseconds = nsecs_to_jiffies64(task_max->stime) / HZ;
            totaltime_inseconds = utime_inseconds + stime_inseconds;
            vruntime = task_max->se.vruntime;

            // after find task max write to mytaskinfo
            ssize_t bytes_to_be_written =
                sprintf(process_info,
                        "pid = %d, utime = %llu, stime = %llu, utime+stime = "
                        "%llu, vruntime = %llu\n",
                        task_max_pid, utime_inseconds, stime_inseconds,
                        totaltime_inseconds, vruntime);

            // check buffer overflow
            int bytes_written =
                min((int)(my_data->size - *offset), (int)bytes_to_be_written);
            if (bytes_written <= 0) return total_bytes_written;

            strcat(my_data->buf + *offset, process_info);
            if (copy_to_user(usr_buf + *offset, my_data->buf + *offset, bytes_written)) return -EFAULT;
            // change offset value
            *offset += bytes_written;

            total_bytes_written += bytes_written;
        }
    }

    return total_bytes_written; /*the number of bytes copied*/
}

ssize_t my_read_simple(struct file *file, char __user *usr_buf, size_t size,
                       loff_t *offset) {
    char buf[MYBUF_SIZE] = {'\0'};

    int len = sprintf(buf, "Hello World\n");

    /* copy len byte to userspace usr_buf
     Returns number of bytes that could not be copied.
     On success, this will be zero.
    */
    if (copy_to_user(usr_buf, buf, len)) return -EFAULT;

    return len; /*the number of bytes copied*/
}

/**
 * @brief TODO: task infoya gore ayarlamaniz gerekiyor
 *
 * @param file
 * @param usr_buf
 * @param size
 * @param offset
 * @return ssize_t
 */
ssize_t my_write(struct file *file, const char __user *usr_buf, size_t size,
                 loff_t *offset) {
    char *buf = kmalloc(size + 1, GFP_KERNEL);
    //struct my_data *my_data = (struct my_data *)file->private_data;

    /* copies user space usr_buf to kernel buffer */
    if (copy_from_user(buf, usr_buf, size)) {
        printk(KERN_INFO "Error copying from user\n");
        return -EFAULT;
    }
    // *offset += size;
    /* yine offseti bazi durumlarda set etmeniz vs gerekebilir,
    user tekrar write yaptiginda buf+*offsete yaziyor */
    buf[size] = '\0';

    // get number of process
    int retval;
    retval = kstrtoul(buf, 10, &number_of_process);
    if (retval == -ERANGE) printk(KERN_INFO "Error kstrtoul overflow");
    if (retval == -EINVAL) printk(KERN_INFO "Error kstrtoul parsing");

    kfree(buf);
    return size;
}