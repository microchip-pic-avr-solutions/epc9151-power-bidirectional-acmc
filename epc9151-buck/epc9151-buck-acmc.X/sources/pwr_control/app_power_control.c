/*
 * File:   pwr_control.c
 * Author: M91406
 *
 * Created on March 12, 2020, 11:55 AM
 */


#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdint.h> // include standard integer data types
#include <stdbool.h> // include standard boolean data types
#include <stddef.h> // include standard definition data types

#include "pwr_control/app_power_control.h"
#include "config/epc9151_r10_hwdescr.h"
#include "fault_handler/app_faults.h"

/*!BUCK_POWER_CONTROLLER_t data structure
 * *************************************************************************************************
 * Summary:
 * Global data object for a BUCK CONVERTER 
 * 
 * Description:
 * The 'buck' data object holds all status, control and monitoring values of the BUCK power 
 * controller. The BUCK_POWER_CONTROLLER_t data structure is defined in dev_buck_converter.h.
 * Please refer to the comments on top of this file for further information.
 *  
 * *************************************************************************************************/
volatile BUCK_POWER_CONTROLLER_t buck;

/* PRIVATE FUNCTION PROTOTYPES */
volatile uint16_t appPowerSupply_ConverterObjectInitialize(void);
volatile uint16_t appPowerSupply_ControllerInitialize(void);
volatile uint16_t appPowerSupply_PeripheralsInitialize(void);

void appPowerSupply_CurrentBalancing(void); 
void appPowerSupply_CurrentSenseCalibration(void);


/* CURRENT SENSE CALIBRATION */
#define  CS_CALIB_STEPS         8
typedef struct {
    volatile uint16_t cs_calib_cnt;
    volatile uint16_t cs_calib_offset;
}CS_CALIBRATION_t;
    
volatile CS_CALIBRATION_t calib_cs1;
volatile CS_CALIBRATION_t calib_cs2;

/* *************************************************************************************************
 * PUBLIC FUNCTIONS
 * ************************************************************************************************/

volatile uint16_t appPowerSupply_Initialize(void)
{ 
    volatile uint16_t retval=1;

    // Run initialization sequence
    retval &= appPowerSupply_ConverterObjectInitialize();
    retval &= appPowerSupply_ControllerInitialize();
    retval &= appPowerSupply_PeripheralsInitialize();

    // Sequence Peripheral Startup
    retval &= buckPWM_Start(&buck);   // Start PWM (All Outputs Disabled)
    if (retval) buck.status.bits.pwm_active = 1;
    
    retval &= buckADC_Start(); // Start ADC
    
    // Initialize Control Interrupt
#if (BUCK_VOUT_TRIGGER_MODE == BUCK_VOUT_TRIG_ADC)   
    ADEIELbits.EIEN0 = 1; // Early interrupt is enabled for the channel 0
 
#endif
    _BUCK_VLOOP_ISR_IP = 5;
    _BUCK_VLOOP_ISR_IF = 0;
    _BUCK_VLOOP_ISR_IE = 1;
    
    // Enable Buck Converter
    buck.status.bits.enabled = true;
    
    return(retval); 
}

/* @@<function_name>
 * ********************************************************************************
 * Summary:
 * 
 * Parameters:
 * 
 * Returns:
 * 
 * Description:
 * 
 * ********************************************************************************/

volatile uint16_t appPowerSupply_Execute(void)
{ 
    volatile uint16_t retval=1;
    volatile uint16_t _i=0;
    volatile uint16_t i_dummy=0;

    // Capture most recent samples
    buck.data.v_in = BUCK_VIN_ADCBUF;
    buck.data.i_sns[0] = BUCK_ISNS1_ADCBUF;
    buck.data.i_sns[1] = BUCK_ISNS2_ADCBUF;
    
    // Accumulate phase currents
    for (_i=0; _i<buck.set_values.phases; _i++) 
    { i_dummy += buck.data.i_sns[_i]; }
    buck.data.i_out = i_dummy; // Set output current value

    // Check conditional parameters and fault flags
    buck.status.bits.power_source_detected = (bool)
        ((BUCK_VIN_UVLO_TRIP < buck.data.v_in) && (buck.data.v_in<BUCK_VIN_OVLO_TRIP));
    
    // Combine individual fault bits to a common fault indicator
    buck.status.bits.fault_active = (bool) (
        fltobj_BuckUVLO.status.bits.fault_status | 
        fltobj_BuckOVLO.status.bits.fault_status |
        fltobj_BuckRegErr.status.bits.fault_status |
        fltobj_BuckOCP.status.bits.fault_status
        );
    
    // Execute buck converter state machine
    retval &= drv_BuckConverter_Execute(&buck);
    
    // Execute slower, advanced control options
    appPowerSupply_CurrentSenseCalibration();
//    appPowerSupply_CurrentBalancing();

    // Buck regulation error is only active while controller is running
    // and while being tied to a valid reference
    if(buck.mode >= BUCK_STATE_V_RAMP_UP) 
    {
        fltobj_BuckRegErr.ref_obj = buck.v_loop.controller->Ports.ptrControlReference;
        #if (PLANT_MEASUREMENT == false)
        fltobj_BuckRegErr.status.bits.enabled = buck.v_loop.controller->status.bits.enabled;
        #endif
    }
    else 
    {
        fltobj_BuckRegErr.status.bits.enabled = false;
    }
    
    return(retval); 
}

