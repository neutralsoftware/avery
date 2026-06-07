;
; idt_load.asm
; Created by Max Van den Eynde
; in 2026 as part of the Avery project
;

global idt_load
extern idtp
idt_load:
	lidt [rel idtp]
	ret