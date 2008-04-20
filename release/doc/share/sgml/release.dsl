<!-- $FreeBSD$ -->

<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY % output.html		"IGNORE">
<!ENTITY % output.print 	"IGNORE">
<!ENTITY % include.historic	"IGNORE">
<!ENTITY % no.include.historic	"IGNORE">
<!ENTITY freebsd.dsl PUBLIC "-//FreeBSD//DOCUMENT DocBook Stylesheet//EN" CDATA DSSSL>
<!ENTITY % release.ent PUBLIC "-//FreeBSD//ENTITIES Release Specification//EN">
%release.ent;
]>

<style-sheet>
  <style-specification use="docbook">
    <style-specification-body>

; Configure behavior of this stylesheet
<![ %include.historic; [
      (define %include-historic% #t)
]]>
<![ %no.include.historic; [
      (define %include-historic% #f)
]]>

; String manipulation functions
(define (split-string-to-list STR)
  ;; return list of STR separated with char #\ or #\,
  (if (string? STR)
      (let loop ((i (string-delim-index STR)))
        (cond ((equal? (cdr i) '()) '())
              (else (cons (substring STR (list-ref i 0) (- (list-ref i 1) 1))
                          (loop (cdr i))))))
      '()))

(define (string-delim-index STR)
  ;; return indexes of STR separated with char #\ or #\,
  (if (string? STR)
      (let ((strlen (string-length STR)))
        (let loop ((i 0))
          (cond ((= i strlen) (cons (+ strlen 1) '()))
                ((= i 0)      (cons i (loop (+ i 1))))
                ((or (equal? (string-ref STR i) #\ )
                     (equal? (string-ref STR i) #\,)) (cons (+ i 1) (loop (+ i 1))))
                (else (loop (+ i 1))))))
      '()
      ))

(define (string-list-match? STR STR-LIST)
  (let loop ((s STR-LIST))
    (cond
     ((equal? s #f) #f)
     ((equal? s '()) #f)
     ((equal? (car s) #f) #f)
     ((equal? STR (car s)) #t)
     (else (loop (cdr s))))))

; Deal with conditional inclusion of text via entities.
(default
  (let* ((role (attribute-string (normalize "role")))
	 (for-arch (entity-text "arch")))
    (cond

     ;; If role=historic, and we're not printing historic things, then
     ;; don't output this element.
     ((and (equal? role "historic")
          (not %include-historic%))
      (empty-sosofo))

     ;; None of the above
     (else (next-match)))))

(mode qandatoc
  (default
    (let* ((role (attribute-string (normalize "role")))
	   (for-arch (entity-text "arch")))
      (cond

       ;; If role=historic, and we're not printing historic things, then
       ;; don't output this element.
       ((and (equal? role "historic")
	     (not %include-historic%))
	(empty-sosofo))

       ;; None of the above
       (else (next-match))))))

;; $paragraph$ function with arch attribute support.
(define ($paragraph$ #!optional (para-wrapper "P"))
  (let ((footnotes (select-elements (descendants (current-node))
                                    (normalize "footnote")))
        (tgroup (have-ancestor? (normalize "tgroup")))
	(arch (attribute-string (normalize "arch")))
	(role (attribute-string (normalize "role")))
	(arch-string (entity-text "arch"))
	(merged-string (entity-text "merged")))
    (make sequence
      (make element gi: para-wrapper
            attributes: (append
                         (if %default-quadding%
                             (list (list "ALIGN" %default-quadding%))
                             '()))
	    (make sequence
	      (cond
	       ;; If arch= not specified, then print unconditionally.  This clause
	       ;; handles the majority of cases.
	       ((or (equal? arch #f)
		    (equal? arch "")
		    (equal? arch "all"))
		(process-children))
	       (else
		(sosofo-append
		 (make sequence
		   (literal "[")
		   (let loop ((prev (car (split-string-to-list arch)))
			      (rest (cdr (split-string-to-list arch))))
		     (make sequence
		       (literal prev)
		       (if (not (null? rest))
			   (make sequence
			     (literal ", ")
			     (loop (car rest) (cdr rest)))
			   (empty-sosofo))))
		   (literal "] ")
		   (process-children)
		   (if (and (not (null? role)) (equal? role "merged"))
		       (literal " [" merged-string "]")
		       (empty-sosofo))))))
	      (if (or %footnotes-at-end% tgroup (node-list-empty? footnotes))
		  (empty-sosofo)
		  (make element gi: "BLOCKQUOTE"
			attributes: (list
				     (list "CLASS" "FOOTNOTES"))
			(with-mode footnote-mode
			  (process-node-list footnotes)))))))))

; We might have some sect1 level elements where the modification times
; are significant.  An example of this is the "What's New" section in
; the release notes.  We enable the printing of pubdate entry in
; sect1info elements to support this.
(element (sect1info pubdate) (process-children))

    <![ %output.print; [
; Put URLs in footnotes, and put footnotes at the bottom of each page.
      (define bop-footnotes #t)
      (define %footnote-ulinks% #t)
    ]]>

    <![ %output.html; [
      (define %callout-graphics%
	;; Use graphics in callouts?
	#f)

	<!-- Convert " ... " to `` ... '' in the HTML output. -->
	(element quote
	  (make sequence
	    (literal "&#8220;")
	    (process-children)
	    (literal "&#8221;")))

        <!-- Specify how to generate the man page link HREF -->
        (define ($create-refentry-xref-link$ #!optional (n (current-node)))
          (let* ((r (select-elements (children n) (normalize "refentrytitle")))
                 (m (select-elements (children n) (normalize "manvolnum")))
                 (v (attribute-string (normalize "vendor") n))
                 (u (string-append "&release.man.url;?query="
                         (data r) "&" "sektion=" (data m))))
            (case v
              (("xfree86") (string-append u "&" "manpath=XFree86+&release.manpath.xfree86;" ))
              (("xorg")    (string-append u "&" "manpath=Xorg+&release.manpath.xorg;" ))
              (("netbsd")  (string-append u "&" "manpath=NetBSD+&release.manpath.netbsd;"))
              (("ports")   (string-append u "&" "manpath=FreeBSD+&release.manpath.freebsd-ports;"))
              (else        (string-append u "&" "manpath=FreeBSD+&release.manpath.freebsd;")))))
    ]]>

      (define (toc-depth nd)
        (if (string=? (gi nd) (normalize "book"))
            3
            3))

    </style-specification-body>
  </style-specification>

  <external-specification id="docbook" document="freebsd.dsl">
</style-sheet>
