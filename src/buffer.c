/*
 *  buffer.c - Buffer helpers
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
#include "buffer.h"
#include "utils.h"

static int buffer_allocate(Buffer *buffer, unsigned int buf_size)
{
    uint8_t *data;

    data = fast_realloc(
        buffer->data,
        &buffer->data_size_max,
        buf_size
    );
    if (!data)
        return -1;

    buffer->data = data;
    return 0;
}

static inline void buffer_init(Buffer *buffer)
{
    buffer->data          = NULL;
    buffer->data_size     = 0;
    buffer->data_size_max = 0;
}

Buffer *buffer_create(unsigned int size)
{
    Buffer *buffer = malloc(sizeof(*buffer));

    if (!buffer)
        return NULL;

    buffer_init(buffer);

    if (size && buffer_allocate(buffer, size) < 0) {
        buffer_destroy(buffer);
        return NULL;
    }
    return buffer;
}

void buffer_destroy(Buffer *buffer)
{
    if (!buffer)
        return;

    if (buffer->data) {
        free(buffer->data);
        buffer_init(buffer);
    }
    free(buffer);
}

int buffer_append(Buffer *buffer, const uint8_t *buf, unsigned int buf_size)
{
    if (!buffer)
        return -1;

    if (buffer_allocate(buffer, buffer->data_size + buf_size) < 0)
        return -1;

    memcpy(buffer->data + buffer->data_size, buf, buf_size);
    buffer->data_size += buf_size;
    return 0;
}

int buffer_steal(Buffer *buffer, uint8_t **buf, unsigned int *buf_size)
{
    if (!buffer || !buf)
        return -1;

    if (buf_size)
        *buf_size = buffer->data_size;

    *buf = buffer->data;
    buffer_init(buffer);
    return 0;
}
