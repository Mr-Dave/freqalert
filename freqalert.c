/*FreqAlert by MrDave
 *  Many parts based upon aplay.c from ALSA project
 * arecord --device=hw:1,0 --format=S16_LE --rate=44100 -d 10 ./test.wav
 */

#include "freqalert.h"
#include "parms.h"


int exit_app;

static float HammingWindow(int n__1,int N__2){
    return 0.54F - 0.46F * (float)(cos((2 * M_PI * n__1)) / (N__2 - 1));
}

static void exec_cmd(char *pcmd){

    pid_t pid;

    printf("executing command %s \n",pcmd);

    if (pcmd)	{
        pid = fork();
        if (pid == -1) {
            printf("**  Failed to fork\n");
        } else if (pid == 0) {
            if (execlp(pcmd, pcmd, (void *)NULL) == -1){
                printf("**  Failed to execute the command\n");
            }
        }
    }
}

static void exec_background(void) {

    if (daemon(-1, -1) == -1){
        printf("**  Error becoming daemon\n");
    }

}

static void print_terms(void){
    /* Print out the terms   */
    printf(" \n");
    printf("freqalert %s, (C)2018 By MrDave \n",VERSION);
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

static void process_signal(int psignal) {
    if (psignal == SIGTERM)	exit_app = 1;
    if (psignal == SIGINT)	exit_app = 1;
}

static void show_help(void){

	printf("\n"
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

static void check_alerts(config_parms_t *parms, int16_t *input_buffer){

    float freq_value=0;
    int   indx;
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

    for (indx=0;indx < parms->frames_per_buffer;indx++){
        parms->ff_in[indx] = input_buffer[indx];
        if (0){
            /* This is an alternate.  Need to check results*/
            parms->ff_in[indx] = input_buffer[indx] * HammingWindow(indx, 2048);
        }
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

    if (parms->verbosity > 0) printf("Frequency %.8f \n",freq_value);

    /*Now check to see if the freq is in the range for an event*/
    for (indx=0;indx < parms->n_alerts;indx++){
        if (freq_value >= parms->alert_info[indx].freq_low &&
            freq_value <= parms->alert_info[indx].freq_high) {
            alert_triggered[indx]++;
        } else {
        }
    }

    /*  Now go through and call the alerts triggered */
    for (indx=0;indx < parms->n_alerts;indx++) {
        if (alert_triggered[indx] >= parms->alert_info[indx].freq_duration ) {
            if (parms->verbosity > 10){
                printf("Triggering Frequency Alert %d  Duration: %d \n"
                    ,indx, alert_triggered[indx]);
            }
            parms->alert_info[indx].triggered_count=0 ;
            parms->alert_info[indx].triggered_count++ ;
            exec_cmd(parms->alert_info[indx].alert_event_cmd);
        }
    }

}

static int alsa_snddev_open(config_parms_t *parms){

    /* Based upon /inspired by http://equalarea.com/paul/alsa-audio.html */
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_uframes_t frames_per;
    snd_pcm_format_t actl_sndfmt;
    unsigned int actl_rate;
    int retcd;

    retcd = snd_pcm_open(&parms->devhandle, parms->device, SND_PCM_STREAM_CAPTURE, 0);
    if (retcd < 0) {
        printf("error: snd_pcm_open device %s (%s)\n", parms->device, snd_strerror (retcd));
        return -1;
    }

    retcd = snd_pcm_hw_params_malloc (&hw_params);
    if (retcd < 0) {
        printf("error: snd_pcm_hw_params_malloc(%s)\n", snd_strerror (retcd));
        return -1;
    }

    retcd = snd_pcm_hw_params_any(parms->devhandle, hw_params);
    if (retcd < 0) {
        printf("error: snd_pcm_hw_params_any(%s)\n", snd_strerror (retcd));
        return -1;
    }

    retcd = snd_pcm_hw_params_set_access(parms->devhandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (retcd < 0) {
        printf("error: snd_pcm_hw_params_set_access(%s)\n", snd_strerror (retcd));
        return -1;
    }

    retcd = snd_pcm_hw_params_set_format(parms->devhandle, hw_params, SND_PCM_FORMAT_S16_LE);
    if (retcd < 0) {
        printf ("error: snd_pcm_hw_params_set_format(%s)\n", snd_strerror (retcd));
        return -1;
    }

    retcd = snd_pcm_hw_params_set_rate_near(parms->devhandle, hw_params, &parms->sample_rate, 0);
    if (retcd < 0) {
        printf ("error: snd_pcm_hw_params_set_rate_near(%s)\n", snd_strerror (retcd));
        return -1;
    }

    retcd = snd_pcm_hw_params_set_channels(parms->devhandle, hw_params, parms->channels);
    if (retcd < 0) {
        printf("error: snd_pcm_hw_params_set_channels(%s)\n", snd_strerror (retcd));
        return -1;
    }

    frames_per = parms->frames_per_buffer;
    retcd = snd_pcm_hw_params_set_period_size_near(parms->devhandle
        , hw_params, &frames_per, NULL);
    if (retcd < 0) {
        printf("error: snd_pcm_hw_params_set_period_size_near(%s)\n", snd_strerror (retcd));
        return -1;
    }

    retcd = snd_pcm_hw_params (parms->devhandle, hw_params);
    if (retcd < 0) {
        printf("error: snd_pcm_hw_params(%s)\n", snd_strerror (retcd));
        return -1;
    }

    snd_pcm_hw_params_free (hw_params);

    retcd = snd_pcm_prepare(parms->devhandle);
    if (retcd < 0) {
        printf("error: snd_pcm_prepare(%s)\n", snd_strerror (retcd));
        return -1;
    }

    /* get actual parms selected */
	retcd = snd_pcm_hw_params_get_format(hw_params,&actl_sndfmt);
    if (retcd < 0) {
        printf("error: snd_pcm_hw_params_get_format(%s)\n", snd_strerror (retcd));
        return -1;
    }

    retcd = snd_pcm_hw_params_get_rate(hw_params, &actl_rate, NULL);
    if (retcd < 0) {
        printf("error: snd_pcm_hw_params_get_rate(%s)\n", snd_strerror (retcd));
        return -1;
    }

    retcd = snd_pcm_hw_params_get_period_size(hw_params, &frames_per, NULL);
    if (retcd < 0) {
        printf("error: snd_pcm_hw_params_get_rate(%s)\n", snd_strerror (retcd));
        return -1;
    }

    printf("actual rate: %hu\n",actl_rate);
    printf("actual frames per: %lu\n",frames_per);
	if (actl_sndfmt <= 5)
		printf("sound format: 16\n");
	else if (actl_sndfmt <= 9 )
		printf("sound format: 24\n");
	else
        printf("sound format 32\n");

    /*************************************************************/
    /** allocate and initialize the sound buffers                */
    /*************************************************************/
    parms->buf_size = parms->frames_per_buffer * 2;
    parms->input_buffer = malloc(parms->buf_size * sizeof(int16_t));
    memset(parms->input_buffer, 0x00,  parms->buf_size * sizeof(int16_t));

    return retcd;

}

static int alsa_snddev_close(config_parms_t *parms){
    int retcd;

    retcd = 0;

    if (parms->verbosity > 0) printf("**  Closing stream\n");

    snd_pcm_close(parms->devhandle);

    free(parms->input_buffer);

    return retcd;

}

static int alsa_snddev_read(config_parms_t *parms){

    int retcd;

    retcd = snd_pcm_readi(parms->devhandle, parms->input_buffer, parms->frames_per_buffer);
    if (retcd != parms->frames_per_buffer) {
        printf("error: read from audio interface failed (%s)\n", snd_strerror (retcd));
        return -1;
    }

    if (exit_app == 1) exit_app++;

    return 0;

}

static int alsa_snddev_list(config_parms_t *parms){

    snd_ctl_t               *handle;
    snd_ctl_card_info_t     *info;
    snd_pcm_info_t          *pcminfo;
    char                    name[32];
    int card, dev, idx, retcd;
    unsigned int count;

    retcd = 0;

    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);

    card = -1;
    if (snd_card_next(&card) < 0 || card < 0) {
        printf("no soundcards found...");
        return -1;
    }
    printf("**** List of %s Hardware Devices ****\n",
        snd_pcm_stream_name(SND_PCM_STREAM_CAPTURE));

    while (card >= 0) {
        sprintf(name, "hw:%d", card);

        retcd = snd_ctl_open(&handle, name, 0);
        if (retcd < 0) {
            printf("control open (%i): %s", card, snd_strerror(retcd));
            goto next_card;
        }

        retcd = snd_ctl_card_info(handle, info);
        if (retcd < 0) {
            printf("control hardware info (%i): %s", card, snd_strerror(retcd));
            snd_ctl_close(handle);
            goto next_card;
        }

        dev = -1;
        while (1) {
            retcd = snd_ctl_pcm_next_device(handle, &dev);
            if (retcd < 0) {
                printf("snd_ctl_pcm_next_device");
            }

            if (dev < 0) break;

            snd_pcm_info_set_device(pcminfo, dev);
            snd_pcm_info_set_subdevice(pcminfo, 0);
            snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
            retcd = snd_ctl_pcm_info(handle, pcminfo);
            if (retcd < 0) {
                if (retcd != -ENOENT){
                    printf("control digital audio info (%i): %s"
                        , card, snd_strerror(retcd));
                }
                continue;
            }
            printf("card %i: %s [%s], device %i: %s [%s]\n",
                card, snd_ctl_card_info_get_id(info), snd_ctl_card_info_get_name(info),
                dev,
                snd_pcm_info_get_id(pcminfo),
                snd_pcm_info_get_name(pcminfo));

            count = snd_pcm_info_get_subdevices_count(pcminfo);
            printf( "  Subdevices: %i/%i\n",
                snd_pcm_info_get_subdevices_avail(pcminfo), count);

            for (idx = 0; idx < (int)count; idx++) {
                snd_pcm_info_set_subdevice(pcminfo, idx);
                retcd = snd_ctl_pcm_info(handle, pcminfo);
                if (retcd < 0) {
                    printf("control digital audio playback info (%i): %s"
                        , card, snd_strerror(retcd));
                } else {
                    printf("  Subdevice #%i: %s\n",
                        idx, snd_pcm_info_get_subdevice_name(pcminfo));
                }
            }
        }
        snd_ctl_close(handle);
    next_card:
        if (snd_card_next(&card) < 0) {
            printf("snd_card_next");
            break;
        }
    }

    (void)parms;

    return retcd;

}

static int fftw_plan_open(config_parms_t *parms){

    int retcd, indx;

    retcd = 0;

    /*************************************************************/
    /** allocate and initialize the fftw plan and buffer         */
    /*************************************************************/
    if (parms->verbosity > 0) printf("**  Computing the fastest FFTW method \n");
    parms->ff_in   = (double*) fftw_malloc(sizeof(fftw_complex) * parms->frames_per_buffer);
    parms->ff_out  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * parms->frames_per_buffer);
    parms->ff_plan = fftw_plan_dft_r2c_1d(
        parms->frames_per_buffer
        , parms->ff_in, parms->ff_out
        , FFTW_MEASURE);

    for (indx=0;indx < parms->frames_per_buffer;indx++){
        //parms->ff_in[indx][1] = 0;
        parms->ff_in[indx] = 0;
    }
    parms->bin_min = 1;
    parms->bin_max = (parms->frames_per_buffer / 2);
    parms->bin_size = ((float)parms->sample_rate / (float)parms->frames_per_buffer);
    parms->buf_items = parms->sample_rate * parms->channels;

    return retcd;

}

static int fftw_plan_close(config_parms_t *parms){
    /*************************************************************/
    /** Clean up FFTW vars                                       */
    /*************************************************************/

    int retcd;

    retcd = 0;

    fftw_destroy_plan(parms->ff_plan);
    fftw_free(parms->ff_in);
    fftw_free(parms->ff_out);

    return retcd;
}

static int check_levels(config_parms_t *parms, int indx_worker) {
    int indx, max_level, chkval, threshold_count, retcd;
    struct timespec time_start, time_end;

    clock_gettime(CLOCK_MONOTONIC, &time_start);

    /* check audio-levels */
    max_level = 0;
    retcd = 0;
    threshold_count = 0;
    (void) indx_worker;

    for(indx=0; indx < parms->frames_per_buffer ; indx++) {
        chkval = abs((int)parms->input_buffer[indx] / 256);
        if (chkval > max_level ) max_level = chkval ;
        if (chkval > parms->volume_level) threshold_count++;
    }

    if (threshold_count >= parms->volume_triggers) {
        if (parms->verbosity > 10){
            printf("Recording...Detected Level: %d  Threshold Level: %d \n",threshold_count,parms->volume_level);
        }

        check_alerts(parms, parms->input_buffer);
    }

    clock_gettime(CLOCK_MONOTONIC, &time_end);

    if (exit_app == 1) exit_app++;

    return retcd;
}

static int process_normal(config_parms_t *parms) {

    int indx_worker;

    if (alsa_snddev_open(parms) < 0) return -1;

    if (fftw_plan_open(parms) < 0) return -1;

    indx_worker = 0;
    while (exit_app != 2)	{

        alsa_snddev_read(parms);

        check_levels(parms, indx_worker);

        indx_worker++;
        if (indx_worker > 4) indx_worker = 0;
    }

    if (fftw_plan_close(parms) < 0) return -1;

    if (alsa_snddev_close(parms) < 0) return -1;

    return 0;

}

int main(int argc, char *argv[]){
    int retcd;
    config_parms_t parms;

    exit_app = 0;

    print_terms();

    retcd = parms_load(&parms,argc, argv);
    if (retcd < 0) return 1;

    parms_print(&parms);

    if (parms.show_help == 1){
        show_help();
        return 0;
    }

    parms.show_info = 1;
    if (parms.show_info == 1){
        alsa_snddev_list(&parms);
    }

    signal(SIGTERM, process_signal);
    signal(SIGINT, process_signal);

    if (parms.run_in_background == 1) exec_background();

    retcd = process_normal(&parms);
    if (retcd < 0) return 1;

    return retcd ;

}

