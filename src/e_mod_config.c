#include <limits.h>
#include "e_mod_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>


#define NEW(type, num) calloc(num, sizeof(type));
#define FREE(dat) \
{ \
        free(dat); dat = NULL; \
}


static int           dn_config_load(Diversity_Nav_Config *cfg);
static char         *dn_config_file_name_get(Diversity_Nav_Config *cfg);
static int           dn_config_file_read_lock(int fd);
static int           dn_config_file_write_lock(int fd);
static int           dn_config_file_unlock(int fd);
static int           dn_config_file_load(Diversity_Nav_Config *cfg, const char *file);
static Ecore_Hash   *dn_config_create_hash(void);
static void          dn_config_parse(Diversity_Nav_Config *cfg, Ecore_Hash *hash, char *data);
static char         *dn_config_trim(char *v2);


Diversity_Nav_Config *
dn_config_new(void)
{
   Diversity_Nav_Config *cfg;

   cfg = NEW(Diversity_Nav_Config, 1);
   dn_config_load(cfg);

   return cfg;
}

void
dn_config_destroy(Diversity_Nav_Config *cfg)
{
   if(!cfg)
     return;

   if (cfg->cfg_data)
     { 
        ecore_hash_destroy(cfg->cfg_data); 
        cfg->cfg_data = NULL; 
     } 

   FREE(cfg);
}

static int 
dn_config_load(Diversity_Nav_Config *cfg)
{
   char *fname = NULL;
   int ret;

   fname = dn_config_file_name_get(cfg);
   ret = dn_config_file_load(cfg, fname);
   FREE(fname);
   if (!ret)
     return FALSE;
   return TRUE;
}

static char *
dn_config_file_name_get(Diversity_Nav_Config *cfg)
{
   char cfg_filename[PATH_MAX];

   if(!cfg) return NULL;

   snprintf(cfg_filename, sizeof(cfg_filename),
                   "%s/%s.cfg", PACKAGE_DATA_DIR,
                   "diversity-nav");

   return strdup(cfg_filename);
}

static int
dn_config_file_load(Diversity_Nav_Config *cfg, const char *file)
{
   Ecore_Hash *hash;
   int fd;
   long size;
   char *data;

   fd = open(file, O_RDONLY, S_IRUSR);
   if (fd == -1)
     {
        printf("Unable to open cfg file %s.", file);
        return FALSE;
     }

   size = ecore_file_size(file);
   if (!dn_config_file_read_lock(fd))
     {
        printf("Unable to lock %s for read.", file);
        close(fd);
        return FALSE;
     }

   data = malloc(sizeof(char) * (size + 1));
   read(fd, data, size);
   data[size] = '\0';

   /* release the lock as the file is in memory */
   dn_config_file_unlock(fd);
   close(fd);

   /* create the hash to store the values */
   cfg->cfg_data = dn_config_create_hash();
   hash = cfg->cfg_data;
 
   dn_config_parse(cfg, hash, data);
   FREE(data);

   return TRUE;

}

int
dn_config_save(Diversity_Nav_Config *cfg)
{
   Ecore_List *keys;
   char *key, data[512], *path;
   long size;
   int fd;
   char *file;
   if(!cfg) return FALSE;

   file = dn_config_file_name_get(cfg);
   if(!file) return FALSE;

        /* make sure the config directory exists */
   path = strdup(file);
   key = dirname(path);
   if (!ecore_file_exists(key) && !ecore_file_mkpath(key))
     {
        printf("Unable to create %s directory path.", key);
        //DRETURN_INT(FALSE, DLEVEL_STABLE);
        return FALSE;
     }
   FREE(path);

   /* if the hash doesn't exist then treat it is empty */
   if (!cfg->cfg_data)
     return TRUE;

   fd = open(file, O_CREAT | O_WRONLY | O_TRUNC,
                   S_IRWXU | S_IRGRP | S_IROTH);

   if (fd == -1)
     {
        printf("Unable to open cfg file %s.", file);
        return FALSE;
     }

   size = ecore_file_size(file);

   if (!dn_config_file_write_lock(fd))
     {
        printf("Unable to lock %s for write.", file);
        close(fd);
        return FALSE;
     }

   keys = ecore_hash_keys(cfg->cfg_data);
   ecore_list_first_goto(keys);
   while ((key = ecore_list_next(keys)))
     {
        int len;

        len = snprintf(data, sizeof(data), "%s = %s\n", key,
                                (char *)ecore_hash_get(cfg->cfg_data, key));

        write(fd, data, len);  
     }

   /* release the lock */
   dn_config_file_unlock(fd);
   close(fd);

   return TRUE;
}

