;
; tss_flush
; As part of the Avery project
; Created by Max Van den Eynde in 2026
; --------------------------------------
; Description: TSS Flushing system
; Copyright (c) 2026 Max Van den Eynde
;

bits 64

global tss_flush

tss_flush:
	mov ax, 0x28
	ltr ax
	ret