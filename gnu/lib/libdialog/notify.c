/*
 * File: 	notify.c
 * Author: 	Marc van Kempen
 * Desc:	display a notify box with a message
 *
 * Copyright (c) 1995, Marc van Kempen
 *
 * All rights reserved.
 *
 * This software may be used, modified, copied, distributed, and
 * sold, in both source and binary form provided that the above
 * copyright and these terms are retained, verbatim, as the first
 * lines of this file.  Under no circumstances is the author
 * responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with
 * its use.
 * 
 */


#include <dialog.h>
#include <stdio.h>

void
dialog_notify(char *msg)
/* 
 * Desc: display an error message
 */
{
    WINDOW *w;

    w = dupwin(newscr);
    if (w == NULL) {
	endwin();
	fprintf(stderr, "\ndupwin(newscr) failed, malloc memory corrupted\n");
	exit(1);
    }
    dialog_msgbox("Message", msg, -1, -1, TRUE);
    touchwin(w);
    wrefresh(w);
    delwin(w);

    return;

} /* dialog_notify() */