/* @@<function_name>
 * ********************************************************************************
 * Summary:
 * 
 * Parameters:
 * 
 * Returns:
 * 
 * Description:
 * 
 * ********************************************************************************/

volatile uint16_t appPowerSupply_Dispose(void)
{ 
    volatile uint16_t retval=1;
    volatile uint16_t _i=0;

    buck.status.value = 0;
    
    for (_i=0; _i<buck.set_values.phases; _i++) // Reset phase current values
    { buck.data.i_sns[0] = 0; }
    buck.data.i_out = 0; // Reset output current value
    buck.data.v_out = 0; // Reset output voltage value
    buck.data.v_in = 0;  // Reset input voltage value
    buck.data.temp = 0;  // Reset output temperature value
    buck.mode = BUCK_STATE_INITIALIZE; // Set state machine
    
    return(retval); 
}

/* @@<function_name>
 * ********************************************************************************
 * Summary:
 * 
 * Parameters:
 * 
 * Returns:
 * 
 * Description:
 * 
 * ********************************************************************************/

volatile uint16_t appPowerSupply_Suspend(void)
{ 
    volatile uint16_t retval=1;

    retval &= buckPWM_Suspend(&buck); // Shut down PWM immediately
    buck.status.bits.fault_active = true; // Set FAULT flag
    buck.mode = BUCK_STATE_RESET; // Reset State Machine (causes loop reset)

    return(retval); 
}

/* @@appPowerSupply_Resume
 * ********************************************************************************
 * Summary:
 * 
 * Parameters:
 * 
 * Returns:
 * 
 * Description:
 * 
 * ********************************************************************************/

volatile uint16_t appPowerSupply_Resume(void)
{ 
    volatile uint16_t retval=0;

    buck.mode = BUCK_STATE_RESET;       // Ensure State Machine is RESET
    buck.status.bits.enabled = true;    // Ensure Buck Converter is enabled
    
    return(retval); 
}

/* *************************************************************************************************
 * PRIVATE FUNCTIONS
 * ************************************************************************************************/

