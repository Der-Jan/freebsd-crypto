/*
 * Copyright (c) 2002, Jeffrey Roberson <jeff@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 *
 */

/* 
 *
 * This file includes definitions, structures, prototypes, and inlines used
 * when debugging users of the UMA interface.
 *
 */

#ifndef VM_UMA_DBG_H
#define VM_UMA_DBG_H

void trash_ctor(void *mem, int size, void *arg);
void trash_dtor(void *mem, int size, void *arg);
void trash_init(void *mem, int size);
void trash_fini(void *mem, int size);

/* For use only by malloc */
void mtrash_ctor(void *mem, int size, void *arg);
void mtrash_dtor(void *mem, int size, void *arg);
void mtrash_init(void *mem, int size);
void mtrash_fini(void *mem, int size);

void uma_dbg_free(uma_zone_t zone, uma_slab_t slab, void *item);
void uma_dbg_alloc(uma_zone_t zone, uma_slab_t slab, void *item);

#endif /* VM_UMA_DBG_H */
