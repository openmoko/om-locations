/* debug.h -
 *
 * Copyright 2008 OpenMoko, Inc.
 * Authored by Jeremy Chang <jeremy@openmoko.com>
 *
 * This work is based on e17 project.  See also COPYING.e17.
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

#ifndef __MPENV_DEBUG_H__
#define __MPENV_DEBUG_H__

#include <assert.h>

#define CONFIG_DEBUG
#ifdef CONFIG_DEBUG
#include <stdio.h>
#define debug(f, a...)                                             \
    do{                                                             \
	printf ("DEBUG: (%s, %d): %s: ",__FILE__, __LINE__, __FUNCTION__);      \
	printf (f, ## a);                               \
    } while (0)
#else
#define debug(f, a...)
#endif

#define error(f, a...)                                             \
    do{                                                             \
        printf ("ERROR: (%s, %d): %s: ",__FILE__, __LINE__, __FUNCTION__);      \
        printf (f, ## a);                               \
    } while (0)


#endif
