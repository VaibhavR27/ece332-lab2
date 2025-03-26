// #include "address_map_arm.h"
#include <time.h>
#include <stdio.h>

#define KEY_BASE 0xFF200050
#define VIDEO_IN_BASE 0xFF203060
#define FPGA_ONCHIP_BASE 0xC8000000
#define SW_BASE 0xFF200040
#define STRIDE 512
#define HEIGHT 240
#define WIDTH 320
#define CHAR_BASE 0xC9000000

// Global backup buffer
unsigned short image_backup[HEIGHT][STRIDE];
int grayscale_applied = 0;

volatile int *KEY_ptr = (int *)KEY_BASE;
volatile int *Video_In_DMA_ptr = (int *)VIDEO_IN_BASE;
volatile short *Video_Mem_ptr = (short *)FPGA_ONCHIP_BASE;
volatile int *SW_ptr = (int *)SW_BASE;
volatile short *Char_ptr = (short *)CHAR_BASE;


void flip_image(unsigned short *image_pointer, int image_width, int image_height)
{
    const int stride = 512; // D5M frame buffer stride in on-chip memory
    int x, y;
    for (y = 0; y < image_height; y++)
    {
        for (x = 0; x < image_width / 2; x++)
        {
            int left_index = y * stride + x;
            int right_index = y * stride + (image_width - 1 - x);

            // Swap pixels
            unsigned short temp = image_pointer[left_index];
            image_pointer[left_index] = image_pointer[right_index];
            image_pointer[right_index] = temp;
        }
    }
}

void mirror_image(unsigned short *image_pointer, int image_width, int image_height)
{
    int x, y;
    const int stride = 512; // D5M frame buffer row stride

    for (y = 0; y < image_height / 2; y++)
    {
        for (x = 0; x < image_width; x++)
        {
            int top_index = y * stride + x;
            int bottom_index = (image_height - 1 - y) * stride + x;

            // Swap pixels between top and bottom rows
            unsigned short temp = image_pointer[top_index];
            image_pointer[top_index] = image_pointer[bottom_index];
            image_pointer[bottom_index] = temp;
        }
    }
}

void color2blackAndWhite(unsigned short *image_pointer, int image_width, int image_height, int restore_original)
{
    int x, y;

    if (restore_original)
    {
        // Restore original color image from backup
        for (y = 0; y < image_height; y++)
        {
            for (x = 0; x < image_width; x++)
            {
                image_pointer[y * STRIDE + x] = image_backup[y][x];
            }
        }
        grayscale_applied = 0;
        return;
    }

    // Back up original image before modifying
    for (y = 0; y < image_height; y++)
    {
        for (x = 0; x < image_width; x++)
        {
            image_backup[y][x] = image_pointer[y * STRIDE + x];
        }
    }

    // Convert to grayscale
    for (y = 0; y < image_height; y++)
    {
        for (x = 0; x < image_width; x++)
        {
            int offset = y * STRIDE + x;
            unsigned short pixel = image_pointer[offset];

            unsigned short red = (pixel >> 11) & 0x1F;
            unsigned short green = (pixel >> 5) & 0x3F;
            unsigned short blue = pixel & 0x1F;

            unsigned int r8 = (red * 255) / 31;
            unsigned int g8 = (green * 255) / 63;
            unsigned int b8 = (blue * 255) / 31;

            unsigned int luminance = (r8 * 299 + g8 * 587 + b8 * 114) / 1000;

            unsigned short grayRed = (luminance * 31) / 255;
            unsigned short grayGreen = (luminance * 63) / 255;
            unsigned short grayBlue = (luminance * 31) / 255;

            unsigned short grayPixel = (grayRed << 11) | (grayGreen << 5) | grayBlue;
            image_pointer[offset] = grayPixel;
        }
    }

    grayscale_applied = 1;
}

