/*
 *  buffer.h - Buffer helpers
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

#ifndef BUFFER_H
#define BUFFER_H

typedef struct _Buffer Buffer;

struct _Buffer {
    uint8_t            *data;
    unsigned int        data_size;
    unsigned int        data_size_max;
};

Buffer *buffer_create(unsigned int size);
void buffer_destroy(Buffer *buffer);
int buffer_append(Buffer *buffer, const uint8_t *buf, unsigned int buf_size);
int buffer_steal(Buffer *buffer, uint8_t **buf, unsigned int *buf_size);

#endif /* BUFFER_H */
