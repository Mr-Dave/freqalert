/**  FreqAlert by MrDave        **/

#include "freqalert.h"
#include "parms.h"
#include "pa_devs.h"

int exit_app = 0;

config_parms_t parms;

float HammingWindow(int n__1,int N__2){
    return 0.54F - 0.46F * (float)(cos((2 * M_PI * n__1)) / (N__2 - 1));
}
void exec_cmd(char *pcmd){

printf("executing command %s \n",pcmd);

	if (pcmd)
	{
		pid_t pid;

		pid = fork();

		if (pid == -1)
		{
			//error_syslog ("ERROR: Failed to fork! %m");
			printf("**  Failed to fork\n");
		}
		else if (pid == 0)
		{
      if (-1 == execlp(pcmd, pcmd, (void *)NULL))
        //error_exit("Failed to start childprocess %s: %m", pcmd);
        printf("**  Failed to execute the command\n");
   	}
	}
}
void exec_background() {
  if (daemon(-1, -1) == -1)
    //error_exit("problem becoming daemon process");
    printf("**  Error becoming daemon\n");

}
long time_24hr(){
  /** This function returns a
   ** integer that is used in
   ** the alert criteria. The day of
   ** the week is 1-7 then a
   ** 24 hr HHMMSS.  This way
   ** simple > or < can be used.
   */

  time_t rawtime;
  struct tm *timeinfo;
  int ret_val;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  ret_val = ((timeinfo->tm_wday+1) * 1000000) +
            ((timeinfo->tm_hour)   * 10000) +
            ((timeinfo->tm_min)    * 100) +
            ((timeinfo->tm_sec));

  return (ret_val);

}
void print_terms(){
  /* Print out the terms   */
  printf(" \n");
  printf("freqalert 0.1.0, (C)2014 By MrDave \n");  
  printf(" \n");

  printf("THE SOFTWARE IS PROVIDED ""AS IS"", WITHOUT WARRANTY OF ANY KIND\n");
  printf("EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF \n");
  printf("MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. \n");
  printf("IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY \n");
  printf("CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, \n");
  printf("TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE \n");
  printf("SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n");

  printf(" \n");

}
void process_signal(int psignal) {
	if (psignal == SIGTERM)	exit_app = 1;
}
void show_help(void){

	printf( "\n"
			"Options: freqalert [options]\n"
			"-c<configfile>   Configfile to use\n"
			"-f               run in the background\n"
			"-s               Be silent\n"
			"-v               Be verbose\n"
			"-p               Be verbose and show frequencies detected\n"
			"-i               List devices information\n"
			"-h               This help text\n"
			);
}
void check_alerts(config_parms_t *parms,float *input_buffer,int *max_vol){
  /* Note that in this routine we want to do as few things as possible.  Rather
   * than add anything else, the goal is to move items out.  Right now, it is 
   * functional but with poor performance. 
   */ 
  /* The process in this routine is that we are sent in a
   * input buffer that is (sample rate * channels) long.
   * We go through that buffer in chunks (aka frames)
   * and determine the frequency of the sound in each of the
   * frames.  The smaller the frame, the greater the resolution
   * of the frequency and the greater the load on the cpu and vice versa.
   */

  long   num_items;
  float  freq_value=0;
  int    indx;  
  float pMaxIntensity;
  int   pMaxBinIndex;
  float pRealNbr;
  float pImaginaryNbr;
  float pIntensity;
  int   alert_triggered[parms->n_alerts - 1];

  /*  Initialize the trigger counts to zero */
  for (indx=0;indx < parms->n_alerts;indx++){
    alert_triggered[indx]=0;
  }

  num_items = parms->buf_items;
  //ff_sample is the number of bins, the window size is 
  //defined to be double the bins.
  //at 44100 sample rate and a 4096 N, then we
  //have 44100 / (4096 * 2) = 5.38hz maximum resolution
  while (num_items > FF_SAMPLE) {
    for (indx=0;indx < FF_SAMPLE;indx++){  
      parms->ff_in[indx][0] = input_buffer[indx] * HammingWindow(indx, FF_SAMPLE);      
    }
 
    fftw_execute(parms->ff_plan);
    
    pMaxIntensity = 0;
    pMaxBinIndex = 0;
  
    for (indx = parms->bin_min; indx <= parms->bin_max; indx++){
      pRealNbr = parms->ff_out[indx][0];
      pImaginaryNbr = parms->ff_out[indx][1];
      pIntensity = pRealNbr * pRealNbr + pImaginaryNbr * pImaginaryNbr;    
      if (pIntensity > pMaxIntensity){
        pMaxIntensity = pIntensity;
        pMaxBinIndex = indx;
      }
    }

    freq_value = (parms->bin_size * pMaxBinIndex * parms->channels);

    if (parms->verbosity > 2) printf("Frequency %.8f \n",freq_value);

    /*Now check to see if the freq is in the range for an event*/
    for (indx=0;indx < parms->n_alerts;indx++){
      //printf("checking %f  Low %f High %f ",freq_value,parms->alert_info[indx].freq_low,parms->alert_info[indx].freq_high);          
      if (freq_value >= parms->alert_info[indx].freq_low &&
          freq_value <= parms->alert_info[indx].freq_high) {
          alert_triggered[indx]++;
          //printf(" yes \n");
      } else {
        //printf(" no \n");
      }
    }
    num_items = num_items - FF_SAMPLE;
    input_buffer = input_buffer + FF_SAMPLE;
  }
  
  /*  Now go through and call the alerts triggered */
  for (indx=0;indx < parms->n_alerts;indx++) {
    if (alert_triggered[indx] >= parms->alert_info[indx].freq_duration ) {
      if (parms->verbosity > 1) printf("Triggering Frequency Alert %d  Duration: %d \n",indx,alert_triggered[indx]);
      parms->alert_info[indx].triggered_count=0 ;
      parms->alert_info[indx].triggered_count++ ;
      exec_cmd(parms->alert_info[indx].alert_event_cmd);
    }
  }
      
 }
