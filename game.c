#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include "./fpga_dot_font.h"

unsigned char quit = 0;
void user_signal1(int sig) { quit = 1; }

#define MAX_BUTTON 9
#define MAX_BUFF 32
#define MAX_DIGIT 4
#define FPGA_TEXT_LCD_DEVICE "/dev/fpga_text_lcd"
#define FND_DEVICE "/dev/fpga_fnd"
#define FPGA_DOT_DEVICE "/dev/fpga_dot"
#define BUZZER_DEVICE "/dev/fpga_buzzer"
#define LED_DEVICE "/dev/fpga_led"

int buzzer_dev;
int buzzer_count;
unsigned char buzzer_retval;
unsigned char buzzer_data;
int scale[9] = { 3822,3405,3034,2863,2551,2273,2025,1911,1703 };

void buzzer_play(int receive)
{
	for (buzzer_count = 0; buzzer_count*scale[receive] < 500 * 1000; buzzer_count++)
	{
		buzzer_data = 1;
		buzzer_retval = write(buzzer_dev, &buzzer_data, 1);
		usleep(scale[receive] / 2);
		buzzer_data = 0;
		buzzer_retval = write(buzzer_dev, &buzzer_data, 1);
		usleep(scale[receive] / 2);
	}
	buzzer_count = 0;
}

