/*
 * File:   app_power_control_isr.c
 * Author: M91406
 *
 * Created on May 26, 2020, 7:32 PM
 */


#include "pwr_control/app_power_control.h"

/*!Power Converter Control Loop Interrupt
 * **************************************************************************************************
 * 
 * **************************************************************************************************/

/* @@_BUCK_VLOOP_Interrupt
 * ********************************************************************************
 * Summary: Main Control Interrupt
 * 
 * Parameters:
 *  (none)
 * 
 * Returns:
 *  (none)
 * 
 * Description:
 * The control interrupt is calling the control loop. The point in time where
 * this interrupt is thrown is determined by selecting the BUCK_VOUT_TRIGGER_MODE
 * option. 
 * 
 * ********************************************************************************/

void __attribute__((__interrupt__, auto_psv, context))_BUCK_VLOOP_Interrupt(void)
{
    
    DBGPIN_1_SET;
//    PWRGOOD_SET;
    
    buck.status.bits.adc_active = true;
    #if (PLANT_MEASUREMENT == false)
    buck.v_loop.ctrl_Update(buck.v_loop.controller);
    buck.i_loop[0].ctrl_Update(buck.i_loop[0].controller);
    buck.i_loop[1].ctrl_Update(buck.i_loop[1].controller);
    #else
    v_loop_PTermUpdate(&v_loop);
    #endif

    Nop(); // Debugging break point anchors
    Nop();
    Nop();
    Nop();
    
    // Clear the ADCANx interrupt flag 
    _BUCK_VLOOP_ISR_IF = 0;  
    
    DBGPIN_1_CLEAR;
//    PWRGOOD_CLEAR;
    
}