void
dn_config_string_set(Diversity_Nav_Config *cfg, const char *k, const char *v)
{
   if(!k) return;

   ecore_hash_set(cfg->cfg_data, strdup(k), strdup(v));
}

const char *
dn_config_string_get(Diversity_Nav_Config *cfg, const char *k)
{
   if(!k) return NULL;

   return ecore_hash_get(cfg->cfg_data, k);
}

void
dn_config_int_set(Diversity_Nav_Config *cfg, const char *k, int v)
{
   char buf[128];

   if(!k) return;

   snprintf(buf, sizeof(buf), "%d", v);

   ecore_hash_set(cfg->cfg_data, strdup(k), strdup(buf));
}

int
dn_config_int_get(Diversity_Nav_Config *cfg, const char *k)
{
   const char *val;
   int v = 0;

   if(!k) return v;

   val = ecore_hash_get(cfg->cfg_data, k);
   if (val) v = atoi(val);

   return v;
}

void
dn_config_float_set(Diversity_Nav_Config *cfg, const char *k, float v)
{
   char buf[128];

   if(!k) return;

   snprintf(buf, sizeof(buf), "%f", v);
     
   ecore_hash_set(cfg->cfg_data, strdup(k), strdup(buf));
}

float
dn_config_float_get(Diversity_Nav_Config *cfg, const char *k)
{
   const char *val;
   float v = 0.0;   

   if(!k) return 0.0;

   val = ecore_hash_get(cfg->cfg_data, k);
   if (val) v = atof(val);

   return v;
}

static Ecore_Hash *
dn_config_create_hash(void)
{
   Ecore_Hash *hash;

   hash = ecore_hash_new(ecore_str_hash, ecore_str_compare);
   ecore_hash_free_key_cb_set(hash, free);
   ecore_hash_free_value_cb_set(hash, free);

   return hash;
}

static void
dn_config_parse(Diversity_Nav_Config *cfg, Ecore_Hash *hash, char *data)
{
   char *start;

   if(!cfg) return;
   if(!hash) return;
   if(!data) return;

   start = data;
   while (start)
     {
        char *middle = NULL, *end, *key = NULL, *val = NULL;

        /* skip over blank space */
        while (isspace(*start) && (*start != '\0'))
          start ++;

        if (*start == '\0') break;

        /* skip over comment lines */
        if (*start == '#')
          {
             while ((*start != '\n') && (*start != '\0'))
               start ++;
             if (*start == '\0') break;

             start ++;

             continue;
          }

        /* at this point we should have an actual key/value pair */
        end = start;
        while ((*end != '\0') && (*end != '\n') && (*end != '\r'))
          {
             if (*end == '=')
               {
                  middle = end;
                  *middle = '\0';
               }
             end ++;
          }
        *end = '\0';

        if (start && middle && end)
          {
             key = strdup(start);
             key = dn_config_trim(key);

             val = strdup(middle + 1);
             val = dn_config_trim(val);

             ecore_hash_set(hash, key, val);
          }

        start = end + 1;
   }

   return;
}

static char *
dn_config_trim(char *v2)
{
   char *end, *old, *v;

   if(!v2) return NULL;

   old = v2;
   v = v2;
   end = v + strlen(v);

   /* strip from beginning */
   while (isspace(*v) && (*v != '\0')) v++;
   while ((isspace(*end) || (*end == '\0')) && (end != v)) end --;
   *(++end) = '\0';

   v2 = strdup(v);
   FREE(old);

   return v2;
}

static int
dn_config_file_read_lock(int fd)
{
   struct flock fl;

   fl.l_type = F_RDLCK;
   fl.l_whence = SEEK_SET;
   fl.l_start = 0;
   fl.l_len = 0;

   return (fcntl(fd, F_SETLKW, &fl) == 0);
}

static int
dn_config_file_write_lock(int fd)
{
   struct flock fl;

   fl.l_type = F_WRLCK;
   fl.l_whence = SEEK_SET;
   fl.l_start = 0;
   fl.l_len = 0;

   return (fcntl(fd, F_SETLKW, &fl) == 0);
}

static int
dn_config_file_unlock(int fd)
{
   struct flock fl;

   fl.l_type = F_UNLCK;
   fl.l_whence = SEEK_SET;
   fl.l_start = 0;
   fl.l_len = 0;

   return (fcntl(fd, F_SETLK, &fl) == 0);
}
