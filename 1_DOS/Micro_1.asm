WDTCH DATA 0FFH
ORG 0000H
AJMP START
ORG 0100H

START:
	MOV WDTCN, #0DEH
	MOV WDTCN, #0ADH 	;stop the watch dog circuit

	;20H with low byte of final result
	MOV R0, #20H 		
	;MOV A, #00H		
	CLR A
	MOV @R0, A 		;initialize 20H-22H
	INC R0	  
	MOV @R0, A 			
	INC R0
	MOV @R0, A 			
	;MOV R0, #20H		;initialize

	MOV DPTR, #CONST	;acquire loop numbers
	CLR A
	MOVC A,@A+DPTR
	MOV R7, A
	MOV DPTR, #0000H	;first number location
LOOP:
	;add low byte
	MOV R0, #20H
	MOV A, @R0
	MOV R1, A
	MOVX A, @DPTR
	ADD A, R1
	DA A
	MOV @R0, A
	INC DPTR

	;add high byte
	INC R0
	MOV A, @R0
	MOV R1, A
	MOVX A, @DPTR
	ADDC A, R1
	DA A
	MOV @R0, A
	INC DPTR

	;carry add
	INC R0
	MOV A, @R0
	MOV R1, A
	CLR A
	MOV ACC.0, C
	ADD A, R1
	DA A
	MOV @R0, A

	DJNZ R7, LOOP
	SJMP $	;keep looping
	
CONST:
	DW 0009H	;data 'N'
END