/* e_mod_config.h -
 *
 * Copyright 2008 Openmoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H 

#include <Ecore.h>
#include <Ecore_Data.h>
#include <Ecore_File.h>

#define DEFAULT_VALUE_LAT 0.0
#define DEFAULT_VALUE_LON 0.0
#define DEFAULT_VALUE_SPAN 1024
#define DEFAULT_VALUE_NEO_ME_LON 121.575348
#define DEFAULT_VALUE_NEO_ME_LAT 25.073111
#define DEFAULT_VALUE_TILE_PATH "/tmp/diversity-maps"
#define DEFAULT_VALUE_GPS_DEVICE "/dev/ttySAC1:9600"

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
