;;; This is just a slightly modified version of asm-mode for use
;;; with vu assembly code, particularly 2-column format... Tyler Daniel

;;; asm-mode.el --- mode for editing assembler code

;; Copyright (C) 1991 Free Software Foundation, Inc.

;; Author: Eric S. Raymond <esr@snark.thyrsus.com>
;; Maintainer: FSF
;; Keywords: tools, languages

;; This file is part of GNU Emacs.

;; GNU Emacs is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; GNU Emacs is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs; see the file COPYING.  If not, write to the
;; Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.

;;; Commentary:

;; This mode was written by Eric S. Raymond <esr@snark.thyrsus.com>,
;; inspired by an earlier asm-mode by Martin Neitzel.

;; This minor mode is based on text mode.  It defines a private abbrev table
;; that can be used to save abbrevs for assembler mnemonics.  It binds just
;; five keys:
;;
;;	TAB		tab to next tab stop
;;	:		outdent preceding label, tab to tab stop
;;	comment char	place or move comment
;;			asm-comment-char specifies which character this is;
;;			you can use a different character in different
;;			Asm mode buffers.
;;	C-j, C-m	newline and tab to tab stop
;;
;; Code is indented to the first tab stop level.

;; This mode runs two hooks:
;;   1) An asm-mode-set-comment-hook before the part of the initialization
;; depending on asm-comment-char, and
;;   2) an asm-mode-hook at the end of initialization.

;;; Code:

