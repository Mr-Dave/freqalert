/********************************************************
 *
 *  parms.c
 *  MrDave
 *  Populate the parms with the values specified by the user
 *
 *  parms_load  - main function to load parameters
 *  parms_default - load the hard coded program defaults
 *  parms_getconfigname - get any config file name from command line
 *  parms_configfile - get configuration items from file
 *  parms_cmdline - get the configuration items specified on command line
 *  parms_addfilter - add filters to array in parms
 *  parms_addalertinfo - add alert data to array in parms
 *  parms_checkforyes - translate the parm option into 1 or 0
 *  parms_show_verbose - print out all parameters for verbose mode
 *  parms_validate  - check to make sure parameters specified are valid
 *  util_rtrim   - trim spaces from right of string
 *  util_ltrim   - trim spaces from left of a string.
 *  util_trim    - trim spaces from left and right of string
 *  util_substr  - typical substring / mid function
 */

#include "freqalert.h"
#include "parms.h"

static void util_rtrim(char *src) {
    int indx;

    indx = strlen(src) ;

    while(indx > 0){
        if ((src[indx-1] !=' ') &&
            (src[indx-1] !='\n') &&
            (src[indx-1] !='\t') &&
            (src[indx-1] !='\r')) {
                break;
            }
        indx--;
    }
    src[indx] = '\0';
}

static void util_ltrim(char *src) {

    int  indx;

    indx = 0;
    while(indx < (int)(strlen(src)-1)){
        if (src[indx + indx] !=' '){
            break;
        }
        indx++;
    }
    memmove(src, src + indx, strlen(src) - indx + 1);

}

static void util_trim(char *src) {

    util_rtrim(src);

    util_ltrim(src);

}

static void util_substr(char *dest, char *src, int position, int length) {

    if (position < 1) {
        printf("Contact developer, util_substr is 1 based not 0 \n");
        return;
    }

    memcpy(dest, src + position - 1, length);

    dest[length] = '\0';

}

static int parms_count(config_parms_t *parms, int indx){
    /* return the count the number of specs for the alert */
    int retcd;

    retcd = 0;
    if (parms -> alert_info[indx].freq_low  >= 0) retcd++;
    if (parms -> alert_info[indx].freq_high >= 0) retcd++;
    if (parms -> alert_info[indx].freq_duration >= 0) retcd++;
    if (strlen(parms -> alert_info[indx].alert_event_cmd) >0) retcd++;
    if (parms -> alert_info[indx].alert_volume_level >= 0) retcd++;

    return retcd;
}

static void parms_invalid_ntc(config_parms_t *parms, int indx){

    /* At least one of the parms was NOT specified */
    printf(" \n");
    printf("Error in Configuration: \n");
    printf("Incomplete specification of Alert %d \n",indx);
    printf("Need to have something like this \n");
    printf(" \n");
    printf("Alert_00_Freq_Low=3330.000\n");
    printf("Alert_00_Freq_High=3500.000\n");
    printf("Alert_00_Duration = 1\n");
    printf("Alert_00_Command = /home/sendwaterleakalert.sh\n");
    printf("Alert_00_Volume_Level = 100\n");
    printf(" \n");

    printf("Values specified \n");
    printf("Freq_Low %f \n",parms->alert_info[indx].freq_low);
    printf("Freq_High %f \n",parms->alert_info[indx].freq_high);
    printf("Duration %d \n",parms->alert_info[indx].freq_duration);
    printf("Command %s \n",parms->alert_info[indx].alert_event_cmd);
    printf("Volume_Level %d \n",parms->alert_info[indx].alert_volume_level);
    printf(" \n");

    printf("Respecify the alert parameters in config file.\n");

}

static int parms_validate(config_parms_t *parms) {

    int indx, parmcnt;

    if (parms->run_in_background == 1) parms->verbosity = 0;
    if (parms->frames_per_buffer == 0) parms->frames_per_buffer = 2048;

    for(indx=0;indx < MAX_N_ALERTS;indx++) {
        parmcnt = parms_count(parms, indx);
        if (parmcnt > 0) {
            if (parmcnt < 5){
                parms_invalid_ntc(parms, indx);
                return -1;
            }

            /* The alert info is valid, now check for being in sequence */
            if (indx == 0) {
                parms->n_alerts=1;
            } else {
                if (parms -> alert_info[indx-1].freq_low  < 0 ) {
                    printf("\n\nError in Configuration: Alert numbers must be sequential.\n");
                    return -1;
                } else {
                    parms->n_alerts++;
                }
            }
        }
    }

    /* Set the minimum volume of all the alerts */
    parms->volume_level =0;
    for(indx=0;indx < MAX_N_ALERTS;indx++) {
        if (parms->volume_level < parms->alert_info[indx].alert_volume_level) {
            parms->volume_level = parms->alert_info[indx].alert_volume_level;
        }
    }

    return 0;

}

