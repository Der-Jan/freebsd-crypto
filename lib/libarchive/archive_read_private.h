/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef ARCHIVE_READ_PRIVATE_H_INCLUDED
#define	ARCHIVE_READ_PRIVATE_H_INCLUDED

#include "archive.h"
#include "archive_string.h"
#include "archive_private.h"

struct archive_read {
	struct archive	archive;

	struct archive_entry	*entry;

	/* Dev/ino of the archive being read/written. */
	dev_t		  skip_file_dev;
	ino_t		  skip_file_ino;

	/* Utility:  Pointer to a block of nulls. */
	const unsigned char	*nulls;
	size_t			 null_length;

	/*
	 * Used by archive_read_data() to track blocks and copy
	 * data to client buffers, filling gaps with zero bytes.
	 */
	const char	 *read_data_block;
	off_t		  read_data_offset;
	off_t		  read_data_output_offset;
	size_t		  read_data_remaining;

	/* Callbacks to open/read/write/close archive stream. */
	archive_open_callback	*client_opener;
	archive_read_callback	*client_reader;
	archive_skip_callback	*client_skipper;
	archive_write_callback	*client_writer;
	archive_close_callback	*client_closer;
	void			*client_data;

	/*
	 * Blocking information.  Note that bytes_in_last_block is
	 * misleadingly named; I should find a better name.  These
	 * control the final output from all compressors, including
	 * compression_none.
	 */
	int		  bytes_per_block;
	int		  bytes_in_last_block;

	/*
	 * These control whether data within a gzip/bzip2 compressed
	 * stream gets padded or not.  If pad_uncompressed is set,
	 * the data will be padded to a full block before being
	 * compressed.  The pad_uncompressed_byte determines the value
	 * that will be used for padding.  Note that these have no
	 * effect on compression "none."
	 */
	int		  pad_uncompressed;
	int		  pad_uncompressed_byte; /* TODO: Support this. */

	/* File offset of beginning of most recently-read header. */
	off_t		  header_position;

	/*
	 * Detection functions for decompression: bid functions are
	 * given a block of data from the beginning of the stream and
	 * can bid on whether or not they support the data stream.
	 * General guideline: bid the number of bits that you actually
	 * test, e.g., 16 if you test a 2-byte magic value.  The
	 * highest bidder will have their init function invoked, which
	 * can set up pointers to specific handlers.
	 */
	struct {
		int	(*bid)(const void *buff, size_t);
		int	(*init)(struct archive_read *, const void *buff, size_t);
	}	decompressors[4];
	/* Read/write data stream (with compression). */
	void	 *compression_data;		/* Data for (de)compressor. */
	int	(*compression_finish)(struct archive_read *);

	/*
	 * Read uses a peek/consume I/O model: the decompression code
	 * returns a pointer to the requested block and advances the
	 * file position only when requested by a consume call.  This
	 * reduces copying and also simplifies look-ahead for format
	 * detection.
	 */
	ssize_t	(*compression_read_ahead)(struct archive_read *,
		    const void **, size_t request);
	ssize_t	(*compression_read_consume)(struct archive_read *, size_t);
	off_t (*compression_skip)(struct archive_read *, off_t);

	/*
	 * Format detection is mostly the same as compression
	 * detection, with two significant differences: The bidders
	 * use the read_ahead calls above to examine the stream rather
	 * than having the supervisor hand them a block of data to
	 * examine, and the auction is repeated for every header.
	 * Winning bidders should set the archive_format and
	 * archive_format_name appropriately.  Bid routines should
	 * check archive_format and decline to bid if the format of
	 * the last header was incompatible.
	 *
	 * Again, write support is considerably simpler because there's
	 * no need for an auction.
	 */

	struct archive_format_descriptor {
		int	(*bid)(struct archive_read *);
		int	(*read_header)(struct archive_read *, struct archive_entry *);
		int	(*read_data)(struct archive_read *, const void **, size_t *, off_t *);
		int	(*read_data_skip)(struct archive_read *);
		int	(*cleanup)(struct archive_read *);
		void	 *format_data;	/* Format-specific data for readers. */
	}	formats[8];
	struct archive_format_descriptor	*format; /* Active format. */

	/*
	 * Storage for format-specific data.  Note that there can be
	 * multiple format readers active at one time, so we need to
	 * allow for multiple format readers to have their data
	 * available.  The pformat_data slot here is the solution: on
	 * read, it is guaranteed to always point to a void* variable
	 * that the format can use.
	 */
	void	**pformat_data;		/* Pointer to current format_data. */
	void	 *format_data;		/* Used by writers. */

	/*
	 * Pointers to format-specific functions for writing.  They're
	 * initialized by archive_write_set_format_XXX() calls.
	 */
	int	(*format_init)(struct archive *); /* Only used on write. */
	int	(*format_finish)(struct archive *);
	int	(*format_finish_entry)(struct archive *);
	int 	(*format_write_header)(struct archive *,
		    struct archive_entry *);
	ssize_t	(*format_write_data)(struct archive *,
		    const void *buff, size_t);

	/*
	 * Various information needed by archive_extract.
	 */
	struct extract		 *extract;
	int			(*cleanup_archive_extract)(struct archive_read *);
};

int	__archive_read_register_format(struct archive_read *a,
	    void *format_data,
	    int (*bid)(struct archive_read *),
	    int (*read_header)(struct archive_read *, struct archive_entry *),
	    int (*read_data)(struct archive_read *, const void **, size_t *, off_t *),
	    int (*read_data_skip)(struct archive_read *),
	    int (*cleanup)(struct archive_read *));

int	__archive_read_register_compression(struct archive_read *a,
	    int (*bid)(const void *, size_t),
	    int (*init)(struct archive_read *, const void *, size_t));

#endif
