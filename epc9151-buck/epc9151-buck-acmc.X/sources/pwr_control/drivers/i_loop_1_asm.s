
; **********************************************************************************
;  SDK Version: z-Domain Control Loop Designer v0.9.10.501
;  AGS Version: Assembly Generator Script v2.0.23 (08/31/2020)
;  Author:      Yuanzhe
;  Date/Time:   09/08/2020 15:13:38
; **********************************************************************************
;  2P2Z Control Library File (Dual Bitshift-Scaling Mode)
; **********************************************************************************
	
;------------------------------------------------------------------------------
;file start
	.nolist
	.list
	
;------------------------------------------------------------------------------
;local inclusions.
	.section .data    ; place constant data in the data section
	
;------------------------------------------------------------------------------
; Define status flags bit positions
	.equ NPNZ16_STATUS_ENABLED,      15    ; bit position of the ENABLE control bit
	.equ NPNZ16_STATUS_INVERT_INPUT, 14    ; bit position of the INVERT_INPUT control bit
	.equ NPNZ16_STATUS_SWAP_SOURCE,  13    ; bit position of the SWAP_SOURCE control bit
	.equ NPNZ16_STATUS_SWAP_TARGET,  12    ; bit position of the SWAP_TARGET control bit
	.equ NPNZ16_STATUS_AGC_ENABLED,  11    ; bit position of the AGC_ENABLED control bit
	.equ NPNZ16_STATUS_USAT,         1    ; bit position of the UPPER_SATURATION_FLAG status bit
	.equ NPNZ16_STATUS_LSAT,         0    ; bit position of the LOWER_SATURATION_FLAG status bit
	
