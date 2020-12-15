/* *********************************************************************************
 * z-Domain Control Loop Designer, Version 0.9.10.501
 * *********************************************************************************
 * 2p2z controller function declarations and compensation filter coefficients
 * derived for following operating conditions:
 * *********************************************************************************
 *
 *  Controller Type:    2P2Z - Basic Current Mode Compensator
 *  Sampling Frequency: 250000 Hz
 *  Fixed Point Format: Q15
 *  Scaling Mode:       3 - Dual Bit-Shift Scaling
 *  Input Gain:         1
 *
 * *********************************************************************************
 * CGS Version:         2.0.13
 * CGS Date:            07/30/2020
 * *********************************************************************************
 * User:                Yuanzhe
 * Date/Time:           9/8/2020 3:14:20 PM
 * ********************************************************************************/

#ifndef __SPECIAL_FUNCTION_LAYER_I_LOOP_2_H__
#define __SPECIAL_FUNCTION_LAYER_I_LOOP_2_H__

#include <xc.h> // include processor files - each processor file is guarded
#include <dsp.h> // include DSP data types (e.g. fractional)
#include <stdint.h> // include standard integer number data types
#include <stdbool.h> // include standard boolean data types (true/false)

#include "./pwr_control/drivers/npnz16b.h" // include NPNZ library header file

/* *******************************************************************************
 * Data Arrays:
 * The NPNZ16b_t data structure contains pointers to coefficient, control and error
 * history arrays. The pointer target objects (variables and arrays) are defined
 * in controller source file i_loop_2.c
 *
 * Type definitions for A- and B- coefficient arrays as well as error- and control
 * history arrays are aligned in memory using the 'packed' attribute for optimized
 * addressing during DSP computations. These aligned data structures need to be
 * placed in specific memory locations to allow direct X/Y-access from the DSP.
 * This X/Y-memory placement is covered by the declarations used in controller
 * source file i_loop_2.c
 * ******************************************************************************/
typedef struct I_LOOP_2_CONTROL_LOOP_COEFFICIENTS_s
{
    volatile int32_t ACoefficients[2]; // A-Coefficients
    volatile int32_t BCoefficients[3]; // B-Coefficients
} __attribute__((packed)) I_LOOP_2_CONTROL_LOOP_COEFFICIENTS_t;

typedef struct I_LOOP_2_CONTROL_LOOP_HISTORIES_s
{
    volatile fractional ControlHistory[2];  // Control History Array
    volatile fractional ErrorHistory[3];  // Error History Array
} __attribute__((packed)) I_LOOP_2_CONTROL_LOOP_HISTORIES_t;


// User-defined NPNZ16b_s controller data object
extern volatile struct NPNZ16b_s i_loop_2; // user-controller data object

/* *******************************************************************************
 * Function call prototypes for initialization routines and control loops
 * ******************************************************************************/

// Initialization of i_loop_2 controller object
extern volatile uint16_t i_loop_2_Initialize( // i_loop_2 initialization function call
        volatile struct NPNZ16b_s* controller // Pointer to nPnZ data type object
    );

// Clears the 2P2Z controller output and error histories
extern void i_loop_2_Reset( // i_loop_2 reset function call (Assembly)
        volatile struct NPNZ16b_s* controller // Pointer to nPnZ data type object
    );

// Loads user-defined values into 2P2Z controller output and error histories
extern void i_loop_2_Precharge( // i_loop_2 history pre-charge function call (Assembly)
        volatile struct NPNZ16b_s* controller, // Pointer to nPnZ data type object
        volatile fractional ctrl_input, // user-defined, constant error history value
        volatile fractional ctrl_output // user-defined, constant control output history value
    );

// Calls the i_loop_2 control loop
extern void i_loop_2_Update( // Calls the 2P2Z controller (Assembly)
        volatile struct NPNZ16b_s* controller // Pointer to nPnZ data type object
    );


#endif // end of __SPECIAL_FUNCTION_LAYER_I_LOOP_2_H__


//**********************************************************************************
// Download latest version of this tool here: https://areiter128.github.io/DCLD
//**********************************************************************************

