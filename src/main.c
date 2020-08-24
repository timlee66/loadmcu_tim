/* Copyright (c) 2020, Nuvoton Corporation */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>		// exit
#include <getopt.h>
#include <stdint.h>
#include <fcntl.h>		// open
#include <stdbool.h>
#include <sys/time.h>
#include "mcu.h"
#include <sys/ioctl.h>	// ioctl

#define MCUIOC_BASE          'M'
#define MCU_RUNTEST          _IOW(MCUIOC_BASE, 9, unsigned int)

#define I2C_BIND_PATH   "/sys/bus/platform/drivers/nuvoton-i2c/bind"
#define I2C_UNBIND_PATH "/sys/bus/platform/drivers/nuvoton-i2c/unbind"
#define I2C_DEVICE_NAME "f008d000.i2c"

char *mcufw_path = NULL;
char *mcu_dev = NULL;
static MCU_Handler* mcu_handler = NULL;

//-----------------------------------------------------------------------------------------
int i2c_device_write(char *path, const void *ptr, size_t size){
	FILE *pFile;
	pFile = fopen(path,"w");
	if (pFile == NULL) {
		fprintf(stderr, "fopen fail:(%s)\n", path);
		return -1;
	}else if(size>0){
		fwrite(ptr, size, 1, pFile);
	}else{
		fprintf(stderr, "clear:(%s)\n", path);
	}
	fclose(pFile);
	return 0;
}
//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
void showUsage(char **argv)
{
	fprintf(stderr, "Usage: %s [option(s)]\n", argv[0]);
	fprintf(stderr, "  -d <device>   mcu device\n");
	fprintf(stderr, "  -s <filepath> load mcu file\n");
}
//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
void process_command_line(int argc, char **argv)
{
	int c = 0;
	int v;

	while ((c = getopt(argc, argv, "m:f:l:s:d:g")) != -1) {
		switch (c) {
			case 'd': {
				mcu_dev = malloc(strlen(optarg) + 1);
				strcpy(mcu_dev, optarg);
				break;
			}
			case 's': {
				mcufw_path = malloc(strlen(optarg) + 1);
				strcpy(mcufw_path, optarg);
				break;
			}
			default: {
				showUsage(argv);
				exit(EXIT_SUCCESS);
			}
		}
	}
	if (optind < argc) {
		fprintf(stderr, "invalid non-option argument(s)\n");
		showUsage(argv);
		exit(EXIT_SUCCESS);
	}
}
//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
MCU_Handler* MCU_initialize(void)
{
	MCU_Handler* state = (MCU_Handler*)malloc(sizeof(MCU_Handler));

	if (!state){
		fprintf(stderr, "MCU_initialize: return NULL\n");
		return NULL;
	}

	//fprintf(stderr, "MCU_initialize: mcu_open\n");
	state->MCU_driver_handle = open(mcu_dev, O_RDWR);
	if (state->MCU_driver_handle == -1) {
		fprintf(stderr, "MCU_initialize: Can't open %s\n", mcu_dev);
		goto err;
	}

	//fprintf(stderr, "MCU_initialize: i2c unbind\n");
	i2c_device_write(I2C_UNBIND_PATH, I2C_DEVICE_NAME, 16);

	return state;

err:
	free(state);
	//fprintf(stderr, "MCU_initialize: i2c bind\n");
	//i2c_device_write(I2C_BIND_PATH, I2C_DEVICE_NAME, 16);
	return NULL;
}
//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
void MemoryDumpB (__u32 Address, __u32 DisplayAddress, __16 NumOfLines)
{	
	__16 line;
	__u8 *pData8 = (__u8*)Address;
	__u8 index;
	__u8 Data8;

	for (line=0;line<NumOfLines;line++)
	{
		fprintf(stderr, "0x%08lX  ", DisplayAddress);
		DisplayAddress+=16;
		for (index=0;index<16;index++)
		{
			Data8 = *pData8++;
			fprintf(stderr, "0x%02X  ",Data8);
		}
		fprintf(stderr, "\n");
	}
}
//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
int handle_mcu_command(MCU_Handler* state, char *filename)
{
	static FILE *pFile;
	long lSize;
	char * buffer;
	size_t result;
	
	mcu_handler = state;

	pFile = fopen(filename , "rb");
	if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

	// obtain file size:
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);

	// allocate memory to contain the whole file:
	buffer = (char*) malloc (sizeof(char)*lSize);
	if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

	// copy the file into the buffer:
	result = fread (buffer,1,lSize,pFile);
	if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

	// aline BytesReceived to 32 bytes. 
	result = ((result+31)/32)*32;

	//MemoryDumpB ((__u32)buffer, 0, result/16);

	if (pwrite(mcu_handler->MCU_driver_handle, buffer, result, 0) < 0) {
		fprintf(stderr, "handle_mcu_command: pwrite FAIL\n");
	}

/*
	if (ioctl(state->MCU_driver_handle, MCU_RUNTEST, 1) < 0) {
		fprintf(stderr, "handle_mcu_command: MCU_RUNTEST FAIL\n");
	}
*/

	//fprintf(stderr, "handle_mcu_command: i2c bind\n");
	i2c_device_write(I2C_BIND_PATH, I2C_DEVICE_NAME, 16);

	// terminate
	fclose (pFile);
	free (buffer);
	return 0;
}
//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	struct  timeval start, end;
	unsigned long diff;

	process_command_line(argc, argv);
	if (!mcufw_path || !mcu_dev) {
		showUsage(argv);
		return 0;
	}

	mcu_handler = MCU_initialize();
	if (!mcu_handler) {
		fprintf(stderr, "Failed to initialize MCU\n");
		goto err;
	}

	//printf("MCU F/W Start Programming ........ \n");
	//gettimeofday(&start,NULL);
	handle_mcu_command(mcu_handler, mcufw_path);
	//gettimeofday(&end,NULL);
	//diff = 1000 * (end.tv_sec-start.tv_sec)+ (end.tv_usec-start.tv_usec) / 1000;
	//printf("MCU F/W End Programming .......... SUCCESS. It took %ld ms.\n",diff);

err:
	free(mcufw_path);
	free(mcu_dev);
	return 0;
}
