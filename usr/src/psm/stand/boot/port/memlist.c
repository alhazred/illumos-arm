/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2017 Hayashi Naoyuki
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/types.h>
#include <sys/salib.h>
#include <sys/memlist.h>
#include <sys/memlist_impl.h>
#include <sys/promif.h>

#ifdef DEBUG
static const int debug = 1;
#else
extern int debug;
#endif

#define	dprintf		if (debug) printf

void
memlist_dump(struct memlist *listp)
{
	prom_printf("memlist 0x%p content\n", (void *)listp);
	while (listp) {
		prom_printf("(0x%p, 0x%p)\n", listp->ml_address, listp->ml_size);
		listp = listp->ml_next;
	}
}

struct memlist *
memlist_get_one()
{
	return get_memlist_struct();
}

void
memlist_free_one(struct memlist *buf)
{
	add_to_freelist(buf);
}

/*
 * Insert into a sorted memory list.
 * new = new element to insert
 * curmemlistp = memory list to which to add segment.
 */
void
memlist_insert(
	struct memlist *new,
	struct memlist **curmemlistp)
{
	struct memlist *cur, *last;
	uint64_t start, end;

	start = new->ml_address;
	end = start + new->ml_size;
	last = NULL;
	for (cur = *curmemlistp; cur; cur = cur->ml_next) {
		last = cur;
		if (cur->ml_address >= end) {
			new->ml_next = cur;
			new->ml_prev = cur->ml_prev;
			cur->ml_prev = new;
			if (cur == *curmemlistp)
				*curmemlistp = new;
			else
				new->ml_prev->ml_next = new;
			return;
		}
		if (cur->ml_address + cur->ml_size > start)
			prom_panic("munged memory list\n");
	}
	new->ml_next = NULL;
	new->ml_prev = last;
	if (last != NULL)
		last->ml_next = new;
}

void
memlist_del(struct memlist *memlistp,
	struct memlist **curmemlistp)
{
	if (*curmemlistp == memlistp) {
		*curmemlistp = memlistp->ml_next;
	}
	if (memlistp->ml_prev != NULL) {
		memlistp->ml_prev->ml_next = memlistp->ml_next;
	}
	if (memlistp->ml_next != NULL) {
		memlistp->ml_next->ml_prev = memlistp->ml_prev;
	}
}

struct memlist *
memlist_find(struct memlist *mlp, uint64_t address)
{
	for (; mlp != NULL; mlp = mlp->ml_next)
		if (address >= mlp->ml_address &&
		    address < (mlp->ml_address + mlp->ml_size))
			break;
	return (mlp);
}

/*
 * Add a span to a memlist.
 * Return:
 * MEML_SPANOP_OK if OK.
 * MEML_SPANOP_ESPAN if part or all of span already exists
 * MEML_SPANOP_EALLOC for allocation failure
 */
int
memlist_add_span(
	uint64_t address,
	uint64_t bytes,
	struct memlist **curmemlistp)
{
	struct memlist *dst;
	struct memlist *prev, *next;

	/*
	 * allocate a new struct memlist
	 */

	dst = memlist_get_one();

	if (dst == NULL) {
		return (MEML_SPANOP_EALLOC);
	}

	dst->ml_address = address;
	dst->ml_size = bytes;

	/*
	 * First insert.
	 */
	if (*curmemlistp == NULL) {
		dst->ml_prev = NULL;
		dst->ml_next = NULL;
		*curmemlistp = dst;
		return (MEML_SPANOP_OK);
	}

	/*
	 * Insert into sorted list.
	 */
	for (prev = NULL, next = *curmemlistp; next != NULL;
	    prev = next, next = next->ml_next) {
		if (address > (next->ml_address + next->ml_size))
			continue;

		/*
		 * Else insert here.
		 */

		/*
		 * Prepend to next.
		 */
		if ((address + bytes) == next->ml_address) {
			memlist_free_one(dst);

			next->ml_address = address;
			next->ml_size += bytes;

			return (MEML_SPANOP_OK);
		}

		/*
		 * Append to next.
		 */
		if (address == (next->ml_address + next->ml_size)) {
			memlist_free_one(dst);

			if (next->ml_next) {
				/*
				 * don't overlap with next->ml_next
				 */
				if ((address + bytes) >
				    next->ml_next->ml_address) {
					return (MEML_SPANOP_ESPAN);
				}
				/*
				 * Concatenate next and next->ml_next
				 */
				if ((address + bytes) ==
				    next->ml_next->ml_address) {
					struct memlist *mlp = next->ml_next;

					if (next == *curmemlistp)
						*curmemlistp = next->ml_next;

					mlp->ml_address = next->ml_address;
					mlp->ml_size += next->ml_size;
					mlp->ml_size += bytes;

					if (next->ml_prev)
						next->ml_prev->ml_next = mlp;
					mlp->ml_prev = next->ml_prev;

					memlist_free_one(next);
					return (MEML_SPANOP_OK);
				}
			}

			next->ml_size += bytes;

			return (MEML_SPANOP_OK);
		}

		/* don't overlap with next */
		if ((address + bytes) > next->ml_address) {
			memlist_free_one(dst);
			return (MEML_SPANOP_ESPAN);
		}

		/*
		 * Insert before next.
		 */
		dst->ml_prev = prev;
		dst->ml_next = next;
		next->ml_prev = dst;
		if (prev == NULL) {
			*curmemlistp = dst;
		} else {
			prev->ml_next = dst;
		}
		return (MEML_SPANOP_OK);
	}