;------------------------------------------------------------------------------
; NPNZ16b_t data structure address offset declarations for data structure addressing
	.equ Status,                    0    ; controller object status word at address-offset = 0
	.equ ptrSourceRegister,         2    ; parameter group Ports.Source: pointer to source memory address
	.equ SourceNormShift,           4    ; parameter group Ports.Source: bit-shift scaler of normalization factor
	.equ SourceNormFactor,          6    ; parameter group Ports.Source: Q15 normalization factor
	.equ SourceOffset,              8    ; parameter group Ports.Source: value of source input signal/value offset
	.equ ptrAltSourceRegister,      10    ; parameter group Ports.AltSource: pointer to alternate source memory address
	.equ AltSourceNormShift,        12    ; parameter group Ports.AltSource: bit-shift scaler of normalization factor
	.equ AltSourceNormFactor,       14    ; parameter group Ports.AltSource: Q15 normalization factor
	.equ AltSourceOffset,           16    ; parameter group Ports.AltSource: value of alternate source input signal/value offset
	.equ ptrTargetRegister,         18    ; parameter group Ports.Target: pointer to target memory address
	.equ TargetNormShift,           20    ; parameter group Ports.Target: bit-shift scaler of normalization factor
	.equ TargetNormFactor,          22    ; parameter group Ports.Target: Q15 normalization factor
	.equ TargetOffset,              24    ; parameter group Ports.Target: value of target output signal/value offset
	.equ ptrAltTargetRegister,      26    ; parameter group Ports.AltTarget: pointer to alternate target memory address
	.equ AltTargetNormShift,        28    ; parameter group Ports.AltTarget: bit-shift scaler of normalization factor
	.equ AltTargetNormFactor,       30    ; parameter group Ports.AltTarget: Q15 normalization factor
	.equ AltTargetOffset,           32    ; parameter group Ports.AltTarget: value of alternate target output sigal/value offset
	.equ ptrControlReference,       34    ; parameter group Ports.ConrolReference: pointer to control reference variable/register memory address
	.equ ptrACoefficients,          36    ; parameter group Filter: pointer to A-coefficients array start address
	.equ ptrBCoefficients,          38    ; parameter group Filter: pointer to B-coefficients array start address
	.equ ptrControlHistory,         40    ; parameter group Filter: pointer to control history array start address
	.equ ptrErrorHistory,           42    ; parameter group Filter: pointer to error history array start address
	.equ ACoefficientsArraySize,    44    ; parameter group Filter: size of the A-coefficients array
	.equ BCoefficientsArraySize,    46    ; parameter group Filter: size of the B-coefficients array
	.equ ControlHistoryArraySize,   48    ; parameter group Filter: size of the control history array
	.equ ErrorHistoryArraySize,     50    ; parameter group Filter: size of the error history array
	.equ normPreShift,              52    ; parameter group Filter: value of input value normalization bit-shift scaler
	.equ normPostShiftA,            54    ; parameter group Filter: value of A-term normalization bit-shift scaler
	.equ normPostShiftB,            56    ; parameter group Filter: value of B-term normalization bit-shift scaler
	.equ normPostScaler,            58    ; parameter group Filter: control loop output normalization factor
	.equ PTermScaler,               60    ; parameter group Filter: P-Term coefficient scaler
	.equ PTermFactor,               62    ; parameter group Filter: P-Term coefficient fractional factor
	.equ MinOutput,                 64    ; parameter group Limits: minimum clamping value of primary control output
	.equ MaxOutput,                 66    ; parameter group Limits: maximum clamping value of primary control output
	.equ AltMinOutput,              68    ; parameter group Limits: minimum clamping value of alternate control output
	.equ AltMaxOutput,              70    ; parameter group Limits: maximum clamping value of alternate control output
	.equ ptrADCTriggerARegister,    72    ; parameter group ADCTriggerControl: pointer to ADC trigger A register memory address
	.equ ADCTriggerAOffset,         74    ; parameter group ADCTriggerControl: value of ADC trigger A offset
	.equ ptrADCTriggerBRegister,    76    ; parameter group ADCTriggerControl: pointer to ADC trigger B register memory address
	.equ ADCTriggerBOffset,         78    ; parameter group ADCTriggerControl: value of ADC trigger B offset
	.equ ptrDProvControlInput,      80    ; parameter group DataProviders: pointer to external variable/register the most recent, raw control input will be pushed to
	.equ ptrDProvControlInputComp,  82    ; parameter group DataProviders: pointer to external variable/register the most recent, compensated control input will be pushed to
	.equ ptrDProvControlError,      84    ; parameter group DataProviders: pointer to external variable/register the most recent control error will be pushed to
	.equ ptrDProvControlOutput,     86    ; parameter group DataProviders: pointer to external variable/register the most recent control output will be pushed to
	.equ ptrCascadedFunction,       88    ; parameter group CascadeTrigger: pointer to external, cascaded function which will be called by this controller
	.equ CascadedFunctionParam,     90    ; parameter group CascadeTrigger: 16-bit wide function parameter or pointer to a parameter data structure of cascaded function
	.equ AgcScaler,                 92    ; parameter group GainControl: bit-shift scaler of Adaptive Gain Control Modulation factor
	.equ AgcFactor,                 94    ; parameter group GainControl: Q15 value of Adaptive Gain Control Modulation factor
	.equ AgcMedian,                 96    ; parameter group GainControl: Q15 value of Adaptive Gain Control Modulation norminal operating point
	.equ ptrAgcObserverFunction,    98    ; parameter group GainControl: function pointer to observer function updating the AGC modulation factor
	.equ usrParam1,                 100    ; parameter group Advanced: generic 16-bit wide, user-defined parameter #1 for user-defined, advanced control options
	.equ usrParam2,                 102    ; parameter group Advanced: generic 16-bit wide, user-defined parameter #2 for user-defined, advanced control options
	.equ usrParam3,                 104    ; parameter group Advanced: generic 16-bit wide, user-defined parameter #3 for user-defined, advanced control options
	.equ usrParam4,                 106    ; parameter group Advanced: generic 16-bit wide, user-defined parameter #4 for user-defined, advanced control options
	
;------------------------------------------------------------------------------
;source code section.
	.section .text    ; place code in the code section
	
;------------------------------------------------------------------------------
; Global function declaration
; This function calls the z-domain controller processing the latest data point input
;------------------------------------------------------------------------------
	
	.global _i_loop_1_Update
_i_loop_1_Update:    ; provide global scope to routine
	
;------------------------------------------------------------------------------
; Check status word for Enable/Disable flag and bypass computation, if disabled
	btss [w0], #NPNZ16_STATUS_ENABLED    ; check ENABLED bit state, skip (do not execute) next instruction if set
	bra I_LOOP_1_LOOP_BYPASS    ; if ENABLED bit is cleared, jump to end of control code
	
