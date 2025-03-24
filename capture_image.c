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




/*
Method to convert a color image to grayscale.
Instead of converting to a binary (black and white) image using a threshold,
this version calculates the luminance of each pixel and maps it to a grayscale value.
The grayscale value is computed in the RGB565 format, which gives a smooth variation
of brightness levels.
*/
void color2blackAndWhite(unsigned short* image_pointer, int image_width, int image_height) {
    int x, y;
    for (y = 0; y < image_height; y++) {
        for (x = 0; x < image_width; x++) {
            int offset = (y << 9) + x;  // Assumes a row stride of 512 pixels
            unsigned short pixel = image_pointer[offset];

            // Extract the individual RGB components from the 16-bit RGB565 pixel.
            unsigned short red   = (pixel >> 11) & 0x1F;  // 5 bits for red
            unsigned short green = (pixel >> 5)  & 0x3F;  // 6 bits for green
            unsigned short blue  =  pixel        & 0x1F;  // 5 bits for blue

            // Convert each component to an 8-bit value.
            // This scales red and blue from [0,31] to [0,255] and green from [0,63] to [0,255].
            unsigned int r8 = (red   * 255) / 31;
            unsigned int g8 = (green * 255) / 63;
            unsigned int b8 = (blue  * 255) / 31;

            // Compute the luminance (brightness) using standard coefficients.
            // The result is an 8-bit grayscale value in the range [0,255].
            unsigned int luminance = (r8 * 299 + g8 * 587 + b8 * 114) / 1000;

            // Map the 8-bit luminance value back to the RGB565 format.
            // For red and blue, scale from [0,255] to [0,31]; for green, scale to [0,63].
            unsigned short grayRed   = (luminance * 31) / 255;
            unsigned short grayGreen = (luminance * 63) / 255;
            unsigned short grayBlue  = (luminance * 31) / 255;
            unsigned short grayPixel = (grayRed << 11) | (grayGreen << 5) | grayBlue;

            image_pointer[offset] = grayPixel;
        }
    }
}
