/*
 *  utils.c - Utilities
 *
 *  hwdecode-demos (C) 2009-2010 Splitted-Desktop Systems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include <time.h>
#include <errno.h>
#include "utils.h"

// For NetBSD with broken pthreads headers
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

// Lookup for substring NAME in string EXT using SEP as separators
int find_string(const char *name, const char *ext, const char *sep)
{
    const char *end;
    int name_len, n;

    if (!name || !ext)
        return 0;

    end = ext + strlen(ext);
    name_len = strlen(name);
    while (ext < end) {
        n = strcspn(ext, sep);
        if (n == name_len && strncmp(name, ext, n) == 0)
            return 1;
        ext += (n + 1);
    }
    return 0;
}

const char *string_of_FOURCC(uint32_t fourcc)
{
    static int buf;
    static char str[2][5];

    buf ^= 1;
    str[buf][0] = fourcc;
    str[buf][1] = fourcc >> 8;
    str[buf][2] = fourcc >> 16;
    str[buf][3] = fourcc >> 24;
    str[buf][4] = '\0';
    return str[buf];
}

void *fast_realloc(void *ptr, unsigned int *size, unsigned int min_size)
{
    if (min_size < *size)
        return ptr;

    *size= MAX(17*min_size/16 + 32, min_size);

    if ((ptr = realloc(ptr, *size)) == NULL)
        *size = 0;

    return ptr;
}

uint64_t get_ticks_usec(void)
{
#ifdef HAVE_CLOCK_GETTIME
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (uint64_t)t.tv_sec * 1000000 + t.tv_nsec / 1000;
#else
    struct timeval t;
    gettimeofday(&t, NULL);
    return (uint64_t)t.tv_sec * 1000000 + t.tv_usec;
#endif
}

#if defined(__linux__)
// Linux select() changes its timeout parameter upon return to contain
// the remaining time. Most other unixen leave it unchanged or undefined.
#define SELECT_SETS_REMAINING
#elif defined(__FreeBSD__) || defined(__sun__) || (defined(__MACH__) && defined(__APPLE__))
#define USE_NANOSLEEP
#elif defined(HAVE_PTHREADS) && defined(sgi)
// SGI pthreads has a bug when using pthreads+signals+nanosleep,
// so instead of using nanosleep, wait on a CV which is never signalled.
#include <pthread.h>
#define USE_COND_TIMEDWAIT
#endif

void delay_usec(unsigned int usec)
{
    int was_error;

#if defined(USE_NANOSLEEP)
    struct timespec elapsed, tv;
#elif defined(USE_COND_TIMEDWAIT)
    // Use a local mutex and cv, so threads remain independent
    pthread_cond_t delay_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t delay_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct timespec elapsed;
    uint64_t future;
#else
    struct timeval tv;
#ifndef SELECT_SETS_REMAINING
    uint64_t then, now, elapsed;
#endif
#endif

    // Set the timeout interval - Linux only needs to do this once
#if defined(SELECT_SETS_REMAINING)
    tv.tv_sec = 0;
    tv.tv_usec = usec;
#elif defined(USE_NANOSLEEP)
    elapsed.tv_sec = 0;
    elapsed.tv_nsec = usec * 1000;
#elif defined(USE_COND_TIMEDWAIT)
    future = get_ticks_usec() + usec;
    elapsed.tv_sec = future / 1000000;
    elapsed.tv_nsec = (future % 1000000) * 1000;
#else
    then = get_ticks_usec();
#endif

    do {
        errno = 0;
#if defined(USE_NANOSLEEP)
        tv.tv_sec = elapsed.tv_sec;
        tv.tv_nsec = elapsed.tv_nsec;
        was_error = nanosleep(&tv, &elapsed);
#elif defined(USE_COND_TIMEDWAIT)
        was_error = pthread_mutex_lock(&delay_mutex);
        was_error = pthread_cond_timedwait(&delay_cond, &delay_mutex, &elapsed);
        was_error = pthread_mutex_unlock(&delay_mutex);
#else
#ifndef SELECT_SETS_REMAINING
        // Calculate the time interval left (in case of interrupt)
        now = get_ticks_usec();
        elapsed = now - then;
        then = now;
        if (elapsed >= usec)
            break;
        usec -= elapsed;
        tv.tv_sec = 0;
        tv.tv_usec = usec;
#endif
        was_error = select(0, NULL, NULL, NULL, &tv);
#endif
    } while (was_error && (errno == EINTR));
}

uint32_t gen_random_int(void)
{
    static int initialized = 0;

    if (!initialized) {
        srand(time(NULL));
        initialized = 1;
    }
    return rand();
}

// Return a number in the range [ begin .. end - 1 ]
uint32_t gen_random_int_range(uint32_t begin, uint32_t end)
{
    return (begin +
            (uint32_t)((double)gen_random_int() /
                       (RAND_MAX / (double)(end - begin) + 1.0)));
}