;------------------------------------------------------------------------------
; Setup pointers to A-Term data arrays
	mov [w0 + #ptrACoefficients], w8    ; load pointer to first index of A coefficients array
	
;------------------------------------------------------------------------------
; Load pointer to first element of control history array
	mov [w0 + #ptrControlHistory], w10    ; load pointer address into wreg
	
;------------------------------------------------------------------------------
; Compute compensation filter term
	clr a, [w8]+=4, w4, [w10]+=2, w6    ; clear accumulator A and prefetch first operands
	mac w4*w6, a, [w8]+=4, w4, [w10]+=2, w6    ; multiply control output (n-1) from the delay line with coefficient A1
	mac w4*w6, a    ; multiply & accumulate last control output with coefficient of the delay line (no more prefetch)
	
;------------------------------------------------------------------------------
; Backward normalization of recent result
	mov [w0 + #normPostShiftA], w6    ; load A-coefficients post bit-shift scaler value into working register
	sftac a, w6    ; shift accumulator A by number of bits loaded in working register
	
;------------------------------------------------------------------------------
; Update error history (move error one tick along the delay line)
	mov [w10 + #2], w6    ; move entry (n-2) into buffer
	mov w6, [w10 + #4]    ; move buffered value one tick down the delay line
	mov [w10 + #0], w6    ; move entry (n-1) into buffer
	mov w6, [w10 + #2]    ; move buffered value one tick down the delay line
	
;------------------------------------------------------------------------------
; Read data from input source and calculate error input to transfer function
	mov [w0 + #ptrSourceRegister], w2    ; load pointer to input source register
	mov [w2], w1    ; move value from input source into working register
	mov [w0 + #ptrDProvControlInput], w2    ; load pointer address of target buffer of most recent raw controller input from data structure
	mov w1, [w2]    ; copy most recent controller input value to given data buffer target
	mov [w0 + #SourceOffset], w2    ; load input offset value into working register
	subr w2, w1, w1    ; remove offset from control input
	mov [w0 + #ptrControlReference], w2    ; move pointer to control reference into working register
	subr w1, [w2], w1    ; calculate error (=reference - input)
	mov [w0 + #normPreShift], w2    ; move error input scaler into working register
	sl w1, w2, w1    ; normalize error result to fractional number format
	
;------------------------------------------------------------------------------
; Setup pointers to B-Term data arrays
	mov [w0 + #ptrBCoefficients], w8    ; load pointer to first index of B coefficients array
	mov w1, [w10]    ; add most recent error input to history array
	
;------------------------------------------------------------------------------
; Compute B-Term of the compensation filter
	clr b, [w8]+=4, w4, [w10]+=2, w6    ; clear accumulator B and prefetch first operands
	mac w4*w6, b, [w8]+=4, w4, [w10]+=2, w6    ; multiply & accumulate error input (n-0) from the delay line with coefficient B0 and prefetch next operands
	mac w4*w6, b, [w8]+=4, w4, [w10]+=2, w6    ; multiply & accumulate error input (n-1) from the delay line with coefficient B1 and prefetch next operands
	mac w4*w6, b    ; multiply & accumulate last error input with coefficient of the delay line (no more prefetch)
	
;------------------------------------------------------------------------------
; Backward normalization of recent result
	mov [w0 + #normPostShiftB], w6    ; load B-coefficients post bit-shift scaler value into working register
	sftac b, w6    ; shift accumulator B by number of bits loaded in working register
	
;------------------------------------------------------------------------------
; Add accumulators finalizing LDE computation
	add a    ; add accumulator b to accumulator a
	sac.r a, w4    ; store most recent accumulator result in working register
	
;------------------------------------------------------------------------------
; Controller Anti-Windup (control output value clamping)
	; Check for upper limit violation
	mov [w0 + #MaxOutput], w6    ; load upper limit value
	cpslt w4, w6    ; compare values and skip next instruction if control output is within operating range (control output < upper limit)
	mov w6, w4    ; override controller output
	I_LOOP_1_CLAMP_MAX_EXIT:
	; Check for lower limit violation
	mov [w0 + #MinOutput], w6    ; load lower limit value
	cpsgt w4, w6    ; compare values and skip next instruction if control output is within operating range (control output > lower limit)
	mov w6, w4    ; override controller output
	I_LOOP_1_CLAMP_MIN_EXIT:
	
;------------------------------------------------------------------------------
; Write control output value to target
	mov [w0 + #ptrTargetRegister], w8    ; move pointer to target to working register
	mov w4, [w8]    ; move control output to target address
	
;------------------------------------------------------------------------------
; Update ADC trigger locations
	asr w4, #1, w6    ; half control output by shifting value one bit to the right
	; Update ADC trigger A position
	mov [w0 + #ADCTriggerAOffset], w8    ; load user-defined ADC trigger A offset value into working register
	add w6, w8, w10    ; add user-defined ADC trigger A offset to half of control output
	mov [w0 + #ptrADCTriggerARegister], w8    ; load pointer to ADC trigger A register into working register
	mov w10, [w8]    ; push new ADC trigger value to ADC trigger A register
	
;------------------------------------------------------------------------------
; Load pointer to first element of control history array
	mov [w0 + #ptrControlHistory], w10    ; load pointer address into wreg
	
;------------------------------------------------------------------------------
; Update control output history
	mov [w10 + #0], w6    ; move entry (n-1) one tick down the delay line
	mov w6, [w10 + #2]
	mov w4, [w10]    ; add most recent control output to history
	
;------------------------------------------------------------------------------
; Enable/Disable bypass branch target with dummy read of source buffer
	goto I_LOOP_1_LOOP_EXIT    ; when enabled, step over dummy read and go straight to EXIT
	I_LOOP_1_LOOP_BYPASS:    ; Enable/Disable bypass branch target to perform dummy read of source to clear the source buffer
	mov [w0 + #ptrSourceRegister], w2    ; load pointer to input source register
	mov [w2], w1    ; move value from input source into working register
	mov [w0 + #ptrDProvControlInput], w2    ; load pointer address of target buffer of most recent raw controller input from data structure
	mov w1, [w2]    ; copy most recent controller input value to given data buffer target
	I_LOOP_1_LOOP_EXIT:    ; Exit control loop branch target
	
;------------------------------------------------------------------------------
; End of routine
	return
;------------------------------------------------------------------------------
	
;------------------------------------------------------------------------------
; Global function declaration _i_loop_1_Reset
; This function clears control and error histories enforcing a reset
;------------------------------------------------------------------------------
	
	.global _i_loop_1_Reset
_i_loop_1_Reset:
	
;------------------------------------------------------------------------------
; Clear control history array
	push w0    ; save contents of working register WREG0
	mov [w0 + #ptrControlHistory], w0    ; set pointer to the base address of control history array
	clr [w0++]    ; clear next address of control history array
	clr [w0]    ; clear last address of control history array
	pop w0    ; restore contents of working register WREG0
	
;------------------------------------------------------------------------------
; Clear error history array
	push w0    ; save contents of working register WREG0
	mov [w0 + #ptrErrorHistory], w0    ; set pointer to the base address of error history array
	clr [w0++]    ; Clear next address of error history array
	clr [w0++]    ; Clear next address of error history array
	clr [w0]    ; clear last address of error history array
	pop w0    ; restore contents of working register WREG0
	
;------------------------------------------------------------------------------
; End of routine
	return
;------------------------------------------------------------------------------
	
;------------------------------------------------------------------------------
; Global function declaration _i_loop_1_Precharge
; This function loads user-defined default values into control and error histories
;------------------------------------------------------------------------------
	
	.global _i_loop_1_Precharge
_i_loop_1_Precharge:
	
;------------------------------------------------------------------------------
; Charge error history array with defined value
	push w0    ; save contents of working register WREG0
	push w1    ; save contents of working register WREG1
	mov  [w0 + #ptrErrorHistory], w0    ; set pointer to the base address of error history array
	mov w1, [w0++]    ; Load user value into next address of error history array
	mov w1, [w0++]    ; Load user value into next address of error history array
	mov w1, [w0]    ; load user value into last address of error history array
	pop w1    ; restore contents of working register WREG1
	pop w0    ; restore contents of working register WREG0
	
;------------------------------------------------------------------------------
; Charge control history array with defined value
	push w0    ; save contents of working register WREG0
	push w2    ; save contents of working register WREG2
	mov  [w0 + #ptrControlHistory], w0    ; set pointer to the base address of control history array
	mov w2, [w0++]    ; Load user value into next address of control history array
	mov w2, [w0]    ; Load user value into last address of control history array
	pop w2    ; restore contents of working register WREG2
	pop w0    ; restore contents of working register WREG0
	
;------------------------------------------------------------------------------
; End of routine
	return
;------------------------------------------------------------------------------
	
;------------------------------------------------------------------------------
; End of file
	.end
;------------------------------------------------------------------------------
	 
; **********************************************************************************
;  Download latest version of this tool here: https://areiter128.github.io/DCLD
; **********************************************************************************
	