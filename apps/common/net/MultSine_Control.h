#ifndef MultSine_Control_h__
#define MultSine_Control_h__

typedef struct MultSine_Control_st MultSine_Control;
//detect_nfreq: Total number of frequency for detecting;
//analysis_npoint: Every n point input then output a result of detection
int MultSine_Control_QuaryBufSize(int detect_nfreq, int analysis_npoint);
//
//ms_con = malloc(MultSine_Control_QuaryBufSize(detect_nfreq,analysis_npoint));
//detect_nfreq: The same with arguments in MultSine_Control_QuaryBufSize
//analysis_npoint: The same with arguments in MultSine_Control_QuaryBufSize
//percode_nfreq: In a code,how much Frequency is using
//Dectect_Threshold: A Threshold for dectection (  Dectect_Threshold = log2(x)*2^16 ),Usually set to 3000
//code_post: A function to get the result of detection,detect result is in bin,get it by bit
//coeff: A table for calculation. Matlab code:  table(i) = round( 2*cos(2*pi*freq_target(i)/fs)*2^21 ); freq_target(i) subject to  freq_target(i) = k/analysis_npoint*fs; k must integer;
//nChannel: how many channel in input data
void MultSine_Control_init(MultSine_Control *ms_con, int detect_nfreq, int analysis_npoint, int percode_nfreq, int Dectect_Threshold, void (*code_post)(int), const int *coeff, int nchannel);
//ms_con: the arguments in MultSine_Control_init
//data: PCM data(Stereo)
//npoint: number of point in data per channel
void MultSine_Control_run(MultSine_Control *ms_con, short *data, int npoint);
/*
const int Target_Freq_Coeff_48k[11] =
{
    -3791606, //20625Hz
    -3746454, //2.043750e+004Hz
    -3699046, //20250Hz
    -3649409, //2.006250e+004Hz
    -3597575, //19875Hz
    -3543573, //1.968750e+004Hz
    -3487436, //19500Hz
    -3429199, //1.931250e+004Hz
    -3368897, //19125Hz
    -3306565, //1.893750e+004Hz
    -3242241, //18750Hz
};

const int Target_Freq_Coeff_41k[11] =
{
    -4108155, //20625Hz
    -4084099, //2.043750e+004Hz
    -4057129, //20250Hz
    -4027263, //2.006250e+004Hz
    -3994524, //19875Hz
    -3958934, //1.968750e+004Hz
    -3920518, //19500Hz
    -3879306, //1.931250e+004Hz
    -3835325, //19125Hz
    -3788606, //1.893750e+004Hz
    -3739185, //18750Hz
};
Template:
void code_post(int code)
{
    if( code != 0)
        printf("%03x\n",code);
}
void MultSine_Init()
{
    MultSine_Control *ms_con = malloc(MultSine_Control_QuaryBufSize(11,256));
    MultSine_Control_init(ms_con,11,256,3,3000,code_post,Target_Freq_Coeff_48k);
}
void dac_write(u8 *buf ,u32 len)
{
    u8 err ;
    u32 wlen;
    MultSine_Control_run(ms_con,buf,len>>2); //Call In dac_write
    while(1)
    {
        wlen = cbuf_write(&audio_cb,buf,len);
        len -= wlen;
        if (len == 0) break;
        buf += wlen;
        err = os_sem_pend(&audio_sem,0) ;
        if(err != OS_NO_ERR)
        {

        }
    }
}

*/
#endif // MultSine_Control_h__