volatile uint16_t appPowerSupply_ConverterObjectInitialize(void)
{
    volatile uint16_t retval = 1;
    volatile uint16_t _i = 0;
    volatile bool adc_trigger_mode=false;
    
    // Initialize Buck Converter Object Status
    buck.status.bits.adc_active = false; // Clear ADC STARTED flag
    buck.status.bits.pwm_active = false; // clear PWM STARTED flag
    buck.status.bits.power_source_detected = false; // Clear POWER SOURCE DETECTED flag
    buck.status.bits.cs_calib_complete = false; // Clear Current Sense Calibration flag
    buck.status.bits.fault_active = true; // Set global FAULT flag
    buck.status.bits.cs_calib = BUCK_ISNS_NEED_CALIBRATION; // Topology current sensors need to be calibrated
    buck.status.bits.autorun = true;  // Allow the buck converter to run when cleared of faults
    buck.status.bits.enabled = false; // Disable buck converter
 
    // Set Initial State Machine State
    buck.mode = BUCK_STATE_INITIALIZE; // Reset Buck State Machine
    
    // Set Reference values
    buck.set_values.control_mode = BUCK_CONTROL_MODE_ACMC; // Set Control Mode
    buck.set_values.i_ref = BUCK_ISNS_REF; // Set current loop reference
    buck.set_values.v_ref = BUCK_VOUT_REF; // Set voltage loop reference
    buck.set_values.phases = BUCK_NO_OF_PHASES; // Set number of converter phases
    
    // Clear Runtime Data
    for (_i=0; _i<buck.set_values.phases; _i++) // Reset phase current values
    { buck.data.i_sns[0] = 0; }
    buck.data.i_out = 0; // Reset output current value
    buck.data.v_out = 0; // Reset output voltage value
    buck.data.v_in = 0;  // Reset input voltage value
    buck.data.temp = 0;  // Reset output temperature value
    
    // Initialize Switch Node of PWM #1
    buck.sw_node[0].pwm_instance = BUCK_PWM1_CHANNEL;
    buck.sw_node[0].gpio_instance = BUCK_PWM1_GPIO_INSTANCE;
    buck.sw_node[0].gpio_high = BUCK_PWM1_GPIO_PORT_PINH;
    buck.sw_node[0].gpio_low = BUCK_PWM1_GPIO_PORT_PINL;
    buck.sw_node[0].master_period = true;
    buck.sw_node[0].period = BUCK_PWM_PERIOD;
    buck.sw_node[0].phase = BUCK_PWM_PHASE_SHIFT; 
    buck.sw_node[0].duty_ratio_min = BUCK_PWM_DC_MIN;
    buck.sw_node[0].duty_ratio_init = BUCK_PWM_DC_MIN;
    buck.sw_node[0].duty_ratio_max = BUCK_PWM_DC_MAX;
    buck.sw_node[0].dead_time_rising = BUCK_PWM_DEAD_TIME_LE;
    buck.sw_node[0].dead_time_falling = BUCK_PWM_DEAD_TIME_FE;
    buck.sw_node[0].leb_period = BUCK_LEB_PERIOD;
    buck.sw_node[0].trigger_offset = BUCK_PWM1_ADTR1OFS;
    buck.sw_node[0].trigger_scaler = BUCK_PWM1_ADTR1PS;

    // Initialize Switch Node of PWM #1
    buck.sw_node[1].pwm_instance = BUCK_PWM2_CHANNEL;
    buck.sw_node[1].gpio_instance = BUCK_PWM2_GPIO_INSTANCE;
    buck.sw_node[1].gpio_high = BUCK_PWM2_GPIO_PORT_PINH;
    buck.sw_node[1].gpio_low = BUCK_PWM2_GPIO_PORT_PINL;
    buck.sw_node[1].master_period = true;
    buck.sw_node[1].period = BUCK_PWM_PERIOD;
    buck.sw_node[1].phase = BUCK_PWM_PHASE_SHIFT; 
    buck.sw_node[1].duty_ratio_min = BUCK_PWM_DC_MIN;
    buck.sw_node[1].duty_ratio_init = BUCK_PWM_DC_MIN;
    buck.sw_node[1].duty_ratio_max = BUCK_PWM_DC_MAX;
    buck.sw_node[1].dead_time_rising = BUCK_PWM_DEAD_TIME_LE;
    buck.sw_node[1].dead_time_falling = BUCK_PWM_DEAD_TIME_FE;
    buck.sw_node[1].leb_period = BUCK_LEB_PERIOD;
    buck.sw_node[1].trigger_offset = BUCK_PWM2_ADTR1OFS;
    buck.sw_node[1].trigger_scaler = BUCK_PWM2_ADTR1PS;
    
    // Initialize additional GPIOs 
    
    // ~~~ EXTERNAL ENABLE INPUT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    buck.gpio.Enable.enabled = false; // this converter doesn't support external enable control
    // ~~~ EXTERNAL ENABLE INPUT END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // ~~~ POWER GOOD OUTPUT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    buck.gpio.PowerGood.enabled = true; // This converter supports an additional POWER GOOD output
    buck.gpio.PowerGood.port = PWRGOOD_PORT; // Number of the GPIO port (0=A, 1=B, 2=C, etc.)
    buck.gpio.PowerGood.pin = PWRGOOD_PIN; // Number of the GPIO port pin
    buck.gpio.PowerGood.polarity = 0;   // This pin is ACTIVE HIGH (only required if io_type = OUTPUT)
    buck.gpio.PowerGood.io_type = 0;    // This pin is configured as OUTPUT
    
    // ~~~ POWER GOOD OUTPUT END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // Initialize Feedback Channels
    
    // ~~~ OUTPUT VOLTAGE FEEDBACK ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    buck.feedback.ad_vout.enabled = true;   // Use this channel
    adc_trigger_mode = (bool)(BUCK_VOUT_TRIGGER_MODE == BUCK_VOUT_TRIG_ADC);

    buck.feedback.ad_vout.adc_input = BUCK_VOUT_ADCIN;
    buck.feedback.ad_vout.adc_core = BUCK_VOUT_ADCCORE;
    buck.feedback.ad_vout.adc_buffer = &BUCK_VOUT_ADCBUF;
    buck.feedback.ad_vout.trigger_source = BUCK_VOUT_TRGSRC;

    buck.feedback.ad_vout.differential_input = false;
    buck.feedback.ad_vout.interrupt_enable = adc_trigger_mode;
    buck.feedback.ad_vout.early_interrupt_enable = adc_trigger_mode;
    buck.feedback.ad_vout.level_trigger = false;
    buck.feedback.ad_vout.signed_result = false;
    
    buck.feedback.ad_vout.scaling.factor = BUCK_VOUT_NORM_FACTOR;
    buck.feedback.ad_vout.scaling.scaler = BUCK_VOUT_NORM_SCALER;
    buck.feedback.ad_vout.scaling.offset = BUCK_VOUT_OFFSET;
    
    BUCK_VOUT_ANSEL = (int)buck.feedback.ad_vout.enabled;
    
    // ~~~ OUTPUT VOLTAGE FEEDBACK END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // ~~~ INPUT VOLTAGE FEEDBACK ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    buck.feedback.ad_vin.enabled = true;   // Use this channel

    buck.feedback.ad_vin.adc_input = BUCK_VIN_ADCIN;
    buck.feedback.ad_vin.adc_core = BUCK_VIN_ADCCORE;
    buck.feedback.ad_vin.adc_buffer = &BUCK_VIN_ADCBUF;
    buck.feedback.ad_vin.trigger_source = BUCK_VIN_TRGSRC;

    buck.feedback.ad_vin.differential_input = false;
    buck.feedback.ad_vin.interrupt_enable = false;
    buck.feedback.ad_vin.early_interrupt_enable = false;
    buck.feedback.ad_vin.level_trigger = false;
    buck.feedback.ad_vin.signed_result = false;

    buck.feedback.ad_vin.scaling.factor = BUCK_VIN_NORM_FACTOR;
    buck.feedback.ad_vin.scaling.scaler = BUCK_VIN_NORM_SCALER;
    buck.feedback.ad_vin.scaling.offset = BUCK_VIN_OFFSET;
    
    BUCK_VIN_ANSEL = (int)buck.feedback.ad_vin.enabled;
    
    // ~~~ INPUT VOLTAGE FEEDBACK END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // ~~~ OUTPUT CURRENT FEEDBACK ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    buck.feedback.ad_isns[0].enabled = true;   // Use this channel

    buck.feedback.ad_isns[0].adc_input = BUCK_ISNS1_ADCIN;
    buck.feedback.ad_isns[0].adc_core = BUCK_ISNS1_ADCCORE;
    buck.feedback.ad_isns[0].adc_buffer = &BUCK_ISNS1_ADCBUF;
    buck.feedback.ad_isns[0].trigger_source = BUCK_ISNS1_TRGSRC;

    buck.feedback.ad_isns[0].differential_input = false;
    buck.feedback.ad_isns[0].interrupt_enable = false;
    buck.feedback.ad_isns[0].early_interrupt_enable = false;
    buck.feedback.ad_isns[0].level_trigger = false;
    buck.feedback.ad_isns[0].signed_result = false;

    buck.feedback.ad_isns[0].scaling.factor = BUCK_ISNS_NORM_FACTOR;
    buck.feedback.ad_isns[0].scaling.scaler = BUCK_ISNS_NORM_SCALER;
    buck.feedback.ad_isns[0].scaling.offset = BUCK_ISNS1_OFFFSET;
    
    
    BUCK_ISNS1_ANSEL = (int)buck.feedback.ad_isns[0].enabled;

    
    buck.feedback.ad_isns[1].enabled = true;   // Use this channel

    buck.feedback.ad_isns[1].adc_input = BUCK_ISNS2_ADCIN;
    buck.feedback.ad_isns[1].adc_core = BUCK_ISNS2_ADCCORE;
    buck.feedback.ad_isns[1].adc_buffer = &BUCK_ISNS2_ADCBUF;
    buck.feedback.ad_isns[1].trigger_source = BUCK_ISNS2_TRGSRC;

    buck.feedback.ad_isns[1].differential_input = false;
    buck.feedback.ad_isns[1].interrupt_enable = false;
    buck.feedback.ad_isns[1].early_interrupt_enable = false;
    buck.feedback.ad_isns[1].level_trigger = false;
    buck.feedback.ad_isns[1].signed_result = false;

    BUCK_ISNS2_ANSEL = (int)buck.feedback.ad_isns[1].enabled;

    
    // ~~~ OUTPUT CURRENT FEEDBACK END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Initialize Startup Settings
    
    buck.startup.power_on_delay.counter = 0;
    buck.startup.power_on_delay.period = BUCK_POD;
    buck.startup.power_on_delay.ref_inc_step = 0;
    buck.startup.power_on_delay.reference = 0;
    
    buck.startup.v_ramp.counter = 0;
    buck.startup.v_ramp.period = BUCK_VRAMP_PER;
    buck.startup.v_ramp.ref_inc_step = BUCK_VREF_STEP;
    if (buck.startup.v_ramp.ref_inc_step == 0)
        buck.startup.v_ramp.ref_inc_step = 1;
    buck.startup.v_ramp.reference = 0;
    
    if (buck.set_values.control_mode == BUCK_CONTROL_MODE_ACMC) 
    {
        buck.startup.i_ramp.counter = 0;
        buck.startup.i_ramp.period = BUCK_IRAMP_PER;
        buck.startup.i_ramp.ref_inc_step = BUCK_IREF_STEP;
        if (buck.startup.i_ramp.ref_inc_step == 0)
            buck.startup.i_ramp.ref_inc_step = 1;        
        buck.startup.i_ramp.reference = BUCK_ISNS_REF;
    }
    
    buck.startup.power_good_delay.counter = 0;
    buck.startup.power_good_delay.period = BUCK_PGD;
    buck.startup.power_good_delay.ref_inc_step = 0;
    buck.startup.power_good_delay.reference = BUCK_VOUT_REF;
    
    
    return(retval);
}

