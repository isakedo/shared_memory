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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Isak Edo Vivancos - 682405, Dariel Figueredo Piñero - 568659");
MODULE_DESCRIPTION("Gestión memoria compartida para los cores lx y baremetal");
MODULE_VERSION("0.0.06"); 

#define MEM_SIZE sizeof(u32)*1024

static void * init_addr;

int init_module(void)
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
*	int read(char* out_buffer, size_t content_size, int option)
*
* out_buffer: buffer de salida para el contenido copiado
* content_size: cantidad de contenido a copiar < 4MB
* option: seleccionar una opción de lectura(0 ,1)
*		0 lectura secuencial
* 	
* return (0,-1):
*		0 ok
*		-1 error
*/
int read(char* out_buffer, size_t content_size, int option)
{
	if(option == 0)
	//lee hasta content_size datos de memoria y los copia al buffer 
	{
		if(content_size <= MEM_SIZE)
		{
			for(int i = 0; i < content_size; i+4)
				out_buffer = init_addr + i;	
			return 0;
		}
		return -1;
	}
	else if(option == 1)
	{
		return -1;
	}
	else
	{
		return -1;
	}
}

void cleanup_module(void)
{
	kfree(init_addr);
}	

void * shared_address(void) 
{
	return init_addr;
}