(defgroup vuasm nil
  "Mode for editing ps2 vu assembler code."
  :group 'languages)

(defcustom vuasm-comment-char ?;
  "*The comment-start character assumed by Asm mode."
  :type 'character
  :group 'asm)

(defvar vuasm-mode-syntax-table nil
  "Syntax table used while in Asm mode.")

(defvar vuasm-mode-abbrev-table nil
  "Abbrev table used while in Asm mode.")
(define-abbrev-table 'vuasm-mode-abbrev-table ())

(defvar vuasm-mode-map nil
  "Keymap for Asm mode.")

(if vuasm-mode-map
    nil
  (setq vuasm-mode-map (make-sparse-keymap))
  ;; Note that the comment character isn't set up until vuasm-mode is called.
  (define-key vuasm-mode-map ":"		'vuasm-colon)
  (define-key vuasm-mode-map "\C-c;"      'comment-region)
  (define-key vuasm-mode-map "\C-i"	'tab-to-tab-stop)
  (define-key vuasm-mode-map "\C-j"	'vuasm-newline)
  ; (define-key vuasm-mode-map "\C-m"	'vuasm-newline)
  )

(defconst vuasm-font-lock-keywords
 '(("^\\(\\(\\sw\\|\\s_\\)+\\)\\>:?[ \t]*\\(\\sw+\\)?"
    (1 font-lock-function-name-face) (3 font-lock-keyword-face nil t))
   ; these two are for assembler directives (beginning with '.')
   ("[ \t]+\\(\\.[^ \t]+\\)[ \t]*" 1 font-lock-type-face)
   ("^\\(\\.[^ \t]+\\)[ \t]*" 1 font-lock-type-face)
   ; instructions with either no arguments or one
   ("\\(\\(\\bb\\b\\|\\|\\bjr\\|nop\\|waitq\\|waitp\\|loi\\|xtop\\|xgkick\\|\\bB\\b\\|\\bJR\\b\\|\\|NOP\\|WAITQ\\|WAITP\\|LOI\\|XTOP\\|XGKICK\\)\\(\\[[edt]\\]\\)*\\)[ \t\n]+"
    1 font-lock-keyword-face)
   ; all other instructions
   ("\\([^ \t,]+\\)[ \t]+[^ \t]+,"
    1 font-lock-keyword-face))
 "Additional expressions to highlight in Assembler mode.")

(defvar vuasm-code-level-empty-comment-pattern nil)
(defvar vuasm-flush-left-empty-comment-pattern nil)
(defvar vuasm-inline-empty-comment-pattern nil)

;;;###autoload
(defun vuasm-mode ()
  "Major mode for editing vu assembler code.
Features a private abbrev table and the following bindings:

\\[vuasm-colon]\toutdent a preceding label, tab to next tab stop.
\\[tab-to-tab-stop]\ttab to next tab stop.
\\[vuasm-newline]\tnewline, then tab to next tab stop.
\\[vuasm-comment]\tsmart placement of assembler comments.

The character used for making comments is set by the variable
`vuasm-comment-char' (which defaults to `?;').

Alternatively, you may set this variable in `vuasm-mode-set-comment-hook',
which is called near the beginning of mode initialization.

Turning on Asm mode runs the hook `vuasm-mode-hook' at the end of initialization.

Special commands:
\\{vuasm-mode-map}
"
  (interactive)
  (kill-all-local-variables)
  (setq mode-name "VU Assembler")
  (setq major-mode 'vuasm-mode)
  (setq local-abbrev-table vuasm-mode-abbrev-table)
  (make-local-variable 'font-lock-defaults)
  (setq font-lock-defaults '(vuasm-font-lock-keywords))
  (make-local-variable 'tab-width)
  (setq tab-width 5)
  (make-local-variable 'tab-stop-list)
  (setq tab-stop-list '(5 20 35 70 110))
  (make-local-variable 'vuasm-mode-syntax-table)
  (setq vuasm-mode-syntax-table (make-syntax-table))
  (set-syntax-table vuasm-mode-syntax-table)

  (run-hooks 'vuasm-mode-set-comment-hook)
  ;; Make our own local child of vuasm-mode-map
  ;; so we can define our own comment character.
  (use-local-map (nconc (make-sparse-keymap) vuasm-mode-map))
  (local-set-key (vector vuasm-comment-char) 'vuasm-comment)

  (modify-syntax-entry	vuasm-comment-char
			"<" vuasm-mode-syntax-table)
  (modify-syntax-entry	?\n
			 ">" vuasm-mode-syntax-table)
  (let ((cs (regexp-quote (char-to-string vuasm-comment-char))))
    (make-local-variable 'comment-start)
    (setq comment-start (concat cs " "))
    (make-local-variable 'comment-start-skip)
    (setq comment-start-skip (concat cs "+[ \t]*"))
    (setq vuasm-inline-empty-comment-pattern (concat "^.+" cs "+ *$"))
    (setq vuasm-code-level-empty-comment-pattern (concat "^[\t ]+" cs cs " *$"))
    (setq vuasm-flush-left-empty-comment-pattern (concat "^" cs cs cs " *$"))
    )
  (make-local-variable 'comment-end)
  (setq comment-end "")
  (make-local-variable 'comment-column)
  (setq comment-column 32)
  (setq fill-prefix "\t")
  (run-hooks 'vuasm-mode-hook))

(defun vuasm-colon ()
  "Insert a colon; if it follows a label, delete the label's indentation."
  (interactive)
  (save-excursion
    (beginning-of-line)
    (if (looking-at "[ \t]+\\(\\sw\\|\\s_\\)+$")
	(delete-horizontal-space)))
  (insert ":")
  (tab-to-tab-stop)
  )

(defun vuasm-newline ()
  "Insert LFD + fill-prefix, to bring us back to code-indent level."
  (interactive)
  (if (eolp) (delete-horizontal-space))
  (insert "\n")
  (tab-to-tab-stop)
  )

(defun vuasm-line-matches (pattern &optional withcomment)
  (save-excursion
    (beginning-of-line)
    (looking-at pattern)))

(defun vuasm-pop-comment-level ()
  ;; Delete an empty comment ending current line.  Then set up for a new one,
  ;; on the current line if it was all comment, otherwise above it
  (end-of-line)
  (delete-horizontal-space)
  (while (= (preceding-char) vuasm-comment-char)
    (delete-backward-char 1))
  (delete-horizontal-space)
  (if (bolp)
      nil
    (beginning-of-line)
    (open-line 1))
  )


(defun vuasm-comment ()
  "Convert an empty comment to a `larger' kind, or start a new one.
These are the known comment classes:

   1 -- comment to the right of the code (at the comment-column)
   2 -- comment on its own line, indented like code
   3 -- comment on its own line, beginning at the left-most column.

Suggested usage:  while writing your code, trigger vuasm-comment
repeatedly until you are satisfied with the kind of comment."
  (interactive)
  (cond

   ;; Are we trying to comment-out some stuff?
   ;; If so just insert the comment-char (no funny stuff).
   ((looking-at "[ \t]*[^ \t\n]+")
    (insert vuasm-comment-char))

   ;; Blank line?  Then start comment at code indent level.
   ((vuasm-line-matches "^[ \t]*$")
    (delete-horizontal-space)
    (tab-to-tab-stop)
    (insert comment-start))

   ;; Nonblank line with no comment chars in it?
   ;; Then start a comment at the current comment column
   ; ((vuasm-line-matches (format "^[^%c\n]+$" vuasm-comment-char))
   ;  (indent-for-comment))

   ;; Flush-left comment present?  Just insert character.
   ; ((vuasm-line-matches vuasm-flush-left-empty-comment-pattern)
   ;  (insert vuasm-comment-char))

   ;; Empty code-level comment already present?
   ;; Then start flush-left comment, on line above if this one is nonempty. 
   ; ((vuasm-line-matches vuasm-code-level-empty-comment-pattern)
   ;  (vuasm-pop-comment-level)
   ;  (insert vuasm-comment-char vuasm-comment-char comment-start))

   ;; Empty comment ends line?
   ;; Then make code-level comment, on line above if this one is nonempty. 
   ((vuasm-line-matches vuasm-inline-empty-comment-pattern)
    (vuasm-pop-comment-level)
    (insert comment-start))

   ;; If all else fails, insert character
   (t
    (insert vuasm-comment-char))

   )
  ; (end-of-line)
  )

(provide 'vuasm-mode)

;;; vuasm-mode.el ends here
