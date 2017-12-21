/*
* ================================================================================
*
*shared_memory.c
*		Establece un área de memoria, reservada por dma, compartida entre los nucleos
*		para cumplir la función de canal de comunicaciones.
*
*
* ================================================================================
*/
#include <linux/module.h> // Needed by all modules
#include <linux/kernel.h> // Needed for KERN_INFO
#include <linux/init.h>
#include <linux/slab.h>	//API kmalloc
#include <linux/cdev.h> //cdev struct
#include <linux/fs.h>//file system support
#include <linux/device.h>//kernel driver model

#define DEVICE_NAME "shm"
#define CLASS_NAME "sh"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Isak Edo Vivancos - 682405, Dariel Figueredo Piñero - 568659");
MODULE_DESCRIPTION("Gestión memoria compartida para los cores lx y baremetal");
MODULE_VERSION("0.1.01");

//=====================================
// constantes
#define MEM_SIZE (sizeof(u32)*1024)

//=====================================
// variables globales
//static int iterador = 0;//iterador para lecturas a trozos
static void * init_addr;

static int dev_open(struct inode *inodep, struct file *filep);
static int dev_relaese(struct inode *inodep, struct file *filep);
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset);
static ssize_t dev_write(struct file *filep, char *buffer, size_t len, loff_t *offset);
static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);

static struct cdev cdev;
static struct file_operations shared_memory_smops=
{
	.read = dev_read,
	.write = dev_write,
	.open = dev_open,
	.release = dev_relaese,
	.unlocked_ioctl = dev_ioctl,
};

//================================
//funciones driver
static int dev_open(struct inode *inodep, struct file *filep)
{
	printk(KERN_INFO "Reservando espacio compartido..\n");
	//Reserva 4MB, maximo 16MB
	init_addr = kmalloc(MEM_SIZE,__GFP_DMA);

	if(!init_addr)
	{
		printk(KERN_INFO "Error, no se ha podido reservar\n");
		return -1;
	}

	printk(KERN_INFO "Se ha reservado: %zu bytes\n",ksize(init_addr));
	printk(KERN_INFO "La direccion de inicio es: 0x%x\n",(u32)init_addr);
	return 0;
}
static int dev_release(struct inode *inodep, struct file *filep)
{
		kfree(init_addr);
		return 0;
}
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	return -1;
}
static ssize_t dev_write(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	return -1;
}
static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	return (long)init_addr;
}

//================================
//funciones modulo

int init_module(void)
{
	printk(KERN_INFO "Successfully installed \n");
	return 0;
}

void cleanup_module(void)
{
	printk(KERN_INFO "Successfully uninstalled \n");
}
