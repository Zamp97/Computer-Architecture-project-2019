#include "hls_video.h"
#include <ap_axi_sdata.h>
#include <stdio.h>


#define IMG_WIDTH 1280
#define IMG_HEIGHT 720
#define KERNEL_DIM 3

typedef ap_axiu<32,1,1,1> uint_32_side_channel;
typedef hls::stream<uint_32_side_channel> axi_stream_32;

short  add(short, short);

using namespace hls;


void sobel(stream<uint_32_side_channel> &inStream, stream<uint_32_side_channel> &outStream) {

#pragma HLS INTERFACE axis port=inStream
#pragma HLS INTERFACE axis port=outStream
#pragma HLS INTERFACE ap_ctrl_none port=return

	LineBuffer<KERNEL_DIM,IMG_WIDTH,unsigned char> lineBuff;
	Window<KERNEL_DIM,KERNEL_DIM,unsigned char> window;

	int idxCol=0;
	int idxRow=0;
	int pixConvolved =0;

	uint_32_side_channel dataOutSideChannel;
	uint_32_side_channel currPixelSideChannel;

/*first row*/

	for(int w = 0; w< IMG_WIDTH/4; w++){
//#pragma HLS unroll factor = 100
		inStream >> currPixelSideChannel;
		unsigned int data = (unsigned int) (currPixelSideChannel.data);
		unsigned char val1 = (unsigned char) (data & 255);
		unsigned char val2 = (unsigned char) (data>>8 & 255);
		unsigned char val3 = (unsigned char) (data>>16 & 255);
		unsigned char val4 = (unsigned char) (data>>24 & 255);

		lineBuff.shift_up(4*w);
		lineBuff.insert_top(val1,4*w);
		lineBuff.shift_up(4*w +1);
		lineBuff.insert_top(val2,4*w +1);
		lineBuff.shift_up(4*w +2);
		lineBuff.insert_top(val3,4*w +2);
		lineBuff.shift_up(4*w+3);
		lineBuff.insert_top(val4,4*w +3);


		dataOutSideChannel.data = 0;
		dataOutSideChannel.keep = currPixelSideChannel.keep;
		dataOutSideChannel.strb = currPixelSideChannel.strb;
		dataOutSideChannel.user = currPixelSideChannel.user;
		dataOutSideChannel.last = currPixelSideChannel.last;
		dataOutSideChannel.id = currPixelSideChannel.id;
		dataOutSideChannel.dest = currPixelSideChannel.dest;

//		outStream.write(dataOutSideChannel);
		outStream << dataOutSideChannel;
	}

/*rest of rows*/

	for(int i = 0; i< IMG_HEIGHT -1; i++){
		for(int w = 0; w< IMG_WIDTH/4; w++){
//#pragma HLS unroll factor =50
//#pragma HLS pipeline
			inStream >> currPixelSideChannel;
					unsigned int data =(unsigned int) currPixelSideChannel.data;
					unsigned char val1 = (unsigned char) (data & 255);
					unsigned char val2 = (unsigned char) (data>>8 & 255);
					unsigned char val3 = (unsigned char) (data>>16 & 255);
					unsigned char val4 = (unsigned char) (data>>24 & 255);

					lineBuff.shift_up(4*w);
					lineBuff.insert_top(val1,4*w);
					lineBuff.shift_up(4*w +1);
					lineBuff.insert_top(val2,4*w +1);
					lineBuff.shift_up(4*w +2);
					lineBuff.insert_top(val3,4*w +2);
					lineBuff.shift_up(4*w+3);
					lineBuff.insert_top(val4,4*w +3);
		}

		for(int pix = 0; pix  < IMG_WIDTH/4; pix++){
//#pragma HLS unroll factor =50
//#pragma HLS pipeline
			unsigned int valout1 = 0;
			for(int j=0; j<4; j++) {
				window.shift_left();
				for(int wrow = 0; wrow < 3; wrow++){
						unsigned char val = lineBuff.getval(wrow,pixConvolved);
						window.insert(val,wrow,2);
				}

				if (i > 0 && pix >= 2) {
					//convolve
					short res1 = add(-window.getval(0,0), window.getval(0,2));
					short res2 =  2*add(-window.getval(1,0) , window.getval(1,2));
					short res3 = add(-window.getval(2,0) , window.getval(2,2));
					short sum1 = add(res1,res2);
					short result1 = sum1 + res3;
					if (result1 < 0) {
						result1 = 0;
					}
					valout1 = valout1<<8 | ((unsigned char) (result1/8));

					pixConvolved++;
				}
			}




			if(pixConvolved == 0 || pixConvolved == IMG_WIDTH -2){
				pixConvolved = 0;
			}
				dataOutSideChannel.data = valout1;
				dataOutSideChannel.keep = currPixelSideChannel.keep;
				dataOutSideChannel.strb = currPixelSideChannel.strb;
				dataOutSideChannel.user = currPixelSideChannel.user;
				dataOutSideChannel.last = pix == IMG_WIDTH/4 -1 ? 1 : 0;
				dataOutSideChannel.id = currPixelSideChannel.id;
				dataOutSideChannel.dest = currPixelSideChannel.dest;
					// Send to the stream (Block if the FIFO receiver is full)
//				outStream.write(dataOutSideChannel);
				outStream << dataOutSideChannel;
		}

	}
}

short add(short a, short b){
#pragma HLS inline
	return (short) a+b;
}

