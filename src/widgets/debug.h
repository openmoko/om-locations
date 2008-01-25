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
