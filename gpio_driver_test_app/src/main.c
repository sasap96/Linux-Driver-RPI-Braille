#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#define RING_SIZE 2000

static int size=1;
static int counter;
static int file_desc;
pthread_mutex_t fileLock;

struct RingBuffer
{
    unsigned int  tail;
    unsigned int  head;
    unsigned char data[RING_SIZE];
};

/* Operacije za rad sa kruznim baferom. */
char ringBufGetChar (struct RingBuffer *apBuffer)
{
    int index;
    index = apBuffer->head;
    apBuffer->head = (apBuffer->head + 1) % RING_SIZE;
    return apBuffer->data[index];
}

void ringBufPutChar (struct RingBuffer *apBuffer, const char c){

    apBuffer->data[apBuffer->tail] = c;
    apBuffer->tail = (apBuffer->tail + 1) % RING_SIZE;
}


void* writer (void *param)
{
	
	FILE *fptr;
	struct RingBuffer buffer;
	buffer.tail=0;
	buffer.head=0;
	char letter;

	if((fptr=fopen("/home/knjiga","r"))==NULL)
	{
		printf("Nije moguce otvoriti fajl");
		return 0;
	}

	fseek(fptr, 0L, SEEK_END);
	size = ftell(fptr);
	fseek(fptr,0L,SEEK_SET);
	
	counter=20;

	while(size)
	{
		letter=getc(fptr);
		ringBufPutChar(&buffer,letter);
		if(counter)
		{
			letter=ringBufGetChar(&buffer);
			pthread_mutex_lock(&fileLock);
			write(file_desc,&letter,1);
			counter--;
			pthread_mutex_unlock(&fileLock);
			size--;
		}
	}
	fclose(fptr);

	return 0;
}

void* reader (void *param)
{

	while(1)
    {
    	pthread_mutex_lock(&fileLock);
        read(file_desc,&counter,4);
        pthread_mutex_unlock(&fileLock);
		if(size==0)
		{
			break;
		}    
    }
    return 0;
}
int main(void)
{
	 file_desc = open("/dev/gpio_driver", O_RDWR);

    if(file_desc < 0)
    {
        printf("'/dev/gpio_driver' device isn't open\n");
        printf("Try:\t1) Check does '/dev/gpio_driver' node exist\n\t2)'chmod 666 /dev/ \
               gpio_driver'\n\t3) ""insmod"" gpio_driver module\n");
				return 0;
    }
    printf("'/dev/gpio_driver' device is successfully opened!\n");	


	pthread_t hWriter;
    pthread_t hReader;
    pthread_mutex_init(&fileLock,NULL);
    
	pthread_create(&hReader, NULL, reader, 0);
	pthread_create(&hWriter, NULL, writer, 0);
   

    /* Cekanje na zavrsetak formiranih niti. */
    pthread_join(hReader, NULL);
    pthread_join(hWriter, NULL);
	pthread_mutex_destroy(&fileLock);
	
	return 0;
}
