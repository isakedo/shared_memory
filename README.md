# Shared_Memory

### Sobre el Driver

Establece un área de memoria, reservada por dma, compartida entre los núcleos para cumplir la función de canal de comunicaciones.

#### Resumen funcionamiento del driver

La llamada open, además de devolver el descriptor, reserva memoria para DMA y esta es liberada en el proceso de close. Las llamas read/write transfieren datos sobre la memoria reservada leyendo y escribiendo entre 'kernel_space' y 'user_space'. Finalmente la llamada ioctl permite conocer la dirección física de inicio del espacio reservado.

![diagram](.\diagram.jpg)

### Sobre las direcciones físicas y virtuales

La dirección que devuelve ioctl es una dirección real dentro del espacio de "memoria físico" del sistema, dado que está pensada para que se le pase dicha dirección a una aplicación bare-metal. En dicho proceso es necesario, además de asegurar que es una dirección directamente mapeada, retirar el offset presente de base "PAGE_OFFSET. Para cumplir esta  se ha recurrido al macro ```__pa()```.

```c

	init_addr_pa = (off_t)__pa(init_addr_va);

```


### Funciones

```c
static struct file_operations shared_memory_smops=
{
	.read = dev_read,
	.write = dev_write,
	.open = dev_open,
	.release = dev_release,
	.unlocked_ioctl = dev_ioctl,
};
 dev_open(struct inode *inodep, struct file *filep);
 dev_release(struct inode *inodep, struct file *filep);
 dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset);
 dev_write(struct file *filep, char *buffer, size_t len, loff_t *offset);
 dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);
```

##### dev_open:

La función `dev_open`  se encarga de reservar la memoria compartida. Es importante llamar a close(dev_release) para liberar dicha memoria.

Se utiliza `kmalloc` con la opción `__GFP_DMA` para reservar el espacio de memoria necesario. Inicialmente se reservan 4KB, hasta el máximo permitido de 16MB. En caso de fallar la función devuelve -1.

##### dev_release:

La función `dev_release` es llamada en el proceso de 'close' y se encarga de liberar el espacio de memoria reservado utilizando `kfree`.

##### dev_read:

La función `dev_read` encapsula la lectura del contenido almacenado en la memoria compartida.

Los datos a leer deben ser copiados del 'kernel space' al 'user_space' para lo cual se utiliza la función copy_to_user que simplifica el proceso. Por otro lado se requieren los parámetros usuales en este tipo de operación: un buffer destino para almacenar los datos leídos y el número de bytes a leer. En cualquier caso se verifica que no se exceda el número de bytes reservados en la lectura, si falla devuelve -1.  

##### dev_write:

La función `dev_write` encapsula la escritura de los datos a la memoria compartida.

Los bytes a escribir en la memoria reservada deben ser copiados de 'user_space' a 'kernel_space' para esto se utiliza la función copy_from_user. La función requiere un buffer con los datos, y el número de bytes a escribir. En cualquier caso se verifica que no se exceda el número de bytes reservados en la escritura, y si falla devuelve -1.

##### dev_ioctl:

La función `dev_ioctl` se utiliza para comunicarse más allá del simple read/write. En este caso se utiliza para informar de la dirección de inicio de la zona reservada.
