
#ifndef QRSFILT_H
#define QRSFILT_H

#define MS_PER_SAMPLE	8  //( (double) 1000/ (double) SAMPLERATE)
#define MS25	3  //((int) (25/MS_PER_SAMPLE + 0.5))
#define LPBUFFER_LGTH 6 //((int) (2*MS25))
#define MS125	16 //((int) (125/MS_PER_SAMPLE + 0.5))
#define HPBUFFER_LGTH 17 //MS125
#define MS10	1  //((int) (10/ MS_PER_SAMPLE + 0.5))
#define DERIV_LENGTH	MS10
#define MS80	10  //((int) (80/MS_PER_SAMPLE + 0.5))
#define WINDOW_WIDTH	MS80 // Moving window integration width.


extern int QRSFilter(int datum,int init);

extern int deriv1( int x0, int init );

#endif