static void parms_default(config_parms_t *parms) {
    int indx;

    parms->sample_rate = 44100;
    parms->channels = 1;
    snprintf(parms->configfile, MAX_LINE, "%s", "freqalert.conf");   /*configurationfile  */
    parms->volume_level = 754;     /* threshold for sound-detection*/
    parms->volume_triggers = 2;		 /* how many times the signal should be above the trigger level*/
    parms->run_in_background = 0;  /* wether to run in the background or not*/
    parms->n_alerts = 0;           /* Number of freqalerts specified*/
    snprintf(parms->device,MAX_LINE,"%s","hw:0,0");  /* default mic*/
    parms->show_help=0;
    parms->show_info=0;
    parms->verbosity = 1;
    parms->frames_per_buffer = 2048;

    for(indx=0;indx < MAX_N_ALERTS;indx++) {
        parms->alert_info[indx].freq_low = -1;
        parms->alert_info[indx].freq_high = -1;
        parms->alert_info[indx].freq_duration = -1;
        snprintf(parms->alert_info[indx].alert_event_cmd
            ,MAX_LINE,"%s","");
        parms->alert_info[indx].alert_volume_level = -1;
    }


}

static void parms_cmdline(config_parms_t *parms,int argc, char *argv[]) {
    int option_cd;

    opterr = 0;

    /* Check command-line options*/
    while ((option_cd = getopt (argc, argv, "fl:hic:svp")) != -1) {
        switch (option_cd) {
        case 'f':
          parms->run_in_background = 1;
          break;
        case 'l':
          parms->volume_level = atoi(optarg);
          break;
        case 'h':
          parms->show_help = 1;
          break;
        case 'i':
          parms->show_info = 1;
          break;
        case 'c':
          snprintf(parms->configfile, MAX_LINE, "%s", optarg);
          break;
        case 's':
          parms->verbosity = 0;
          break;
        case 'v':
          parms->verbosity = 2;
          break;
        case 'p':
          parms->verbosity = 3;
          break;
        default:
          /* unknown switch*/
          parms->show_help = 1;
          break;
        } /* switch..*/
	  } /* while..*/

}

static void parms_getconfigname(config_parms_t *parms,int argc, char *argv[]){
    /* In this function we are ONLY looking for a override config file.
    * Other options on the command line override the ones in config.
    * Have to do this old fashion way since getopt clears the array.
    */
    int  option_len, indx;
    char *option_cd, *option_chk;

    option_cd=malloc(MAX_LINE);
    option_chk=malloc(MAX_LINE);

    for (indx = 1; indx < argc ; indx++) {
        snprintf(option_cd, MAX_LINE, "%s",argv[indx]);
        option_len = strlen(option_cd);
        if (option_len > 2) {
              util_substr(option_chk, option_cd, 1, 2);
            if (strcmp(option_chk,"-c") == 0) {
                util_substr(parms->configfile, option_cd, 3, option_len-3);
                util_trim(parms->configfile);
            }
        }
    }

    free(option_cd);
    free(option_chk);

}

static void parms_addalertinfo(config_parms_t *parms,char *option_cd,char *option_val) {

    char *alert_prefix, *alert_nbr,*alert_item;
    char *underscore_loc;
    int  option_len, indx;

    alert_prefix=malloc(MAX_LINE);
    alert_nbr=malloc(MAX_LINE);
    alert_item=malloc(MAX_LINE);

    option_len=0;
    underscore_loc = strchr(option_cd,'_');
    if (underscore_loc != NULL) {
        /* alert_00_freq_low */
        option_len = strlen(option_cd) - strlen(underscore_loc);
        util_substr(alert_prefix, option_cd, 1, option_len);
        util_trim(alert_prefix);

        /* Trim off the prefix and get next part of parameter */
        util_substr(option_cd, underscore_loc, 2, strlen(underscore_loc));
        underscore_loc = strchr(option_cd,'_');
        if (underscore_loc != NULL) {
            /* 00_freq_low */
            option_len = strlen(option_cd) - strlen(underscore_loc) ;
            util_substr(alert_nbr, option_cd, 1, option_len);
            util_trim(alert_nbr);

            indx = atoi(alert_nbr);

            if (indx < MAX_N_ALERTS ) {
                /* Trim off the nbr and see what we have left */
                util_substr(alert_item, underscore_loc, 2, strlen(underscore_loc)- 1);

                if (strcmp(alert_item, "freq_low") == 0){
                    parms->alert_info[indx].freq_low = atof(option_val);
                }

                if (strcmp(alert_item, "freq_high") == 0){
                    parms->alert_info[indx].freq_high = atof(option_val);
                }

                if (strcmp(alert_item, "duration") == 0){
                    parms->alert_info[indx].freq_duration = atoi(option_val);
                }

                if (strcmp(alert_item, "volume_level") == 0){
                    parms->alert_info[indx].alert_volume_level = atoi(option_val);
                }

                if (strcmp(alert_item, "command") == 0){
                    snprintf(parms->alert_info[indx].alert_event_cmd
                        ,MAX_LINE,"%s",option_val);
                }
            }
        }
    }

    free(alert_prefix);
    free(alert_nbr);
    free(alert_item);

}

