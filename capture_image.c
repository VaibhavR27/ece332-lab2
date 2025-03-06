// #include "address_map_arm.h"

#define KEY_BASE              0xFF200050
#define VIDEO_IN_BASE         0xFF203060
#define FPGA_ONCHIP_BASE      0xC8000000

/* This program demonstrates the use of the D5M camera with the DE1-SoC Board
 * It performs the following: 
 * 	1. Capture one frame of video when any key is pressed.
 * 	2. Display the captured frame when any key is pressed.		  
*/
/* Note: Set the switches SW1 and SW2 to high and rest of the switches to low for correct exposure timing while compiling and the loading the program in the Altera Monitor program.
*/
int main(void){
	volatile int * KEY_ptr		= (int *) KEY_BASE;
	volatile int * Video_In_DMA_ptr	= (int *) VIDEO_IN_BASE;
	volatile short * Video_Mem_ptr	= (short *) FPGA_ONCHIP_BASE;

	int x, y;
	int image_counter = 0;

	*(Video_In_DMA_ptr + 3)	= 0x4;					// Enable the video

	while (1)
	{
		if (*KEY_ptr != 0)					// check if any KEY was pressed
		{
			*(Video_In_DMA_ptr + 3) = 0x0;			// Disable the video to capture one frame
			image_counter++;
			while (*KEY_ptr != 0);				// wait for pushbutton KEY release
			break;
		}
	}

	while (1)
	{
		if (*KEY_ptr != 0)					// check if any KEY was pressed
		{
			break;
		}
	}

	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			short temp2 = *(Video_Mem_ptr + (y << 9) + x);
			*(Video_Mem_ptr + (y << 9) + x) = temp2;
		}
	}
}


/*
Method to take an input image and flip image
TODO: currently flips image across both axes, check to see what flip is desired for final submission
*/
void flip_image(int* image_pointer, int image_width, int image_height){
	for(int y = 0; y < image_height; y++){
		for(int x = 0; x < image_width; x++){
			int y_flipped = image_height - y;	// either keep this or next line
			int x_flipped = image_width - x;	// either keep this or the y flipped
			short temp = *(image_pointer + (y_flipped << 9) + x_flipped);
			*(image_pointer + (y_flipped << 9) + x_flipped) = temp;
		}
	}
}


/*
Method to take an input image and mirror image
TODO: currently doubles width of original image, mirrors across right edge of image
*/
void mirror_image(int* original_image_pointer, int image_width, int image_height, int* new_image_pointer){
	int new_width = image_width * 2;
	int new_height = image_height * 2;
	// new_image_pointer should have a copy of original_image_pointer in it's first half
	// maybe set values for new_image_pointer in this method??
	for(int y = 0; y < image_height; y++){
		for(int x = 0; x < image_width; x++){
			int x_mirror = new_width - x;
			short temp = *(original_image_pointer + (y << 9) + x_mirror);
			*(original_image_pointer + (y << 9) + x_mirror) = temp;
		}
	}
}

