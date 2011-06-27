/*
 *  utils.h - Utilities
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

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// Lookup for substring NAME in string EXT using SEP as separators
int find_string(const char *name, const char *ext, const char *sep);

const char *string_of_FOURCC(uint32_t fourcc);

void *fast_realloc(void *ptr, unsigned int *size, unsigned int min_size);

uint64_t get_ticks_usec(void);
void delay_usec(unsigned int usec);

uint32_t gen_random_int(void);
uint32_t gen_random_int_range(uint32_t begin, uint32_t end);

#endif /* UTILS_H */
