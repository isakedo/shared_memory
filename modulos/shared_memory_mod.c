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

#define DEVICE_NAME "sharedmemory"
#define CLASS_NAME "shmem"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Isak Edo Vivancos - 682405, Dariel Figueredo Piñero - 568659");
MODULE_DESCRIPTION("Gestión memoria compartida para los cores lx y baremetal");
MODULE_VERSION("0.2.01");

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

static int    majorNumber;
static struct cdev cdev;
static struct class*  shmemClass  = NULL;
static struct device* shmemDevice = NULL;

static struct file_operations shared_memory_smops=
{
	.read = dev_read,
	.write = dev_write,
	.open = dev_open,
	.release = dev_release,
	.unlocked_ioctl = dev_ioctl,
};

//================================
//funciones driver
/*
* dev_open
*
* Se encarga de reservar la memoria compartida. Es importante
* llamar a close(dev_release) para liberar dicha memoria.
*/
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

/*
* dev_release
*
* Se encarga de liberar la memoria reservada durante el open.
*
*/
static int dev_release(struct inode *inodep, struct file *filep)
{
		kfree(init_addr);
		return 0;
}

/* dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
*
*	buffer: buffer de salida para el contenido copiado
* len: cantidad de contenido a copiar < 4MB
*
*/
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{

	if(len <= MEM_SIZE)
	{
			int i = 0;
			while(i < len)
			{
				buffer = init_addr[i];
				printk(KERN_INFO "read buffer %x\n", buffer[i]);
				printk(KERN_INFO "read init_addr %x\n", init_addr[i]);
				i++;
			}
			return i*4;//número de bytes leidos
	}
	return -1;
}


static ssize_t dev_write(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	if(len <= MEM_SIZE)
	{
		//void j* = init_addr;
		int i = 0;
		while(i < len)
		{
			init_addr[i] = buffer[i];
			printk(KERN_INFO "write buffer %x\n", buffer[i]);
			printk(KERN_INFO "write init_addr %x\n", init_addr[i]);
			i++;


		}
		return i*4;
	}

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
	// Try to dynamically allocate a major number for the device -- more difficult but worth it
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber<0){
		printk(KERN_ALERT "Shared memory failed to register a major number\n");
		return majorNumber;
	}
	printk(KERN_INFO "Shared memory: registered correctly with major number %d\n", majorNumber);

	// Register the device class
	shmemClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(shmemClass)){
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(shmemClass);
	}
	printk(KERN_INFO "Shared memory: device class registered correctly\n");

   	// Register the device driver
	shmemDevice = device_create(shmemClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(shmemDevice)){
    	class_destroy(shmemClass);
    	unregister_chrdev(majorNumber, DEVICE_NAME);
    	printk(KERN_ALERT "Failed to create the device\n");
    	return PTR_ERR(shmemDevice);
	}

	printk(KERN_INFO "Shared memory successfully installed!! \n");
	return 0;
}

void cleanup_module(void)
{
	device_destroy(shmemClass, MKDEV(majorNumber, 0));
	class_unregister(shmemClass);
	class_destroy(shmemClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO "Shared memory successfully uninstalled :(\n");
}