int main(void)
{
	int push_switch_dev, push_i;
	unsigned char push_sw_buf[MAX_BUTTON];

	int fnd_dev, str_size;
	unsigned char data[4];
	unsigned char retval;

	int text_lcd_dev;
	unsigned char text_lcd_buf[MAX_BUFF];

	int dev_dip;
	unsigned char dip_sw_buff = 0;
	unsigned char level = 0;

	int dev_dot, count, ran, num;
	int flash = 1;
	char save[32];

	int stage = 5;
	int compare = 0;
	int sec_count = 0;
	int stage_count = 0;
	int right = 0;
	int push = 1;
	int outstage = 1;
	int surpass = 1;

	int dev_led;
	unsigned char led_life = 7;

	push_switch_dev = open("/dev/fpga_push_switch", O_RDONLY);
	fnd_dev = open(FND_DEVICE, O_RDWR);
	text_lcd_dev = open(FPGA_TEXT_LCD_DEVICE, O_WRONLY);
	buzzer_dev = open(BUZZER_DEVICE, O_RDWR);
	dev_dip = open("/dev/fpga_dip_switch", O_RDWR);
	dev_led = open(LED_DEVICE, O_RDWR);

	memset(text_lcd_buf, ' ', MAX_BUFF);
	(void)signal(SIGINT, user_signal1);

	printf("Press <ctrl+c> to quit.\n");

	while (level == 0)
	{
		sprintf(text_lcd_buf, " select level    1, 2, 3        ");
		write(text_lcd_dev, text_lcd_buf, MAX_BUFF);

		usleep(400000);
		read(dev_dip, &dip_sw_buff, 1);
		level = dip_sw_buff;
	}

	sprintf(text_lcd_buf, " level selected  start           ");
	write(text_lcd_dev, text_lcd_buf, MAX_BUFF);

	usleep(400000);

	while (!quit)
	{
		write(dev_led, &led_life, 1);

		if (led_life == 0)
		{
			sprintf(text_lcd_buf, " game over...                   ");
			write(text_lcd_dev, text_lcd_buf, MAX_BUFF);

			buzzer_play(4);
			buzzer_play(2);
			buzzer_play(0);
			
			sleep(1);
			
			break;
		}

		if (surpass == 1)
		{
			surpass = 2;
			stage_count = 0;

			for (count = 0; count <= stage; count++)
			{
				dev_dot = open(FPGA_DOT_DEVICE, O_WRONLY);
				srand((unsigned int)time(NULL));

				if(level == 1)
				{
					ran = ((rand() % 2) + 2);
				}
				else if(level == 2)
				{
					ran = ((rand() % 4) + 1);
				}
				else if(level == 4)
				{
					ran = ((rand() % 9) + 1);
					while (ran == 5)
					{
						ran = ((rand() % 9) + 1);
					}
				}

				if (flash == 1)
				{
					if(level == 1 || level == 2)
					{
						ran = ran * 2;
					}
					flash = 0;
					save[stage_count] = ran;
					stage_count++;
				}
				else
				{
					ran = 0;
					flash = 1;
				}
				sleep(1);

				str_size = sizeof(fpga_number[ran]);
				write(dev_dot, fpga_number[ran], str_size);
				close(dev_dot);
				close(dev_dip);
			}
		}

		int startTime = 3;

		sprintf(text_lcd_buf, " game start      stage%d                ", outstage);
		write(text_lcd_dev, text_lcd_buf, MAX_BUFF);

		read(push_switch_dev, &push_sw_buf, sizeof(push_sw_buf));
		if (push_sw_buf[4] == 1)
		{
			while (startTime >= 0)
			{
				int sleeper = 20;
				push = 1;

				while (push == 1)
				{
					data[0] = right;
					data[1] = 0;
					data[2] = startTime / 10;
					data[3] = (startTime % 10);
					retval = write(fnd_dev, &data, 4);
					read(push_switch_dev, &push_sw_buf, sizeof(push_sw_buf));
					sleeper--;
					usleep(50000);

					if (startTime == 0)
					{
						led_life--;
						startTime = -1;
						surpass = 1;
						push = 0;
						data[0] = 0;
						data[1] = 0;
						data[2] = 0;
						data[3] = 0;
						retval = write(fnd_dev, &data, 4);
						read(push_switch_dev, &push_sw_buf, sizeof(push_sw_buf));
						sleep(1);
					}

					if (sleeper == 0)
					{
						sleeper = 20;
						startTime--;
					}

					for (push_i = 0; push_i <= 8; push_i++)
					{
						if ((push_i != 4) && push_sw_buf[push_i] == 1)
						{
							while (compare != (push_i + 1))
							{
								read(push_switch_dev, &push_sw_buf, sizeof(push_sw_buf));
								sleeper--;
								usleep(50000);
								data[2] = startTime / 10;
								data[3] = (startTime % 10);
								retval = write(fnd_dev, &data, 4);

								if (push_sw_buf[push_i] == 0)
								{
									buzzer_play(push_i);
									push = 0;
									compare = (push_i + 1);
									retval = write(fnd_dev, &data, 4);
								}

								if (sleeper == 0)
								{
									sleeper = 20;
									startTime--;
								}
							}
						}
					}
				}

				if (compare == save[sec_count])
				{
					push = 1;
					sec_count++;
					right++;
					compare = 0;
					data[0] = right;
					data[1] = 0;
					retval = write(fnd_dev, &data, 4);
					startTime = 3;
				}
				else
				{
					data[0] = 0;
					data[1] = 0;
					sec_count = 0;
					startTime = -1;
					right = 0;
					sprintf(text_lcd_buf, " try again...                    ");
					write(text_lcd_dev, text_lcd_buf, MAX_BUFF);
					sleep(1);

					surpass = 1;
					retval = write(fnd_dev, &data, 4);
					
					led_life = led_life >> 1;
					write(dev_led, &led_life, 1);
				}

				if (right == stage_count)
				{
					sprintf(text_lcd_buf, " stage %d clear!  next stage...  ", outstage);
					write(text_lcd_dev, text_lcd_buf, MAX_BUFF);
					outstage++;

					buzzer_play(0);
					buzzer_play(2);
					buzzer_play(4);

					sleep(1);

					sec_count = 0;
					startTime = -3;
					right = 0;
					data[2] = startTime / 10;
					data[3] = startTime % 10;
					retval = write(fnd_dev, &data, 4);
					surpass = 1;
					stage += 2;
				}
			}
		}
	}

	close(dev_led);
	close(buzzer_dev);
	close(text_lcd_dev);
	close(fnd_dev);
	close(push_switch_dev);

	return 0;

}
