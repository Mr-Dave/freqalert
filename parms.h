#ifndef _INCLUDE_PARMS_H_
#define _INCLUDE_PARMS_H_

int parms_load(config_parms_t *parms,int argc, char *argv[]);
void parms_free(config_parms_t *parms);
void parms_print(config_parms_t *parms);

#endif