	/*
	 * End of list, prev is valid and next is NULL.
	 */
	prev->ml_next = dst;
	dst->ml_prev = prev;
	dst->ml_next = NULL;

	return (MEML_SPANOP_OK);
}

/*
 * Delete a span from a memlist.
 * Return:
 * MEML_SPANOP_OK if OK.
 * MEML_SPANOP_ESPAN if part or all of span does not exist
 * MEML_SPANOP_EALLOC for allocation failure
 */
int
memlist_delete_span(
	uint64_t address,
	uint64_t bytes,
	struct memlist **curmemlistp)
{
	struct memlist *dst, *next;

	/*
	 * Find element containing address.
	 */
	for (next = *curmemlistp; next != NULL; next = next->ml_next) {
		if ((address >= next->ml_address) &&
		    (address < next->ml_address + next->ml_size))
			break;
	}

	/*
	 * If start address not in list.
	 */
	if (next == NULL) {
		return (MEML_SPANOP_ESPAN);
	}

	/*
	 * Error if size goes off end of this struct memlist.
	 */
	if (address + bytes > next->ml_address + next->ml_size) {
		return (MEML_SPANOP_ESPAN);
	}

	/*
	 * Span at beginning of struct memlist.
	 */
	if (address == next->ml_address) {
		/*
		 * If start & size match, delete from list.
		 */
		if (bytes == next->ml_size) {
			if (next == *curmemlistp)
				*curmemlistp = next->ml_next;
			if (next->ml_prev != NULL)
				next->ml_prev->ml_next = next->ml_next;
			if (next->ml_next != NULL)
				next->ml_next->ml_prev = next->ml_prev;

			memlist_free_one(next);
		} else {
			/*
			 * Increment start address by bytes.
			 */
			next->ml_address += bytes;
			next->ml_size -= bytes;
		}
		return (MEML_SPANOP_OK);
	}

	/*
	 * Span at end of struct memlist.
	 */
	if (address + bytes == next->ml_address + next->ml_size) {
		/*
		 * decrement size by bytes
		 */
		next->ml_size -= bytes;
		return (MEML_SPANOP_OK);
	}

	/*
	 * Delete a span in the middle of the struct memlist.
	 */
	{
		/*
		 * create a new struct memlist
		 */
		dst = memlist_get_one();

		if (dst == NULL) {
			return (MEML_SPANOP_EALLOC);
		}

		/*
		 * Existing struct memlist gets address
		 * and size up to start of span.
		 */
		dst->ml_address = address + bytes;
		dst->ml_size =
		    (next->ml_address + next->ml_size) - dst->ml_address;
		next->ml_size = address - next->ml_address;

		/*
		 * New struct memlist gets address starting
		 * after span, until end.
		 */

		/*
		 * link in new memlist after old
		 */
		dst->ml_next = next->ml_next;
		dst->ml_prev = next;

		if (next->ml_next != NULL)
			next->ml_next->ml_prev = dst;
		next->ml_next = dst;
	}
	return (MEML_SPANOP_OK);
}

/*
 * find and claim a memory chunk of given size, first fit
 */
uint64_t
memlist_get(uint64_t size, int align, struct memlist **listp)
{
	uint64_t delta, total_size;
	uint64_t paddr;
	struct memlist *prev = 0, *next;

	/* find the chunk with sufficient size */
	next = *listp;
	while (next) {
		delta = next->ml_address & ((align != 0) ? (align - 1) : 0);
		if (delta != 0)
			total_size = size + align - delta;
		else
			total_size = size; /* the addr is already aligned */
		if (next->ml_size >= total_size)
			break;
		prev = next;
		next = prev->ml_next;
	}

	if (next == 0)
		return (0);	/* Not found */

	paddr = next->ml_address;
	if (delta)
		paddr += align - delta;
	memlist_delete_span(paddr, size, listp);

	return (paddr);
}
