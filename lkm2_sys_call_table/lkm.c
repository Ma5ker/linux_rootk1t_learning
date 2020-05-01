#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
unsigned long **get_sys_call_table(void){
    //get sys_call_table via sys_close
    unsigned long **entry = (unsigned long **)PAGE_OFFSET;
    for(;(unsigned long)entry<ULONG_MAX;entry+=1){
        if(entry[__NR_close]==(unsigned long*)sys_close){
            return entry;
        }
    }   
    return NULL;
}

void disable_wp(void){
    unsigned long cr0;
    preempt_disable();//lock cpu
    cr0 = read_cr0();
    clear_bit(X86_CR0_WP_BIT,&cr0);
    write_cr0(cr0);
    preempt_enable();
    printk("disable WP success\n");
    return;
}

void enable_wp(void){
    unsigned long cr0;
    preempt_disable();
    cr0 = read_cr0();
    set_bit(X86_CR0_WP_BIT,&cr0);
    write_cr0(cr0);
    preempt_enable();
    printk("enable WP success\n");
    return;
}


static int lkm_init(void){
    printk("[+]lkm2 module loaded!\n");
    
    unsigned long **real_sys_call_table = get_sys_call_table();    
    printk("PAGE_OFFSET:%lx\n",PAGE_OFFSET);
    printk("sys_call_table:%p\n",real_sys_call_table);

    disable_wp();
    //modify sys_call_table
    enable_wp();
    return 0;
}

static void lkm_exit(void){
    printk("[+]lkm2 module removed\n");
}

module_init(lkm_init);
module_exit(lkm_exit);