void invert_black_and_white(unsigned short *image_pointer, int image_width, int image_height)
{
    int x, y;
    for (y = 0; y < image_height; y++)
    {
        for (x = 0; x < image_width; x++)
        {
            int offset = (y << 9) + x; // Assumes a row stride of 512 pixels
            unsigned short pixel = image_pointer[offset];

            // Extract the RGB components from the 16-bit RGB565 pixel.
            unsigned short red = (pixel >> 11) & 0x1F;  // 5 bits for red
            unsigned short green = (pixel >> 5) & 0x3F; // 6 bits for green
            unsigned short blue = pixel & 0x1F;         // 5 bits for blue

            // Invert each component relative to its maximum.
            // For red and blue, the maximum value is 31; for green, it's 63.
            unsigned short inv_red = 31 - red;
            unsigned short inv_green = 63 - green;
            unsigned short inv_blue = 31 - blue;

            // Combine the inverted components back into a 16-bit RGB565 pixel.
            unsigned short inv_pixel = (inv_red << 11) | (inv_green << 5) | inv_blue;

            image_pointer[offset] = inv_pixel;
        }
    }
}

/* This program demonstrates the use of the D5M camera with the DE1-SoC Board
 * It performs the following:
 * 1. Capture one frame of video when any key is pressed.
 * 2. Display the captured frame when any key is pressed.
 * TODO: for all methods, in order to do image processing on entire frame, every method should save the entire folder r
 */
/* Note: Set the switches SW1 and SW2 to high and rest of the switches to low for correct exposure timing while compiling and the loading the program in the Altera Monitor program.
 */
int main(void)
{
    int image_width = 320;
    int image_height = 240;

    int image_counter = 0;

    *(Video_In_DMA_ptr + 3) = 0x4; // Enable video

    while (1)
    {
        if (*KEY_ptr != 0)
        {
            printf("Key Pressed! Capturing image...\n");
            *(Video_In_DMA_ptr + 3) = 0x0; // Disable video input to capture the frame

            while (*KEY_ptr); // Wait for key release to prevent multiple triggers

            image_counter++;

            time_t rawtime;
            struct tm *info;
            char buffer[80];
            time(&rawtime);
            info = localtime(&rawtime);
            snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d Img: %d", 
                info->tm_year + 1900, info->tm_mon + 1, info->tm_mday,
                info->tm_hour, info->tm_min, info->tm_sec, image_counter);

            printf("Current date and time: %s\n", buffer);
            printf("Image number: %d\n", image_counter);
            // coordinates of text on image
	    	int a = 2;
            int b = 2;
            
            char* text_ptr = buffer;
            int offset = (a << 7) + b;
            while(*(text_ptr)) {
                *(volatile char *) (0xC9000000 + offset) = *(text_ptr); // write to the character buffer 
                ++text_ptr; 
                ++offset;
            }
		
            int prev_switch_0 = (int)((*SW_ptr) & (1));
            int prev_switch_1 = (int)((*SW_ptr) & (1 << 1));
            int prev_switch_2 = (int)((*SW_ptr) & (1 << 2));
            int prev_switch_3 = (int)((*SW_ptr) & (1 << 3));

            int gray_scale_toggle = 0;

            while (1)
            {
                int cur_switch_0 = (*SW_ptr) & (1);
                int cur_switch_1 = (*SW_ptr) & (1 << 1);
                int cur_switch_2 = (*SW_ptr) & (1 << 2);
                int cur_switch_3 = (*SW_ptr) & (1 << 3);
                // check if switch 0 was flipped
                if (cur_switch_0 != prev_switch_0)
                {
                    flip_image((unsigned short *)Video_Mem_ptr, image_width, image_height);
                    prev_switch_0 = cur_switch_0;
                }
                // check if switch 1 was flipped
                if (cur_switch_1 != prev_switch_1)
                {
                    mirror_image((unsigned short *)Video_Mem_ptr, image_width, image_height);
                    prev_switch_1 = cur_switch_1;
                }
                // check if switch 2 was flipped
                if (cur_switch_2 != prev_switch_2)
                {
                    color2blackAndWhite((unsigned short *)Video_Mem_ptr, image_width, image_height, (gray_scale_toggle) % 2);
                    gray_scale_toggle++;
                    prev_switch_2 = cur_switch_2;
                }
                // check if switch 3 was flipped
                if (cur_switch_3 != prev_switch_3)
                {
                    invert_black_and_white((unsigned short *)Video_Mem_ptr, image_width, image_height);
                    prev_switch_3 = cur_switch_3;
                }

                if (*KEY_ptr != 0)
                { // return to video
                    printf("Resetting video...\n");
                    *(Video_In_DMA_ptr + 3) = 0x0; // Full reset
                    *(Video_In_DMA_ptr + 3) = 0x4; // Re-enable video
                    printf("Ready for next capture...\n");
                    break;
                }
            }
        }
    }
}