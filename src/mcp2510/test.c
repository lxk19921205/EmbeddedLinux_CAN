#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>

#include "mcp2510.h"

//#define DEVICE_NAME "/dev/candev"
#define DEVICE_NAME "/dev/tmpdev"


char buf[111];
int dev;

typedef void (*sighandler_t)(int);

void sig_usr()//接收到信号后执行的函数
{	
	int count = 0;
	int i = 0;

	signal(SIGIO,sig_usr);	//继续接收信号
	printf("----receive signal, in sig_usr()------\n");
	while ((count = read(dev, buf, 8)) != -1)//读取接收到的数据
	{
		printf("read count = %d \n", count);
	}

	for(i = 0; i < count; i++)
	{
//		printf("buf[%d] is %x\n",i,buf[i]);
	}

//	signal(SIGIO, sig_usr);	//继续接收信号
}

int main()
{
	unsigned char mode = -1;

	sighandler_t prev_handler = NULL;

	//输入发送数据
	char TXdata[16];
	TXdata[0] =  0x0f;	//to be modified
	TXdata[1] =  0x0f;
	TXdata[2] =  0x0e;
	TXdata[3] =  0x00;

	TXdata[4] =  0x04;
	TXdata[5] =  0x05;
	TXdata[6] =  0x06;
	TXdata[7] =  0x07;
	TXdata[8] =  0x08;
	TXdata[9] =  0x09;
	TXdata[10]=  0x0a;
	TXdata[11]=  0x0b;

	//dlc
	TXdata[12]=  0x06;
	//isExt
	TXdata[13]=  0x00;
	//rxRTR
	TXdata[14]=  0x00;
	//padbyte
	TXdata[15]=  0x00;

	printf("prepare to open dev\n");
	prev_handler = signal(SIGIO, sig_usr);//等待信号
	if (prev_handler == SIG_ERR)
	{
		printf("returns SIG_ERR, signal failed\n");
	}
	else
	{
		printf("signal successed\n");
	}

	dev = open(DEVICE_NAME, O_RDWR);
	if(dev>=0)
	{
		int oflag;
		int char_exit = '\0';
		unsigned char to_mode = OP_NORMAL;
		unsigned char mode = -1;
		int i = 0;
		int count = 0;

		to_mode = OP_LOOPBACK;
		
		printf("preparing to set loopback: %d\n", OP_LOOPBACK);
		ioctl(dev, IOCTL_GET_MODE, &mode);
		printf("current mode(to set loopback) is %d, by Andriy\n", mode);

		//设置为loopback模式
		if (ioctl(dev, IOCTL_MOD_SET, &to_mode) == 1)
		{
			printf("set loopback, ioctl success\n");
		}
		else
		{
			printf("set loopback, ioctl returns not 1, so failed\n");
		}

		ioctl(dev, IOCTL_GET_MODE, &mode);
		printf("current mode(mode set) is %d, by Andriy\n", mode);

		fcntl(dev, F_SETOWN, getpid());//将用户进程号写到设备文件中，让驱动发送信号到用户进程
		oflag = fcntl(dev, F_GETFL);
		fcntl(dev, F_SETFL, oflag|FASYNC);

		printf("preparing to getchar()\n");
		char_exit = getchar();
		while(char_exit != 'q')
		{
			if (char_exit == '\n' || char_exit == '\r')
			{
			}
			else
			{
				i = write(dev, TXdata, 16);	//发送数据
				printf("write result: %d\n", i);
				if (i == 1)
				{
					//getchar();
					//kill(getpid(), SIGIO);
				}
			}

			printf("preparing to getchar()\n");
			char_exit = getchar();
		}
	}
	else
	{
		printf("Open failed !\n");
	}

	if (prev_handler != SIG_ERR)
	{
		//if last time it didn't return an error, restore the previous handler
		signal(SIGIO, prev_handler);
	}
	close(dev);
	return 0; 
}
