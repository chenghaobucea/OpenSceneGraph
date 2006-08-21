/*
 * The 3D Studio File Format Library
 * Copyright (C) 1996-2001 by J.E. Hoffmann <je-h@gmx.net>
 * All rights reserved.
 *
 * This program is  free  software;  you can redistribute it and/or modify it
 * under the terms of the  GNU Lesser General Public License  as published by 
 * the  Free Software Foundation;  either version 2.1 of the License,  or (at 
 * your option) any later version.
 *
 * This  program  is  distributed in  the  hope that it will  be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A  PARTICULAR PURPOSE.  See the  GNU Lesser General Public  
 * License for more details.
 *
 * You should  have received  a copy of the GNU Lesser General Public License
 * along with  this program;  if not, write to the  Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */
#define LIB3DS_EXPORT
#include "chunk.h"
#include "readwrite.h"
#include "chunktable.h"
#include <string.h>
#include <stdarg.h>


/*#define LIB3DS_CHUNK_DEBUG*/
/*#define LIB3DS_CHUNK_WARNING*/


/*!
 * \defgroup chunk Chunk Handling
 *
 * \author J.E. Hoffmann <je-h@gmx.net>
 */


static Lib3dsBool enable_dump=LIB3DS_FALSE;
static Lib3dsBool enable_unknown=LIB3DS_FALSE;
static char lib3ds_chunk_level[128]="";


static void
lib3ds_chunk_debug_enter(Lib3dsChunk *)
{
  strcat(lib3ds_chunk_level, "  ");
}


static void
lib3ds_chunk_debug_leave(Lib3dsChunk *)
{
  lib3ds_chunk_level[strlen(lib3ds_chunk_level)-2]=0;
}


static void
lib3ds_chunk_debug_dump(Lib3dsChunk *c)
{
  if (enable_dump) {
    printf("%s%s (0x%X) size=%u\n",
      lib3ds_chunk_level,
      lib3ds_chunk_name(c->chunk),
      c->chunk,
      c->size
    );
  }
}


/*!
 * \ingroup chunk
 */
void
lib3ds_chunk_enable_dump(Lib3dsBool enable, Lib3dsBool unknown)
{
  enable_dump=enable;
  enable_unknown=unknown;
}


/*!
 * \ingroup chunk
 *
 * Reads a 3d-Studio chunk header from a little endian file stream.
 *
 * \param c  The chunk to store the data.
 * \param f  The file stream.
 *
 * \return   True on success, False otherwise.
 */
Lib3dsBool
lib3ds_chunk_read(Lib3dsChunk *c, FILE *f)
{
  ASSERT(c);
  ASSERT(f);
  c->cur=ftell(f);
  c->chunk=lib3ds_word_read(f);
  c->size=lib3ds_dword_read(f);
  c->end=c->cur+c->size;
  c->cur+=6;
  if (ferror(f) || (c->size<6)) {
    return(LIB3DS_FALSE);
  }
  return(LIB3DS_TRUE);
  
}


/*!
 * \ingroup chunk
 */
Lib3dsBool
lib3ds_chunk_read_start(Lib3dsChunk *c, Lib3dsWord chunk, FILE *f)
{
  ASSERT(c);
  ASSERT(f);
  if (!lib3ds_chunk_read(c, f)) {
    return(LIB3DS_FALSE);
  }
  lib3ds_chunk_debug_enter(c);
  return((chunk==0) || (c->chunk==chunk));
}


/*!
 * \ingroup chunk
 */
void
lib3ds_chunk_read_tell(Lib3dsChunk *c, FILE *f)
{
  c->cur=ftell(f);
}


/*!
 * \ingroup chunk
 */
Lib3dsWord
lib3ds_chunk_read_next(Lib3dsChunk *c, FILE *f)
{
  Lib3dsChunk d;

  if (c->cur>=c->end) {
    ASSERT(c->cur==c->end);
    return(0);
  }

  fseek(f, (long)c->cur, SEEK_SET);
  d.chunk=lib3ds_word_read(f);
  d.size=lib3ds_dword_read(f);
  lib3ds_chunk_debug_dump(&d);
  c->cur+=d.size;
  return(d.chunk);
}


/*!
 * \ingroup chunk
 */
void
lib3ds_chunk_read_reset(Lib3dsChunk *, FILE *f)
{
  fseek(f, -6, SEEK_CUR);
}


/*!
 * \ingroup chunk
 */
void
lib3ds_chunk_read_end(Lib3dsChunk *c, FILE *f)
{
  lib3ds_chunk_debug_leave(c);
  fseek(f, c->end, SEEK_SET);
}


/*!
 * \ingroup chunk
 *
 * Writes a 3d-Studio chunk header into a little endian file stream.
 *
 * \param c  The chunk to be written.
 * \param f  The file stream.
 *
 * \return   True on success, False otherwise.
 */
Lib3dsBool
lib3ds_chunk_write(Lib3dsChunk *c, FILE *f)
{
  ASSERT(c);
  if (!lib3ds_word_write(c->chunk, f)) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  if (!lib3ds_dword_write(c->size, f)) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup chunk
 */
Lib3dsBool
lib3ds_chunk_write_start(Lib3dsChunk *c, FILE *f)
{
  ASSERT(c);
  c->size=0;
  c->cur=ftell(f);
  if (!lib3ds_word_write(c->chunk, f)) {
    return(LIB3DS_FALSE);
  }
  if (!lib3ds_dword_write(c->size, f)) {
    return(LIB3DS_FALSE);
  }
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup chunk
 */
Lib3dsBool
lib3ds_chunk_write_end(Lib3dsChunk *c, FILE *f)
{
  ASSERT(c);
  c->size=ftell(f) - c->cur;
  fseek(f, c->cur+2, SEEK_SET);
  if (!lib3ds_dword_write(c->size, f)) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }

  c->cur+=c->size;
  fseek(f, c->cur, SEEK_SET);
  if (ferror(f)) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup chunk
 */
const char*
lib3ds_chunk_name(Lib3dsWord chunk)
{
  Lib3dsChunkTable *p;

  for (p=lib3ds_chunk_table; p->name!=0; ++p) {
    if (p->chunk==chunk) {
      return(p->name);
    }
  }
  return("***UNKNOWN***");
}


/*!
 * \ingroup chunk
 */
void
lib3ds_chunk_unknown(Lib3dsWord chunk)
{
  if (enable_unknown) {
    printf("%s***WARNING*** Unknown Chunk: %s (0x%X)\n",
      lib3ds_chunk_level,
      lib3ds_chunk_name(chunk),
      chunk
    );
  }
}


/*!
 * \ingroup chunk
 */
void 
lib3ds_chunk_dump_info(const char *format, ...)
{
  if (enable_dump) {
    char s[1024];
    va_list marker;

    va_start(marker, format);
    vsprintf(s, format, marker);
    va_end(marker);

    printf("%s%s\n", lib3ds_chunk_level, s);
  }
}







