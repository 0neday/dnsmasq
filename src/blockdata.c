/* dnsmasq is Copyright (c) 2000-2014 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991, or
   (at your option) version 3 dated 29 June, 2007.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
     
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dnsmasq.h"

#ifdef HAVE_DNSSEC

static struct blockdata *keyblock_free = NULL;

struct blockdata *blockdata_alloc(char *data, size_t len)
{
  struct blockdata *block, *ret = NULL;
  struct blockdata **prev = &ret;
  size_t blen;

  while (len > 0)
    {
      if (keyblock_free)
	{
	  block = keyblock_free;
	  keyblock_free = block->next;
	}
      else
	block = whine_malloc(sizeof(struct blockdata));

      if (!block)
	{
	  /* failed to alloc, free partial chain */
	  blockdata_free(ret);
	  return NULL;
	}
      
      blen = len > KEYBLOCK_LEN ? KEYBLOCK_LEN : len;
      memcpy(block->key, data, blen);
      data += blen;
      len -= blen;
      *prev = block;
      prev = &block->next;
      block->next = NULL;
    }
  
  return ret;
}

size_t blockdata_walk(struct blockdata **key, unsigned char **p, size_t cnt)
{
  if (*p == NULL)
    *p = (*key)->key;
  else if (*p == (*key)->key + KEYBLOCK_LEN)
    {
      *key = (*key)->next;
      if (*key == NULL)
        return 0;
      *p = (*key)->key;
    }

  return MIN(cnt, (size_t)((*key)->key + KEYBLOCK_LEN - (*p)));
}

void blockdata_free(struct blockdata *blocks)
{
  struct blockdata *tmp;

  if (blocks)
    {
      for (tmp = blocks; tmp->next; tmp = tmp->next);
      tmp->next = keyblock_free;
      keyblock_free = blocks;
    }
}

void  blockdata_retrieve(struct blockdata *block, size_t len, void *data)
{
  size_t blen;
  struct  blockdata *b;
  
  for (b = block; len > 0 && b;  b = b->next)
    {
      blen = len > KEYBLOCK_LEN ? KEYBLOCK_LEN : len;
      memcpy(data, b->key, blen);
      data += blen;
      len -= blen;
    }
}
  
#endif
