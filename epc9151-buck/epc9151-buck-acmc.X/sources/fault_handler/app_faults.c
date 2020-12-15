/*
 * File:   faults.c
 * Author: M91406
 *
 * Created on March 12, 2020, 11:38 AM
 */

#include <stddef.h>

#include "drivers/drv_fault_handler.h"
#include "config/epc9151_r10_hwdescr.h"
#include "pwr_control/app_power_control.h"


// Define fault objects
volatile FAULT_OBJECT_t fltobj_BuckUVLO;
volatile FAULT_OBJECT_t fltobj_BuckOVLO;
volatile FAULT_OBJECT_t fltobj_BuckRegErr;
volatile FAULT_OBJECT_t fltobj_BuckOCP;


volatile uint16_t appFaults_Initialize(void) 
{
    // Initialize UVLO fault object

    fltobj_BuckUVLO.source_obj = &buck.data.v_in;   // Set pointer to variable to monitor
    fltobj_BuckUVLO.ref_obj = NULL;      // Clear pointer to "compare against" variable 
    fltobj_BuckUVLO.bit_mask = 0xFFFF;  // Compare all bits of SOURCE (no bit filter)
    
    fltobj_BuckUVLO.status.bits.type = FLTCMP_LESS_THAN; // Select Compare-Type
    fltobj_BuckUVLO.counter = 0;        // Clear fault event counter
    fltobj_BuckUVLO.trip_level = BUCK_VIN_UVLO_TRIP;    // Set fault trip level
    fltobj_BuckUVLO.tripcnt_max = 50;    // Set counter level at which a FAULT condition will be tripped
    fltobj_BuckUVLO.reset_level = BUCK_VIN_UVLO_RELEASE;   // Set fault reset level
    fltobj_BuckUVLO.rstcnt_max = 500;     // Set counter level at which a FAULT condition will be cleared
    
    // User-function declarations
    fltobj_BuckUVLO.trip_response = &appPowerSupply_Suspend; // Set pointer to user-function which should be called when a FAULT is tripped
    fltobj_BuckUVLO.reset_response = &appPowerSupply_Resume; // Set pointer to user-function which should be called when a FAULT is cleared
    
    fltobj_BuckOVLO.status.bits.fault_active = true; // Set fault condition flag (must be cleared by fault check)
    fltobj_BuckOVLO.status.bits.fault_status = true; // Set fault flag (must be cleared by fault check)
    fltobj_BuckUVLO.status.bits.enabled = true; // Enable fault checks

    
    // Initialize OVLO fault object

    fltobj_BuckOVLO.source_obj = &buck.data.v_in;   // Set pointer to variable to monitor
    fltobj_BuckOVLO.ref_obj = NULL;      // Clear pointer to "compare against" variable 
    fltobj_BuckOVLO.bit_mask = 0xFFFF;  // Compare all bits of SOURCE (no bit filter)
    
    fltobj_BuckOVLO.status.bits.type = FLTCMP_GREATER_THAN; // Select Compare-Type
    fltobj_BuckOVLO.counter = 0;        // Clear fault event counter
    fltobj_BuckOVLO.trip_level = BUCK_VIN_OVLO_TRIP;    // Set fault trip level
    fltobj_BuckOVLO.tripcnt_max = 50;    // Set counter level at which a FAULT condition will be tripped
    fltobj_BuckOVLO.reset_level = BUCK_VIN_OVLO_RELEASE;   // Set fault reset level
    fltobj_BuckOVLO.rstcnt_max = 500;     // Set counter level at which a FAULT condition will be cleared
    
    // User-function declarations
    fltobj_BuckOVLO.trip_response = &appPowerSupply_Suspend; // Set pointer to user-function which should be called when a FAULT is tripped
    fltobj_BuckOVLO.reset_response = &appPowerSupply_Resume; // Set pointer to user-function which should be called when a FAULT is cleared
    
    fltobj_BuckOVLO.status.bits.fault_active = true; // Set fault condition flag (must be cleared by fault check)
    fltobj_BuckOVLO.status.bits.fault_status = true; // Set fault flag (must be cleared by fault check)
    fltobj_BuckOVLO.status.bits.enabled = true; // Enable fault checks
            
    // Initialize regulation error fault object

    fltobj_BuckRegErr.source_obj = &buck.data.v_out; // Set pointer to variable to monitor
    fltobj_BuckRegErr.ref_obj = &buck.set_values.v_ref; // Set pointer to "compare against" variable 
    fltobj_BuckRegErr.bit_mask = 0xFFFF; // Compare all bits of SOURCE (no bit filter)
    
    fltobj_BuckRegErr.status.bits.type = FLTCMP_GREATER_THAN; // Select Compare-Type
    fltobj_BuckRegErr.counter = 0;        // Clear fault event counter
    fltobj_BuckRegErr.trip_level = BUCK_VOUT_DEV_TRIP;    // Set fault trip level
    fltobj_BuckRegErr.tripcnt_max = 250;    // Set counter level at which a FAULT condition will be tripped
    fltobj_BuckRegErr.reset_level = BUCK_VOUT_DEV_RELEASE;   // Set fault reset level
    fltobj_BuckRegErr.rstcnt_max = 1000;     // Set counter level at which a FAULT condition will be cleared
    
    // User-function declarations
    fltobj_BuckRegErr.trip_response = &appPowerSupply_Suspend; // Set pointer to user-function which should be called when a FAULT is tripped
    fltobj_BuckRegErr.reset_response = &appPowerSupply_Resume; // Set pointer to user-function which should be called when a FAULT is cleared
    
    fltobj_BuckRegErr.status.bits.fault_active = false; // Set fault condition flag (must be cleared by fault check)
    fltobj_BuckRegErr.status.bits.fault_status = false; // Set fault flag (must be cleared by fault check)
    fltobj_BuckRegErr.status.bits.enabled = false; // Disable fault checks at startup
    
    // Initialize OCP fault object

    fltobj_BuckOCP.source_obj = &buck.data.i_out;   // Set pointer to variable to monitor
    fltobj_BuckOCP.ref_obj = NULL;      // Clear pointer to "compare against" variable 
    fltobj_BuckOCP.bit_mask = 0xFFFF;  // Compare all bits of SOURCE (no bit filter)
    
    fltobj_BuckOCP.status.bits.type = FLTCMP_GREATER_THAN; // Select Compare-Type
    fltobj_BuckOCP.counter = 0;        // Clear fault event counter
    fltobj_BuckOCP.trip_level = BUCK_ISNS_OCL;    // Set fault trip level
    fltobj_BuckOCP.tripcnt_max = 50;    // Set counter level at which a FAULT condition will be tripped
    fltobj_BuckOCP.reset_level = BUCK_ISNS_OCL_RELEASE;   // Set fault reset level
    fltobj_BuckOCP.rstcnt_max = 2000;     // Set counter level at which a FAULT condition will be cleared
    
    // User-function declarations
    fltobj_BuckOCP.trip_response = &appPowerSupply_Suspend; // Set pointer to user-function which should be called when a FAULT is tripped
    fltobj_BuckOCP.reset_response = &appPowerSupply_Resume; // Set pointer to user-function which should be called when a FAULT is cleared
    
    fltobj_BuckOCP.status.bits.fault_active = true; // Set fault condition flag (must be cleared by fault check)
    fltobj_BuckOCP.status.bits.fault_status = true; // Set fault flag (must be cleared by fault check)
    fltobj_BuckOCP.status.bits.enabled = true; // Enable fault checks    

    return(1);
}

volatile uint16_t appFaults_Dispose(void) 
{
//    fltobj_BuckUVLO = NULL; // Delete Under Voltage Lock Out object
//    fltobj_BuckOVLO = NULL; // Delete Under Voltage Lock Out object
    
    return(1);
}


volatile uint16_t appFaults_Execute(void) 
{
    volatile uint16_t fres=1;
    
    // Call fault handler
    fres &= fault_check(&fltobj_BuckUVLO);
    fres &= fault_check(&fltobj_BuckOVLO);
    fres &= fault_check(&fltobj_BuckRegErr);
    fres &= fault_check(&fltobj_BuckOCP);
    
    return (fres);
}