/* @@<function_name>
 * ********************************************************************************
 * Summary:
 * 
 * Parameters:
 * 
 * Returns:
 * 
 * Description:
 * 
 * ********************************************************************************/

volatile uint16_t appPowerSupply_PeripheralsInitialize(void)
{
    volatile uint16_t retval=1;
    
 
    retval &= buckPWM_ModuleInitialize(&buck); // Initialize PWM Module
    retval &= buckPWM_ChannelInitialize(&buck);  // Initialize PWM Channel of Buck Converter
    
    retval &= buckADC_ModuleInitialize();     // Initialize ADC Module
    
    retval &= buckADC_Channel_Initialize(&buck.feedback.ad_vout); // Initialize Output Voltage Channel
    retval &= buckADC_Channel_Initialize(&buck.feedback.ad_vin);  // Initialize Input Voltage Channel
    retval &= buckADC_Channel_Initialize(&buck.feedback.ad_isns[0]); // Initialize Phase Current #1 Channel
    retval &= buckADC_Channel_Initialize(&buck.feedback.ad_isns[1]); // Initialize Phase Current #2 Channel
    retval &= buckADC_Channel_Initialize(&buck.feedback.ad_temp); // Initialize Temperature Channel
    
    retval &= buckGPIO_Initialize(&buck); // Initialize additional control IOs
    
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Custom configurations
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // ToDo: Incorporate hard coded register settings into generic data structure scheme, if possible
    ADCON4Hbits.C1CHS = 1; // Set ADC  input channel to read from ANA1
    
    // Synchronize PWM channels #1 and #2
    // => taken car of by the state machine automatically, when multiple phases are defined
    // If PWM channels need to be synchronized to other converter PWMs, this needs to be done 
    // manually here
    
    return(retval);
}

