#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int lkm_init(void){
    printk("[+]lkm1 module loaded!\n");
    return 0;
}

static void lkm_exit(void){
    printk("[+]lkm1 module removed\n");
}

module_init(lkm_init);
module_exit(lkm_exit);
