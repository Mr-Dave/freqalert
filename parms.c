/********************************************************
 *
 *  parms.c
 *  January 2012
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
 *  parms_free - Free any allocated structures
 *  rtrim   - trim spaces from right of string
 *  ltrim   - trim spaces from left of a string.
 *  trim    - trim spaces from left and right of string
 *  substr  - typical substring / mid function
 */

#include "freqalert.h"

#define MAX_LINE 200

void rtrim(char *src) {
  int indx;
    
  indx = strlen(src) ;
  while( (indx > 0) && (src[indx-1] ==' ') ) indx--;
  
  if (indx > 0)  memmove(src,src,indx);

  src[indx] = '\0';

}
void ltrim(char *src) {

  int  indx;
  
  indx = 0;
  while(indx < (strlen(src)-1) && isspace(*src + indx)) indx++;

  memmove(src, src + indx, strlen(src) - indx + 1);

}
void trim(char *src) {

  rtrim(src);
  ltrim(src);

}
void substr(char *dest, char *src, int position, int length) {

  if (position < 1) {
    printf("Contact developer, substr is 1 based not 0 \n");
    return; 
  }
  memcpy(dest,src+position-1,length);
  dest[length] = '\0';
  
}
int parms_read_line(FILE *fh,char **line_in) {

  /* This routine seems functional but is very poorly written */

  char one_char;
  int  cnt_char = 1;
  int  end_of_line = 0;
  int  ret_cd = 0;

  char *pline;
  char *ptemp;

  pline = malloc(sizeof(char)*cnt_char);
  ptemp = malloc(sizeof(char)*cnt_char);

  *pline = '\0';

  while (end_of_line == 0) {
    one_char = fgetc(fh);
    if ((one_char != EOF) && (one_char != 10) && (one_char != 13)) {
      free(ptemp);
      ptemp = malloc(cnt_char * sizeof(char));
      strcpy(ptemp, pline);

      cnt_char++;

      free(pline);
      pline = malloc(cnt_char * sizeof(char));
      strcpy(pline, ptemp);
      pline[cnt_char-2] = one_char;
      pline[cnt_char-1] = '\0';
    } else if (one_char == 10) {
      end_of_line = 1;
    } else if (one_char == EOF) {
     end_of_line = 1;
     ret_cd = 1;
    }
    /* Ignore the lf (13) */
  }

  *line_in = malloc(cnt_char * sizeof(char));
  strcpy(*line_in,pline);

  free(ptemp);
  free(pline);
  
  return ret_cd ;
}
void parms_validate(config_parms_t *parms) {
  int indx;

  if (parms->run_in_background == 1) parms->verbosity = 0;
  if (parms->frames_per_buffer == 0) parms->frames_per_buffer = 2048;
  
  for(indx=0;indx < MAX_N_ALERTS;indx++) {
    if (parms -> alert_info[indx].freq_low  >= 0 ||
        parms -> alert_info[indx].freq_high >= 0 ||
        parms -> alert_info[indx].freq_duration >= 0   ||
        parms -> alert_info[indx].alert_event_cmd >0    ||
        parms -> alert_info[indx].alert_volume_level >= 0)
    {
      /* At least one of the parms was specified */
      if (parms -> alert_info[indx].freq_low  < 0 ||
          parms -> alert_info[indx].freq_high < 0 ||
          parms -> alert_info[indx].freq_duration  < 0   ||
          parms -> alert_info[indx].alert_event_cmd <= 0 ||
          parms -> alert_info[indx].alert_volume_level < 0)
      {
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
        exit(EXIT_FAILURE);
      }

      /* The alert info is valid, now check for being in sequence */
      if (indx == 0) {
        parms->n_alerts=1;
      } else {
        if (parms -> alert_info[indx-1].freq_low  < 0 ) {
          printf(" \n");
          printf(" \n");
          printf("Error in Configuration: Alert numbers must be sequential.\n");
          exit(EXIT_FAILURE);
        } else {
          parms->n_alerts++;
        } /* end else */
      } /* end else */

    }/* end checking alert info */
  } /* end of the loop through alerts */

  /* Set the minimum volume of all the alerts */
  parms->volume_level =0;
  for(indx=0;indx < MAX_N_ALERTS;indx++) {
    if (parms->volume_level < parms->alert_info[indx].alert_volume_level) {
      parms->volume_level = parms->alert_info[indx].alert_volume_level;
    }
  }

}
void parms_show_verbose(config_parms_t *parms) {
  int indx;

  printf("*****************************************\n");
  printf("**      Configuration Parameters       **\n");
  printf("*****************************************\n");
  printf("** Priorities of specifications        **\n");
  printf("** 3. Application hard coded defaults  **\n");
  printf("** 2. Configuration file               **\n");
  printf("** 1. Command line                     **\n");
  printf("**  Note: 0=no, 1=yes                  **\n");
  printf("*****************************************\n");
  printf("** Option: volume_triggers       Value: %d\n",parms->volume_triggers);
  printf("** Option: sample rate           Value: %d\n",parms->sample_rate);
  printf("** Option: channels              Value: %d\n",parms->channels);
  printf("** Option: device_number         Value: %d\n",parms->device_number);
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
  printf("*****************************************\n");

}
void parms_default(config_parms_t *parms) {
  int indx;

  parms->sample_rate = 44100;
  parms->channels = 2;
  parms->configfile = strdup("freqalert.conf");   /*configurationfile  */
  parms->volume_level = 754;     /* threshold for sound-detection*/
  parms->volume_triggers = 2;		 /* how many times the signal should be above the trigger level*/
  parms->run_in_background = 0;  /* wether to run in the background or not*/
  parms->n_alerts = 0;           /* Number of freqalerts specified*/
  parms->device_number =0;       /* Zero means use default mic*/
  parms->show_help=0;
  parms->show_info=0;
  parms->verbosity = 1;
  parms->frames_per_buffer = 2048;

  for(indx=0;indx <= MAX_N_ALERTS;indx++) {
    parms -> alert_info[indx].freq_low = -1;
    parms -> alert_info[indx].freq_high = -1;
    parms -> alert_info[indx].freq_duration = -1;
    parms -> alert_info[indx].alert_event_cmd = 0;
    parms -> alert_info[indx].alert_volume_level = -1;

  }

}
void parms_cmdline(config_parms_t *parms,int argc, char *argv[]) {
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
        parms->configfile = strdup(optarg);
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
void parms_getconfigname(config_parms_t *parms,int argc, char *argv[]){
  int   indx;

  /* In this function we are ONLY looking for a override config file.
   * Other options on the command line override the ones in config.
   * Have to do this old fashion way since getopt clears the array.
  */
  int   option_len=0;
  char *option_cd =NULL;
  char *option_chk=NULL;

  for (indx = 1; indx < argc ; indx++) {
    option_cd  = strdup(argv[indx]);
    option_len = strlen(option_cd);
    if (option_len > 2) {
      substr(option_chk,option_cd,1,2);
      if (strcmp(option_chk,"-c") == 0) {
        substr(parms->configfile, option_cd, 3, option_len-3);
      }
    }
  }
}
int parms_checkforyes(char *option_val) {
  int ret_val;

  ret_val = 0;

  if (strcasecmp(option_val, "on") ==0 ||
      strcasecmp(option_val, "yes") == 0 ||
      strcasecmp(option_val, "y") == 0 ||
      strcasecmp(option_val, "1") == 0) ret_val = 1;

  return ret_val;

}
void parms_addalertinfo(config_parms_t *parms,char *option_cd,char *option_val) {
  char alert_prefix[MAX_LINE];
  char alert_nbr[MAX_LINE];
  char alert_item[MAX_LINE];
  char *underscore_loc=NULL;

  int  option_len=0;

  underscore_loc = strchr(option_cd,'_');
  if (underscore_loc > 0) {
    /* alert_00_freq_low */
    option_len = strlen(option_cd) - strlen(underscore_loc);
    substr(alert_prefix, option_cd,1,option_len);
    trim(alert_prefix);
 
    /* Trim off the prefix and get next part of parameter */
    substr(option_cd,underscore_loc,2,strlen(underscore_loc));
    underscore_loc = strchr(option_cd,'_');
    if (underscore_loc > 0) {
      /* 00_freq_low */
      option_len = strlen(option_cd) - strlen(underscore_loc) ;
      substr(alert_nbr,option_cd,1,option_len);
      trim(alert_nbr);

      if (atoi(alert_nbr) <= MAX_N_ALERTS ) {
        /* Trim off the nbr and see what we have left */
        substr(alert_item,underscore_loc,2,strlen(underscore_loc)- 1);

        if (strcasecmp(alert_item, "freq_low") == 0)
          parms -> alert_info[atoi(alert_nbr)].freq_low = atof(option_val);

        if (strcasecmp(alert_item, "freq_high") == 0)
          parms -> alert_info[atoi(alert_nbr)].freq_high = atof(option_val);

        if (strcasecmp(alert_item, "duration") == 0)
          parms -> alert_info[atoi(alert_nbr)].freq_duration = atoi(option_val);

        if (strcasecmp(alert_item, "volume_level") == 0)
          parms -> alert_info[atoi(alert_nbr)].alert_volume_level = atoi(option_val);

        if (strcasecmp(alert_item, "command") == 0)
          parms -> alert_info[atoi(alert_nbr)].alert_event_cmd = strdup(option_val);

      }
    }
  }
}
void parms_configfile(config_parms_t *parms,int argc, char *argv[]) {
/*  The config file is specified in parms
 *  The function getconfig file is run before this
 *  this one and populates the parms with anything
 *  specified on the command line.
 *
 *  Any options in the config file override the defaults
 */

  char *equals_loc;
  char *underscore_loc;
  
  char *config_line;
  char option_prefix[MAX_LINE];
  char option_cd[MAX_LINE];  
  char option_val[MAX_LINE];
  int  option_len=0;
  
  FILE	*file_handle;
  file_handle = fopen(parms->configfile, "rb");
  if (!file_handle) {
    file_handle = fopen("freqalert.conf", "rb");
    if (file_handle) {
      printf("Using freqalert.conf from current directory\n");
    } else {
      printf("error opening configfile %s\n", parms->configfile);
      exit(EXIT_FAILURE);
    }
  }

  while (parms_read_line(file_handle,&config_line) == 0) {

    if (config_line[0] != '#') {
      equals_loc = strchr(config_line, '=');  /*Find the = in the line */
      if (equals_loc > 0) {
        //printf("option >%s<  \n",config_line);
                
        underscore_loc = strchr(config_line,'_');
        if (underscore_loc > 0) {
          option_len =strlen(config_line) - strlen(underscore_loc) ;
          substr(option_prefix,config_line,1,option_len);
          trim(option_prefix);
        } else {
          option_len =strlen(config_line) - strlen(equals_loc) ;
          substr(option_prefix,config_line,1,option_len);
          trim(option_prefix);
        }

        option_len =strlen(config_line) - strlen(equals_loc) ;
        substr(option_cd,config_line,1,option_len);
        trim(option_cd);
 
        trim(equals_loc);
        substr(option_val,equals_loc,2,strlen(equals_loc));
        trim(option_val);

        //printf("option >%s<  >%s< >%s<  \n",option_prefix,option_cd,option_val);

        if (strlen(option_val)>0) {
          if (strcasecmp(option_cd, "volume_triggers") == 0) parms->volume_triggers = atoi(option_val);
          if (strcasecmp(option_cd, "device_number") == 0) parms->device_number = atoi(option_val);
          if (strcasecmp(option_cd, "verbosity") == 0) parms->verbosity = atoi(option_val);
          if (strcasecmp(option_cd, "sample_rate") == 0) parms->sample_rate= atoi(option_val);
          if (strcasecmp(option_cd, "channels") == 0) parms->channels = atoi(option_val);
          if (strcasecmp(option_cd, "frames_per_buffer") == 0) parms->frames_per_buffer = atoi(option_val);
          
          if (strcasecmp(option_prefix, "alert") == 0)  parms_addalertinfo(parms,option_cd,option_val);
        }
        /* End Processing the option and value */
        }  /* End If for equals sign in the line */
    }	   /* End if for comment */
    free(config_line);    
  }
  free(config_line);
  fclose(file_handle);

}
void parms_load(config_parms_t *parms,int argc, char *argv[]) {

  parms_default(parms);

  parms_getconfigname(parms,argc,argv);

  parms_configfile(parms,argc, argv);

  parms_cmdline(parms,argc, argv);

  parms_validate(parms);

  //if (parms->verbosity >= 2) parms_show_verbose(parms);

}
void parms_free(config_parms_t *parms) {

  int indx;

  for(indx=0;indx < MAX_N_ALERTS;indx++) {
    free(parms->alert_info[indx].alert_event_cmd);
  }
  free(parms->configfile);

}
