;/**
 ;****************************************************************************************
 ;*
 ;* @file boot_vectors.s
 ;*
 ;* @brief ARM Exception Vectors table.
 ;*
 ;* Copyright (C) RivieraWaves 2009-2015
 ;*
 ;* $Rev:  $
 ;*
 ;****************************************************************************************
 ;*/

 
	
	EXPORT vectors
	
	IMPORT sys_Reset
	
	IMPORT Undefined_Exception
		
	IMPORT Reserved_Exception
	IMPORT PrefetchAbort_Exception
	IMPORT DataAbort_Exception		
	IMPORT Irq_Exception
		
	PRESERVE8

	CODE32
    AREA SYS_BOOT, CODE, READONLY
    ENTRY
		
vectors
    ; reset handler
    LDR     PC,       boot_reset
    ; undefined handler
    LDR     PC,       boot_undefined
    ; SWI handler
    LDR     PC,		  boot_swi
    ; Prefetch error handler
    LDR     PC,       boot_pabort
    ; abort handler
    LDR     PC,       boot_dabort
    ; reserved vector
	LDR     PC,		  boot_reserved
    ; irq
    LDR     PC,       intc_irq
    ; fiq
    LDR     PC,       intc_fiq
	
boot_reset          DCD     sys_Reset
boot_undefined      DCD     Undefined
boot_swi            DCD     SoftwareInterrupt
boot_pabort        	DCD     PrefetchAbort
boot_dabort       	DCD     DataAbort
boot_reserved       DCD     sys_reserved
intc_irq            DCD     IRQ_Handler
intc_fiq            DCD     FIQ_Handler
	
I_SEL               EQU     0x00400000 
SYSirq_FIQ_Handler 	EQU     0x0001D600
SYSirq_IRQ_Handler 	EQU     0x0001D620
SoftwareInterrupt_Exception 	EQU     0x0001D640

;*******************************************************************************
;* Function Name  : UndefinedHandler
;* Description    : This function called when undefined instruction exception
;*                  is entered.
;* Input          : none
;* Output         : none
;*******************************************************************************
Undefined
    ; r0������ջ
    STMDB SP!, {R12}
    ; ��ָ��ָ��0x400004
    LDR R12, =0x400004
    ; ���� R0 �� R11
    STMIA R12!, {R0-R11}
    
    ;ȡR12
    LDR R0,[SP,#-8]
    STR R0, [R12], #4
    
    ; ���� sp,r13
    ADD R13, R13,#4
    STR R13, [R12], #4
    
    ; ���� LR,r14
    MOV R0, LR
    STR R0, [R12], #4

    ; ����pc,???? 
    MOV R0, PC
    STR R0, [R12], #4
    B       Undefined_Exception

;*******************************************************************************
;* Function Name  : SWIHandler
;* Description    : This function called when SWI instruction executed.
;* Input          : none
;* Output         : none
;*******************************************************************************
SoftwareInterrupt
;	IF :LNOT::DEF:USE_FREERTOS
;    ; r0������ջ
;    STMDB SP!, {R12}
;    ; ��ָ��ָ��0x400004
;    LDR R12, =0x400004
;    ; ���� R0 �� R11
;    STMIA R12!, {R0-R11}
;    
;    ;ȡR12
;    LDR R0,[SP,#-8]
;    STR R0, [R12], #4
;    
;    ; ���� sp,r13
;    ADD R13, R13,#4
;    STR R13, [R12], #4
;    
;    ; ���� LR,r14
;    MOV R0, LR
;    STR R0, [R12], #4

;    ; ����pc,???? 
;    MOV R0, PC
;    STR R0, [R12], #4
;	ENDIF
    B       SoftwareInterrupt_Exception

PrefetchAbort

    B       PrefetchAbort_Exception

;*******************************************************************************
;* Function Name  : DataAbortHandler
;* Description    : This function is called when Data Abort exception is entered.
;* Input          : none
;* Output         : none
;*******************************************************************************
DataAbort
	; r0������ջ
    STMDB SP!, {R12}
    ; ��ָ��ָ��0x400004
    LDR R12, =0x400004
    ; ���� R0 �� R11
    STMIA R12!, {R0-R11}
    
    ;ȡR12
    LDR R0,[SP,#-8]
    STR R0, [R12], #4
    
    ; ���� sp,r13
    ADD R13, R13,#4
    STR R13, [R12], #4
    
    ; ���� LR,r14
    MOV R0, LR
    STR R0, [R12], #4

    ; ����pc,???? 
    MOV R0, PC
    STR R0, [R12], #4
  
    B       DataAbort_Exception

;*******************************************************************************
;* Function Name  : IRQHandler
;* Description    : This function called when IRQ exception is entered.
;* Input          : none
;* Output         : none
;*******************************************************************************
IRQ_Handler	
		STMFD SP!,{R0,R1}
		LDR R0,=I_SEL
		LDR R1,[R0]
		CMP R1,#0X01
		LDMFD SP!,{R0,R1}		
		BEQ    Irq_Exception		
        B      SYSirq_IRQ_Handler

;*******************************************************************************
;* Function Name  : FIQHandler
;* Description    : This function is called when FIQ exception is entered.
;* Input          : none
;* Output         : none
;*******************************************************************************
FIQ_Handler     
        B      SYSirq_FIQ_Handler
       
        
sys_reserved
	B	   Reserved_Exception
	
	

	
	

	END