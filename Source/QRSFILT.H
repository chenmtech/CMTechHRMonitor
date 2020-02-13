
#ifndef QRSFILT_H
#define QRSFILT_H

#define SAMPLE_RATE	200	/* Sample rate in Hz. */
#define MS_PER_SAMPLE	8  //( (double) 1000/ (double) SAMPLE_RATE)
#define MS10	1  //((int) (10/ MS_PER_SAMPLE + 0.5))
#define MS25	3  //((int) (25/MS_PER_SAMPLE + 0.5))
#define MS30	4  //((int) (30/MS_PER_SAMPLE + 0.5))
#define MS80	10  //((int) (80/MS_PER_SAMPLE + 0.5))
#define MS95	12  //((int) (95/MS_PER_SAMPLE + 0.5))
#define MS100	13  //((int) (100/MS_PER_SAMPLE + 0.5))
#define MS125	16 //((int) (125/MS_PER_SAMPLE + 0.5))
#define MS150	19 //((int) (150/MS_PER_SAMPLE + 0.5))
#define MS160	20 //((int) (160/MS_PER_SAMPLE + 0.5))
#define MS175	22 //((int) (175/MS_PER_SAMPLE + 0.5))
#define MS195	24 //((int) (195/MS_PER_SAMPLE + 0.5))
#define MS200	25 //((int) (200/MS_PER_SAMPLE + 0.5))
#define MS220	28 //((int) (220/MS_PER_SAMPLE + 0.5))
#define MS250	31 //((int) (250/MS_PER_SAMPLE + 0.5))
#define MS300	38 //((int) (300/MS_PER_SAMPLE + 0.5))
#define MS360	45 //((int) (360/MS_PER_SAMPLE + 0.5))
#define MS450	56 //((int) (450/MS_PER_SAMPLE + 0.5))
#define MS1000	SAMPLE_RATE
#define MS1500	188 //((int) (1500/MS_PER_SAMPLE))
#define DERIV_LENGTH	MS10
#define LPBUFFER_LGTH 6 //((int) (2*MS25))
#define HPBUFFER_LGTH MS125

#define WINDOW_WIDTH	MS80			// Moving window integration width.


extern int QRSFilter(int datum,int init);

extern int deriv1( int x0, int init );

#endif