int process_normal(){

  PaStream *pa_handle;
  PaError pa_retcd;
  PaStreamParameters pa_parms;

  int   max_vol =0;
  long  sr_ch = 0;
  float *input_buffer;
  int   threshold_count;
  int   indx;
  int   chkval;
  struct timespec time_start={0,0}, time_end={0,0};

  /*************************************************************/
  /** Initialize device.  This spews a lot of junk to console  */
  /*************************************************************/
  pa_retcd = Pa_Initialize ();
  if (pa_retcd < 0){
    printf("Error initializing device");
    return 1;
  }

  /*************************************************************/
  /** Print out the terms and config info after Pa_initialize  */
  /** has finished spewing all over the user                   */
  /*************************************************************/  
  print_terms();
  if (parms.verbosity > 0) printf("**  Starting FreqAlert\n");
  if (parms.verbosity > 0) parms_show_verbose(&parms);
  if (parms.verbosity > 0) printf("**  Device Initalized\n");

  /*************************************************************/
  /** open stream                                              */
  /*************************************************************/
  pa_parms.device = parms.device_number;
  pa_parms.channelCount = parms.channels;
  pa_parms.sampleFormat = paFloat32;
  pa_parms.suggestedLatency = Pa_GetDeviceInfo(parms.device_number)->defaultLowInputLatency;
  pa_parms.hostApiSpecificStreamInfo = NULL;

  pa_retcd = Pa_OpenStream (&pa_handle, &pa_parms, NULL,
                      parms.sample_rate,parms.frames_per_buffer , paNoFlag, NULL, NULL);
  // parms.sample_rate,paFramesPerBufferUnspecified , paNoFlag, NULL, NULL);
  //FramesPerBuffer is the number of samples * channels each read gets
                      
  if (pa_retcd < 0){
    printf("Error opening stream");
    return 1;
  }
  if (parms.verbosity > 0) printf("**  Stream open\n");

  /*************************************************************/
  /** Start Audio stream                                       */
  /*************************************************************/
  pa_retcd = Pa_StartStream (pa_handle);
  if (pa_retcd < 0){
    printf("Error starting stream");
    return 1;
  }
  if (parms.verbosity > 0) printf("**  Started stream\n");

  /*************************************************************/
  /** allocate and initialize the sound buffers                */
  /*************************************************************/
  sr_ch = parms.sample_rate * parms.channels;
  input_buffer    = malloc(sr_ch * sizeof(float));
  memset(input_buffer, 0x00,  sr_ch * sizeof(float));
  
  /*************************************************************/
  /** allocate and initialize the fftw plan and buffer         */
  /*************************************************************/
  if (parms.verbosity > 0) printf("**  Computing the fastest FFTW method \n");
  parms.ff_in   = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FF_SAMPLE);
  parms.ff_out  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FF_SAMPLE);
  parms.ff_plan = fftw_plan_dft_1d(FF_SAMPLE, parms.ff_in, parms.ff_out, FFTW_FORWARD, FFTW_MEASURE);
  for (indx=0;indx < FF_SAMPLE;indx++){        
      parms.ff_in[indx][1] = 0;
  }
  parms.bin_min = 1;
  parms.bin_max = FF_SAMPLE;
  parms.bin_size = ((float)parms.sample_rate / (float)FF_SAMPLE);
  parms.buf_items = parms.sample_rate * parms.channels;

  if (parms.verbosity > 0) printf("**  Got it! Lets roll \n");
  if (parms.verbosity > 0) printf("*****************************************\n");
  /*************************************************************/
  /** Main processing.                                         */
  /** Keep looping until signal sent to close which changes    */
  /** the exit_app to 1                                        */
  /*************************************************************/
  while (exit_app == 0)	{
    threshold_count = 0;
    max_vol=0;

    /** read sound into buffer.  The size of the
     ** buffer dictates how much is read
     ** paInputOverflowed means some data was skipped
     ** Since doing async processing is too much for
     ** this app, treat it like everything was ok
     */
    pa_retcd = Pa_ReadStream (pa_handle, input_buffer, parms.sample_rate);
    if (pa_retcd == paNoError  || pa_retcd == paInputOverflowed){

clock_gettime(CLOCK_MONOTONIC, &time_start);

      /* check audio-levels */
      for(indx=0; indx < sr_ch ; indx++) {
        chkval = abs(input_buffer[indx]*32768);
        if (chkval > max_vol ) max_vol = chkval ;
        if (chkval > parms.volume_level) threshold_count++;
      }

      if (threshold_count >= parms.volume_triggers) {
        if (parms.verbosity > 1)
          printf("Recording...Detected Level: %d  Threshold Level: %d \n",threshold_count,parms.volume_level);

        //check_alerts_normal(&parms,input_buffer,&max_vol);
        check_alerts(&parms,input_buffer,&max_vol);
        
       }
  clock_gettime(CLOCK_MONOTONIC, &time_end);
//  printf("frequency took about %.5f seconds\n",
//        (((double)time_end.tv_sec + 1.0e-9*time_end.tv_nsec) - 
//         ((double)time_start.tv_sec + 1.0e-9*time_start.tv_nsec))*10000);
       
    }
  }

  /*************************************************************/
  /** Clean up FFTW vars                                       */
  /*************************************************************/  
  fftw_destroy_plan(parms.ff_plan);
  fftw_free(parms.ff_in);
  fftw_free(parms.ff_out);
  
  if (parms.verbosity > 0) printf("**  Closing stream\n");
  pa_retcd = Pa_CloseStream (pa_handle);
  if (pa_retcd < 0) {
    printf("Error closing stream");
    return 1;
  }
  /*************************************************************/
  /** Terminate device                                         */
  /*************************************************************/
  pa_retcd = Pa_Terminate ();
  if (pa_retcd < 0) {
    printf("Error terminating stream");
    return 1;
  }

  return 0;

}
int main(int argc, char *argv[]){
  int ret_cd;

  //print_terms();

  parms_load(&parms,argc, argv);

  if (parms.show_help == 1)	{
    show_help();
    return 0;
  }

  if (parms.show_info == 1)	{
    list_devices();
    return 0;
  }

  signal(SIGTERM, process_signal);

  if (parms.run_in_background == 1) exec_background();

  ret_cd = process_normal();

  parms_free(&parms);
  

  return ret_cd ;

}

