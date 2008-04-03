#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H 

#include <Ecore.h>
#include <Ecore_Data.h>
#include <Ecore_File.h>

typedef struct Diversity_Nav_Config Diversity_Nav_Config; 
struct Diversity_Nav_Config
{
   Ecore_Hash *cfg_data;
};


Diversity_Nav_Config     *dn_config_new(void);
void                      dn_config_destroy(Diversity_Nav_Config *cfg);
int                       dn_config_save(Diversity_Nav_Config *cfg);

void                      dn_config_string_set(Diversity_Nav_Config *cfg, const char *k, const char *v);
const char               *dn_config_string_get(Diversity_Nav_Config *cfg, const char *k);
void                      dn_config_int_set(Diversity_Nav_Config *cfg, const char *k, int v);
int                       dn_config_int_get(Diversity_Nav_Config *cfg, const char *k);
void                      dn_config_float_set(Diversity_Nav_Config *cfg, const char *k, float v);
float                     dn_config_float_get(Diversity_Nav_Config *cfg, const char *k);

#endif