/* @@<function_name>
 * ********************************************************************************
 * Summary:
 * 
 * Parameters:
 * 
 * Returns:
 * 
 * Description:
 * 
 * ********************************************************************************/

volatile uint16_t appPowerSupply_ControllerInitialize(void)
{
    volatile uint16_t retval = 1;
    
    // ~~~ VOLTAGE LOOP CONFIGURATION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // Initialize Default Loop Configuration
    buck.v_loop.feedback_offset = BUCK_VOUT_OFFSET;
    buck.v_loop.reference = BUCK_VOUT_REF;
    buck.v_loop.minimum = (int16_t)(-BUCK_ISNS_REF_MAX);
    buck.v_loop.maximum = BUCK_ISNS_REF_MAX;
    buck.v_loop.trigger_offset = BUCK_VOUT_ADC_TRGDLY;
    
     // Set Controller Object of Voltage Loop
    buck.v_loop.controller = &v_loop;
    buck.v_loop.ctrl_Initialization = &v_loop_Initialize;
    buck.v_loop.ctrl_Update = &v_loop_Update;
    buck.v_loop.ctrl_Reset = &v_loop_Reset;
    buck.v_loop.ctrl_Precharge = &v_loop_Precharge;
    
    // Configure Voltage Loop Controller Object
    buck.v_loop.ctrl_Initialization(&v_loop);   // Call Initialization Routine setting histories and scaling
    
    // Configure controller input ports
    buck.v_loop.controller->Ports.Source.ptrAddress = &BUCK_VOUT_ADCBUF; // Output Voltage is Common Source
    buck.v_loop.controller->Ports.Source.Offset = buck.v_loop.feedback_offset; // Output Voltage feedback signal offset 
    buck.v_loop.controller->Ports.Source.NormScaler = BUCK_VOUT_NORM_SCALER; // Output voltage normalization factor bit-shift scaler 
    buck.v_loop.controller->Ports.Source.NormFactor = BUCK_VOUT_NORM_FACTOR; // Output voltage normalization factor fractional
    
    buck.v_loop.controller->Ports.AltSource.ptrAddress = NULL; // Alternative Source not used
    buck.v_loop.controller->Ports.AltSource.Offset = 0; // not used
    buck.v_loop.controller->Ports.AltSource.NormScaler = BUCK_VIN_NORM_SCALER; // Input voltage normalization factor bit-shift scaler 
    buck.v_loop.controller->Ports.AltSource.NormFactor = BUCK_VIN_NORM_FACTOR; // Input voltage normalization factor fractional

    // Configure controller output ports
    buck.v_loop.controller->Ports.Target.ptrAddress = &buck.i_loop[0].reference; // Set pointer to I reference
    buck.v_loop.controller->Ports.Target.Offset = 0; // Static primary output value offset
    buck.v_loop.controller->Ports.Target.NormScaler = 0; // Primary control output normalization factor bit-shift scaler 
    buck.v_loop.controller->Ports.Target.NormFactor = 0x7FFF; // Primary control output normalization factor fractional 

    buck.v_loop.controller->Ports.AltTarget.ptrAddress = &buck.i_loop[1].reference;
    buck.v_loop.controller->Ports.AltTarget.Offset = 0; // Static secondary output value offset
    buck.v_loop.controller->Ports.AltTarget.NormScaler = 0; // Secondary control output normalization factor bit-shift scaler
    buck.v_loop.controller->Ports.AltTarget.NormFactor = 0x7FFF; // Secondary control output normalization factor fractional 
    
    // Configure controller control ports
    buck.v_loop.controller->Ports.ptrControlReference = &buck.set_values.v_ref; // Set pointer to Reference
    
    // Data Input/Output Limit Configuration
    buck.v_loop.controller->Limits.MinOutput = buck.v_loop.minimum;
    buck.v_loop.controller->Limits.MaxOutput = buck.v_loop.maximum;
    buck.v_loop.controller->Limits.AltMinOutput = 0; // not used
    buck.v_loop.controller->Limits.AltMaxOutput = 0; // not used

    // ADC Trigger Control Configuration
    buck.v_loop.controller->ADCTriggerControl.ptrADCTriggerARegister = NULL;
    buck.v_loop.controller->ADCTriggerControl.ADCTriggerAOffset = 0;
    buck.v_loop.controller->ADCTriggerControl.ptrADCTriggerBRegister = NULL;   // Using PGxTRIGB = ADC Trigger 2
    buck.v_loop.controller->ADCTriggerControl.ADCTriggerBOffset = 0;
    
    // Data Provider Configuration
    buck.v_loop.controller->DataProviders.ptrDProvControlInput = &buck.data.v_out;
    buck.v_loop.controller->DataProviders.ptrDProvControlError = NULL;
    buck.v_loop.controller->DataProviders.ptrDProvControlOutput = NULL;
    
    // Cascaded Function Configuration
    buck.v_loop.controller->CascadeTrigger.ptrCascadedFunction = NULL;
    buck.v_loop.controller->CascadeTrigger.CascadedFunctionParam = 0;
    
    // Initialize Advanced Control Settings (not used in this code example)

    // Custom Advanced Control Settings
    buck.v_loop.controller->Advanced.usrParam1 = 0; // No additional advanced control options used
    buck.v_loop.controller->Advanced.usrParam2 = 0; // No additional advanced control options used
    buck.v_loop.controller->Advanced.usrParam3 = 0; // No additional advanced control options used
    buck.v_loop.controller->Advanced.usrParam4 = 0; // No additional advanced control options used

    // Reset Controller Status
    buck.v_loop.controller->status.bits.enabled = false; // Keep controller disabled
    buck.v_loop.controller->status.bits.swap_source = false; // use SOURCE as major control input
    buck.v_loop.controller->status.bits.swap_target = false; // use TARGET as major control output
    buck.v_loop.controller->status.bits.invert_input = false; // Do not invert input value
    buck.v_loop.controller->status.bits.lower_saturation_event = false; // Reset Anti-Windup Minimum Status bit
    buck.v_loop.controller->status.bits.upper_saturation_event = false; // Reset Anti-Windup Minimum Status bits
    buck.v_loop.controller->status.bits.agc_enabled = false;   // Enable Adaptive Gain Modulation by default

    // ~~~ VOLTAGE LOOP CONFIGURATION END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // ~~~ CURRENT LOOP 1 CONFIGURATION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // Initialize Default Loop Configuration
    buck.i_loop[0].feedback_offset = BUCK_ISNS1_OFFFSET;
    buck.i_loop[0].reference = BUCK_ISNS_REF;
    buck.i_loop[0].minimum = BUCK_PWM_DC_MIN;
    buck.i_loop[0].maximum = BUCK_PWM_DC_MAX;
    buck.i_loop[0].trigger_offset = BUCK_ISNS1_ADC_TRGDLY;
    
     // Set Controller Object of Voltage Loop
    buck.i_loop[0].controller = &i_loop_1;
    buck.i_loop[0].ctrl_Initialization = &i_loop_1_Initialize;
    buck.i_loop[0].ctrl_Update = &i_loop_1_Update;
    buck.i_loop[0].ctrl_Reset = &i_loop_1_Reset;
    buck.i_loop[0].ctrl_Precharge = &i_loop_1_Precharge;
    
    // Configure Voltage Loop Controller Object
    buck.i_loop[0].ctrl_Initialization(&i_loop_1);   // Call Initialization Routine setting histories and scaling
    
    // Configure controller input ports
    buck.i_loop[0].controller->Ports.Source.ptrAddress = &BUCK_ISNS1_ADCBUF; // Output Voltage is Common Source
    buck.i_loop[0].controller->Ports.Source.Offset = buck.i_loop[0].feedback_offset; // Phase 1Output Current feedback signal offset 
    buck.i_loop[0].controller->Ports.Source.NormScaler = BUCK_ISNS_NORM_SCALER; // Output voltage normalization factor bit-shift scaler 
    buck.i_loop[0].controller->Ports.Source.NormFactor = BUCK_ISNS_NORM_FACTOR; // Output voltage normalization factor fractional
    
    buck.i_loop[0].controller->Ports.AltSource.ptrAddress = NULL; 
    buck.i_loop[0].controller->Ports.AltSource.Offset = 0; // not used
    buck.i_loop[0].controller->Ports.AltSource.NormScaler = 0; // not used
    buck.i_loop[0].controller->Ports.AltSource.NormFactor = 0; // not used

    // Configure controller output ports
    buck.i_loop[0].controller->Ports.Target.ptrAddress = &BUCK_PWM1_PDC; // PWM Duty Cycle is Control Target
    buck.i_loop[0].controller->Ports.Target.Offset = 0; // Static primary output value offset
    buck.i_loop[0].controller->Ports.Target.NormScaler = 0; // Primary control output normalization factor bit-shift scaler 
    buck.i_loop[0].controller->Ports.Target.NormFactor = 0x7FFF; // Primary control output normalization factor fractional 

    buck.i_loop[0].controller->Ports.AltTarget.ptrAddress = NULL;
    buck.i_loop[0].controller->Ports.AltTarget.Offset = 0; // Static secondary output value offset
    buck.i_loop[0].controller->Ports.AltTarget.NormScaler = 0; // Secondary control output normalization factor bit-shift scaler
    buck.i_loop[0].controller->Ports.AltTarget.NormFactor = 0x7FFF; // Secondary control output normalization factor fractional 
    
    // Configure controller control ports
    buck.i_loop[0].controller->Ports.ptrControlReference = &buck.i_loop[0].reference; // Set pointer to Reference
    
    // Data Input/Output Limit Configuration
    buck.i_loop[0].controller->Limits.MinOutput = buck.i_loop[0].minimum;
    buck.i_loop[0].controller->Limits.MaxOutput = buck.i_loop[0].maximum;
    buck.i_loop[0].controller->Limits.AltMinOutput = 0; // not used
    buck.i_loop[0].controller->Limits.AltMaxOutput = 0; // not used

    // ADC Trigger Control Configuration
    buck.i_loop[0].controller->ADCTriggerControl.ptrADCTriggerARegister = &BUCK_ISNS1_ADCTRIG;    // Using PGxTRIGA = ADC Trigger 1
    buck.i_loop[0].controller->ADCTriggerControl.ADCTriggerAOffset = buck.i_loop[0].trigger_offset; // Triggering at 50% off-time
    
    // Data Provider Configuration
    buck.i_loop[0].controller->DataProviders.ptrDProvControlInput = &buck.data.i_sns[0];
    buck.i_loop[0].controller->DataProviders.ptrDProvControlError = NULL;
    buck.i_loop[0].controller->DataProviders.ptrDProvControlOutput = NULL;
    
    // Cascaded Function Configuration
    buck.i_loop[0].controller->CascadeTrigger.ptrCascadedFunction = NULL;
    buck.i_loop[0].controller->CascadeTrigger.CascadedFunctionParam = 0;

    // Custom Advanced Control Settings
    buck.i_loop[0].controller->Advanced.usrParam1 = 0; // No additional advanced control options used
    buck.i_loop[0].controller->Advanced.usrParam2 = 0; // No additional advanced control options used
    buck.i_loop[0].controller->Advanced.usrParam3 = 0; // No additional advanced control options used
    buck.i_loop[0].controller->Advanced.usrParam4 = 0; // No additional advanced control options used

    // Reset Controller Status
    buck.i_loop[0].controller->status.bits.enabled = false; // Keep controller disabled
    buck.i_loop[0].controller->status.bits.swap_source = false; // use SOURCE as major control input
    buck.i_loop[0].controller->status.bits.swap_target = false; // use TARGET as major control output
    buck.i_loop[0].controller->status.bits.invert_input = false; // Do not invert input value
    buck.i_loop[0].controller->status.bits.lower_saturation_event = false; // Reset Anti-Windup Minimum Status bit
    buck.i_loop[0].controller->status.bits.upper_saturation_event = false; // Reset Anti-Windup Minimum Status bits
    buck.i_loop[0].controller->status.bits.agc_enabled = false;   // Enable Adaptive Gain Modulation by default

    // ~~~ CURRENT LOOP 1 CONFIGURATION END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // ~~~ CURRENT LOOP 2 CONFIGURATION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    // Initialize Default Loop Configuration
    buck.i_loop[1].feedback_offset = BUCK_ISNS2_OFFFSET;
    buck.i_loop[1].reference = BUCK_ISNS_REF;
    buck.i_loop[1].minimum = BUCK_PWM_DC_MIN;
    buck.i_loop[1].maximum = BUCK_PWM_DC_MAX;
    buck.i_loop[1].trigger_offset = BUCK_ISNS2_ADC_TRGDLY;
    
     // Set Controller Object of Voltage Loop
    buck.i_loop[1].controller = &i_loop_2;
    buck.i_loop[1].ctrl_Initialization = &i_loop_2_Initialize;
    buck.i_loop[1].ctrl_Update = &i_loop_2_Update;
    buck.i_loop[1].ctrl_Reset = &i_loop_2_Reset;
    buck.i_loop[1].ctrl_Precharge = &i_loop_2_Precharge;
    
    // Configure Voltage Loop Controller Object
    buck.i_loop[1].ctrl_Initialization(&i_loop_2);   // Call Initialization Routine setting histories and scaling
    
    // Configure controller input ports
    buck.i_loop[1].controller->Ports.Source.ptrAddress = &BUCK_ISNS2_ADCBUF; // Output Voltage is Common Source
    buck.i_loop[1].controller->Ports.Source.Offset = buck.i_loop[1].feedback_offset; // Output Voltage feedback signal offset 
    buck.i_loop[1].controller->Ports.Source.NormScaler = BUCK_ISNS_NORM_SCALER; // Output voltage normalization factor bit-shift scaler 
    buck.i_loop[1].controller->Ports.Source.NormFactor = BUCK_ISNS_NORM_FACTOR; // Output voltage normalization factor fractional
    
    buck.i_loop[1].controller->Ports.AltSource.ptrAddress = NULL;
    buck.i_loop[1].controller->Ports.AltSource.Offset = 0; // not used
    buck.i_loop[1].controller->Ports.AltSource.NormScaler = 0; // not used
    buck.i_loop[1].controller->Ports.AltSource.NormFactor = 0; // not used

    // Configure controller output ports
    buck.i_loop[1].controller->Ports.Target.ptrAddress = &BUCK_PWM2_PDC; // PWM Duty Cycle is Control Target
    buck.i_loop[1].controller->Ports.Target.Offset = 0; // Static primary output value offset
    buck.i_loop[1].controller->Ports.Target.NormScaler = 0; // Primary control output normalization factor bit-shift scaler 
    buck.i_loop[1].controller->Ports.Target.NormFactor = 0x7FFF; // Primary control output normalization factor fractional 

    buck.i_loop[1].controller->Ports.AltTarget.ptrAddress = NULL;
    buck.i_loop[1].controller->Ports.AltTarget.Offset = 0; // Static secondary output value offset
    buck.i_loop[1].controller->Ports.AltTarget.NormScaler = 0; // Secondary control output normalization factor bit-shift scaler
    buck.i_loop[1].controller->Ports.AltTarget.NormFactor = 0x7FFF; // Secondary control output normalization factor fractional 
    
    // Configure controller control ports
    buck.i_loop[1].controller->Ports.ptrControlReference = &buck.i_loop[1].reference; // Set pointer to Reference
    
    // Data Input/Output Limit Configuration
    buck.i_loop[1].controller->Limits.MinOutput = buck.i_loop[1].minimum;
    buck.i_loop[1].controller->Limits.MaxOutput = buck.i_loop[1].maximum;
    buck.i_loop[1].controller->Limits.AltMinOutput = 0; // not used
    buck.i_loop[1].controller->Limits.AltMaxOutput = 0; // not used

    // ADC Trigger Control Configuration
    buck.i_loop[1].controller->ADCTriggerControl.ptrADCTriggerARegister = &BUCK_ISNS2_ADCTRIG;    // Using PGxTRIGA = ADC Trigger 1
    buck.i_loop[1].controller->ADCTriggerControl.ADCTriggerAOffset = buck.i_loop[1].trigger_offset; // Triggering at 50% off-time
    
    // Data Provider Configuration
    buck.i_loop[1].controller->DataProviders.ptrDProvControlInput = &buck.data.i_sns[1];
    buck.i_loop[1].controller->DataProviders.ptrDProvControlError = NULL;
    buck.i_loop[1].controller->DataProviders.ptrDProvControlOutput = NULL;
    
    // Cascaded Function Configuration
    buck.i_loop[1].controller->CascadeTrigger.ptrCascadedFunction = NULL;
    buck.i_loop[1].controller->CascadeTrigger.CascadedFunctionParam = 0;

    // Custom Advanced Control Settings
    buck.i_loop[1].controller->Advanced.usrParam1 = 0; // No additional advanced control options used
    buck.i_loop[1].controller->Advanced.usrParam2 = 0; // No additional advanced control options used
    buck.i_loop[1].controller->Advanced.usrParam3 = 0; // No additional advanced control options used
    buck.i_loop[1].controller->Advanced.usrParam4 = 0; // No additional advanced control options used

    // Reset Controller Status
    buck.i_loop[1].controller->status.bits.enabled = false; // Keep controller disabled
    buck.i_loop[1].controller->status.bits.swap_source = false; // use SOURCE as major control input
    buck.i_loop[1].controller->status.bits.swap_target = false; // use TARGET as major control output
    buck.i_loop[1].controller->status.bits.invert_input = false; // Do not invert input value
    buck.i_loop[1].controller->status.bits.lower_saturation_event = false; // Reset Anti-Windup Minimum Status bit
    buck.i_loop[1].controller->status.bits.upper_saturation_event = false; // Reset Anti-Windup Minimum Status bits
    buck.i_loop[1].controller->status.bits.agc_enabled = false;   // Enable Adaptive Gain Modulation by default

    // ~~~ CURRENT LOOP 2 CONFIGURATION END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~    
    return(retval);
}