static int parms_configfile(config_parms_t *parms) {
    /*  The config file is specified in parms
    *  The function getconfig file is run before this
    *  this one and populates the parms with anything
    *  specified on the command line.
    *
    *  Any options in the config file override the defaults
    */

    char *equals_loc;
    char *config_line, *option_prefix;
    char *option_cd, *option_val;
    int  option_len;
    FILE	*file_handle;

    file_handle = fopen(parms->configfile, "rb");
    if (!file_handle) {
        file_handle = fopen("freqalert.conf", "rb");
        if (file_handle) {
            printf("Using freqalert.conf from current directory\n");
        } else {
            printf("error opening configfile %s\n", parms->configfile);
            return -1;
        }
    } else {
        printf("Using conf from %s\n",parms->configfile);
    }

    config_line = malloc(MAX_LINE);
    option_cd = malloc(MAX_LINE);
    option_prefix = malloc(MAX_LINE);
    option_val = malloc(MAX_LINE);

    while (fgets(config_line,MAX_LINE,file_handle) != NULL) {

        if (config_line[0] == '#') {
            continue;
        }

        equals_loc = strchr(config_line, '=');  /*Find the = in the line */
        if (equals_loc == NULL){
            printf("invalid option.  No equals sign >%s<  \n",config_line);
            continue;
        }

        option_len =strlen(config_line) - strlen(equals_loc) ;
        util_substr(option_cd, config_line, 1, option_len);
        util_trim(option_cd);

        util_trim(equals_loc);
        util_substr(option_val, equals_loc, 2, strlen(equals_loc));
        util_trim(option_val);

        if (strlen(option_val)>0) {
            if (strcmp(option_cd, "volume_triggers") == 0){
                parms->volume_triggers = atoi(option_val);
            }
            if (strcmp(option_cd, "device") == 0){
                snprintf(parms->device,MAX_LINE,"%s",option_val);
            }
            if (strcmp(option_cd, "verbosity") == 0){
                parms->verbosity = atoi(option_val);
            }
            if (strcmp(option_cd, "sample_rate") == 0){
                parms->sample_rate= atoi(option_val);
            }
            if (strcmp(option_cd, "channels") == 0){
                parms->channels = atoi(option_val);
            }
            if (strcmp(option_cd, "frames_per_buffer") == 0){
                parms->frames_per_buffer = atoi(option_val);
            }
            if (strncmp(option_cd,"alert",5) == 0){
                parms_addalertinfo(parms, option_cd, option_val);
            }
        }
    }

    fclose(file_handle);

    free(config_line);
    free(option_cd);
    free(option_prefix);
    free(option_val);

    return 0;
}

void parms_print(config_parms_t *parms) {
    int indx;

    if (parms->verbosity == 0) return;
    printf("******************************************\n");
    printf("**           FreqAlert                  **\n");
    printf("******************************************\n");
    printf("**      Configuration Parameters        **\n");
    printf("******************************************\n");
    printf("** Priorities of specifications         **\n");
    printf("** 3. Application hard coded defaults   **\n");
    printf("** 2. Configuration file                **\n");
    printf("** 1. Command line                      **\n");
    printf("******************************************\n");
    printf("** Option: volume_triggers       Value: %d\n",parms->volume_triggers);
    printf("** Option: sample rate           Value: %d\n",parms->sample_rate);
    printf("** Option: channels              Value: %d\n",parms->channels);
    printf("** Option: device                Value: %s\n",parms->device);
    printf("** Option: verbosity             Value: %d\n",parms->verbosity);
    printf("** Option: frames_per_buffer     Value: %d\n",parms->frames_per_buffer);

    /* Show all the freq alerts*/
    if (parms->n_alerts == 0) {
        printf("** Option: Frequency Alerts      Value: None \n");
    } else {
        for (indx=0 ;indx < parms->n_alerts ;indx++) {
            printf("** Option: Frequency Alert %d \n",indx);
            printf("**   Low Frequency : %f \n",parms->alert_info[indx].freq_low);
            printf("**   High Frequency: %f \n",parms->alert_info[indx].freq_high);
            printf("**   Duration: %d \n",parms->alert_info[indx].freq_duration);
            printf("**   Volume: %d \n",parms->alert_info[indx].alert_volume_level);
            printf("**   Command :%s \n",parms->alert_info[indx].alert_event_cmd);
        }
    }
    printf("******************************************\n");

}


int parms_load(config_parms_t *parms,int argc, char *argv[]) {

    int retcd;

    parms_default(parms);

    parms_getconfigname(parms,argc,argv);

    retcd = parms_configfile(parms);
    if (retcd < 0) return retcd;

    parms_cmdline(parms,argc, argv);

    retcd = parms_validate(parms);
    if (retcd < 0) return retcd;

    return 0;

}

