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

//=====================================
// constantes
#define MEM_SIZE (sizeof(u32)*1024)

//=====================================
// variables globales
static int iterador = 0;//iterador para lecturas a trozos
static void * init_addr;


//=====================================
// funciones

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
void * shared_address(void) 
{
	return init_addr;
}


/*
*	int read(char* out_buffer, size_t content_size, int option)
*
* out_buffer: buffer de salida para el contenido copiado
* content_size: cantidad de contenido a copiar < 4MB
* option: seleccionar una opción de lectura(0 ,1)
*		0 lectura secuencial, lee desde inicio hasta inicio+content_size
* 	1	lectura por trozos, lee desde inicio+iterador hasta inicio+iterador+content_size
				el iterador avanza de 4 en 4
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
			int i = 0;
			while(i < content_size)
			{
				out_buffer = init_addr + i;
				i+=4;
			}
			return 0;
		}
		return -1;
	}
	else if(option == 1)
	{
		if((iterador + content_size) <= MEM_SIZE)
		{
			int i = iterador;
			while(i<iterador+content_size)
			{
				out_buffer = int_addr + i;
				i+=4;
			}
			iterador = i;
			return 0;
		}
		return -1;
	}
	else
	{
		return -1;
	}
}

/*
* pone a 0 el iterador de lectura
*/
void reset_iterator(void)
{
	iterador = 0;
}

/*
* devuelve la diferencia entre final-iterador, es decir el espacio que queda por leer
*/
int size_iterator_end(void)
{
	return (int)MEM_SIZE - iterador;
}



void cleanup_module(void)
{
	kfree(init_addr);
}	