/* @@<function_name>
 * ********************************************************************************
 * Summary:
 * 
 * Parameters:
 * 
 * Returns:
 * 
 * Description:
 * 
 * ********************************************************************************/
inline void appPowerSupply_CurrentBalancing(void) 
{
    static int16_t offset=0;
    
    // Current balancing is only executed in nominal running mode
    if(buck.mode != BUCK_STATE_ONLINE) 
        return;
    
    // if current in phase #1 is higher than phase current #2...
    if(buck.data.i_sns[0] > buck.data.i_sns[1]) 
    { // .. add 1 tick to phase #2 duty cycle
        offset = ((++offset>0x07) ? 0x07 : offset);
    }
    else 
    { // .. sub 1 tick from phase #2 duty cycle
        offset = ((--offset<0x00) ? 0x00 : offset);
    }

    buck.v_loop.controller->Ports.AltTarget.Offset = (uint16_t)offset;

}

/* @@<function_name>
 * ********************************************************************************
 * Summary:
 * 
 * Parameters:
 * 
 * Returns:
 * 
 * Description:
 * 
 * ********************************************************************************/
inline void appPowerSupply_CurrentSenseCalibration(void)
{

    // Current Calibration Procedure
    if ((buck.mode != BUCK_STATE_STANDBY) || 
        (buck.status.bits.cs_calib_complete) || 
        (!buck.status.bits.adc_active)
       )
    { return; }
        
        
    if (++calib_cs1.cs_calib_cnt < CS_CALIB_STEPS)
    {
        calib_cs1.cs_calib_offset += buck.data.i_sns[0]; // Read ADC offset value
        calib_cs2.cs_calib_offset += buck.data.i_sns[1]; // Read ADC offset value
    }
    else
    {
        calib_cs1.cs_calib_offset += buck.data.i_sns[0]; // Read ADC offset value
        calib_cs2.cs_calib_offset += buck.data.i_sns[1]; // Read ADC offset value

        calib_cs1.cs_calib_offset >>= 3;             // Divide accumulated ADC samples (calculate average)
        calib_cs2.cs_calib_offset >>= 3;             // Divide accumulated ADC samples (calculate average)

        buck.i_loop[0].controller->Ports.Source.Offset = calib_cs1.cs_calib_offset; // Update controller offset
        buck.i_loop[1].controller->Ports.Source.Offset = calib_cs2.cs_calib_offset;
        
        buck.status.bits.cs_calib_complete = true;   // Set CALIB_DONE flag
    }

    return;
    
}


// end of file
