#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define PAGE_SIZE 4096
#define BUF_SIZE 512

size_t get_filesize(const char* filename);//get the size of the input file

int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd, ret;// the fd for the device and the fd for the input file
	size_t file_size, offset = 0, tmp;
	char file_name[50], method[20];
	char *kernel_address = NULL, *file_address = NULL;
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	void* mmapped;


	strcpy(file_name, argv[1]);
	strcpy(method, argv[2]);


	if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
	{
		perror("Cannot open /dev/master_device\n");
		return 1;
	}
	gettimeofday(&start ,NULL);
	if( (file_fd = open (file_name, O_RDWR)) < 0 )
	{
		perror("Cannot open input file\n");
		return 1;
	}

	if( (file_size = get_filesize(file_name)) < 0)
	{
		perror("Cannot get filesize\n");
		return 1;
	}


	if(ioctl(dev_fd, 0x12345677) == -1) //0x12345677 : create socket and accept the connection from the slave
	{
		perror("ioclt server creation error\n");
		return 1;
	}
	
	switch(method[0])
	{
		case 'f': //fcntl : read()/write()
			do
			{
				ret = read(file_fd, buf, sizeof(buf)); // read from the input file
				write(dev_fd, buf, ret);
			}while(ret > 0);
			break;
		case 'm': 
			mmapped = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, file_fd, 0);
			
			if(mmapped == MAP_FAILED){
				perror("Mmap error\n");
				return -1;
			}
			ret = file_size;
			do{
				if(ret < BUF_SIZE)
					write(dev_fd, mmapped + file_size - ret, ret);	
				else
					write(dev_fd, mmapped + file_size - ret, BUF_SIZE);					
				
				ret -= BUF_SIZE;
			}while(ret > 0);
			
			if(munmap(mmapped, file_size) == -1)
				perror("Unmap error\n");
	}

	if(ioctl(dev_fd, 0x12345679) == -1) // end sending data, close the connection
	{
		perror("ioclt server error on exit\n");
		return 1;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;

	printf("Master transmission time: %lf ms, File size: %d bytes \n", trans_time, file_size / 8);

	close(file_fd);
	close(dev_fd);

	return 0;
}

size_t get_filesize(const char* filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}
