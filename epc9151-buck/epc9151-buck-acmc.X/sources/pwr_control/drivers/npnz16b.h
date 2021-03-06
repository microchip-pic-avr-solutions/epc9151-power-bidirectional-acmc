/* *********************************************************************************
 * z-Domain Control Loop Designer, Version 0.9.10.501
 * *********************************************************************************
 * Generic library header for z-domain compensation filter assembly functions
 * CGS Version: 2.0.13
 * CGS Date:    07/30/2020
 * ********************************************************************************/
#ifndef __SPECIAL_FUNCTION_LAYER_LIB_NPNZ16B_H__
#define __SPECIAL_FUNCTION_LAYER_LIB_NPNZ16B_H__

#include <xc.h> // include processor files - each processor file is guarded
#include <dsp.h> // include DSP data types (e.g. fractional)
#include <stdint.h> // include standard integer number data types
#include <stddef.h> // include standard definition data types
#include <stdbool.h> // include standard boolean data types (true/false)

/*@@DCLD_VERSION macro
 * *************************************************************************************************
 * Summary:
 * Generic macro allowing to identify the file version of 'npnz16b.h'
 *
 * Description:
 * This version key represents the product version of DCLD as integer number
 * of the form [MAJOR][MINOR][REVISION] => version 0.9.3.xxx would be shown as 903.
 * User code can check if the file version is compliant with the proprietary user
 * code by using pre-compiler directives such as
 *
 * #if (__DCLD_VERSION > 908)
 *  #pragma message "This code has not been tested with the recently included version of npnz16b.h"
 * #endif
 *
 * *************************************************************************************************/

#ifndef __DCLD_VERSION
    #define __DCLD_VERSION    910
#endif  // end of __DCLD_VERSION

/*@@NPNZ_STATUS_t data object
 * *************************************************************************************************
 * Summary:
 * Common Controller Status and Control Flag Bits
 *
 * Description:
 * The 16-bit wide NPNZ_STATUS_t data object holds status and control bits for
 * monitoring and control of the NPNZ16b_t controller during runtime. The lower 8 bit
 * of the status word are used for status indication while the upper 8 bit are used
 * by control bits.
 *
 * For enhanced programming convenience definitions of status and control flags are
 * provided
 *
 * *************************************************************************************************/

/* Status word values */
#define NPNZ16_CONTROL_STATUS_CLEAR     0b0000000000000000
#define NPNZ16_CONTROL_SATUATION_MSK    0b0000000000000011
#define NPNZ16_CONTROL_LSAT_ACTIVE      0b0000000000000001
#define NPNZ16_CONTROL_LSAT_CLEAR       0b0000000000000000
#define NPNZ16_CONTROL_USAT_ACTIVE      0b0000000000000010
#define NPNZ16_CONTROL_USAT_CLEAR       0b0000000000000000
#define NPNZ16_CONTROL_AGC_DISABLE      0b0000000000000000
#define NPNZ16_CONTROL_AGC_ENABLED      0b0000100000000000
#define NPNZ16_CONTROL_TARGET_DEFAULT   0b0000000000000000
#define NPNZ16_CONTROL_TARGET_SWAPED    0b0001000000000000
#define NPNZ16_CONTROL_SOURCE_DEFAULT   0b0000000000000000
#define NPNZ16_CONTROL_SOURCE_SWAPED    0b0010000000000000
#define NPNZ16_CONTROL_INV_INPUT_OFF    0b0000000000000000
#define NPNZ16_CONTROL_INV_INPUT_ON     0b0100000000000000
#define NPNZ16_CONTROL_ENABLE_OFF       0b0000000000000000
#define NPNZ16_CONTROL_ENABLE_ON        0b1000000000000000

/* Status flags (bit-field) */
typedef enum {
    NPNZ_SAT_CLEAR   = 0b0,     // No saturation condition detected
    NPNZ_SAT_ACTIVE = 0b1,      // Saturation limit violation detected
}NPNZ_CONTROL_STATUS_SATURATION_e; // NPNZ output saturation status bits
extern volatile NPNZ_CONTROL_STATUS_SATURATION_e npnzEnumControlStatusSaturation;

/* Control flags (bit-field) */
typedef enum {
    NPNZ_AGC_DISABLED   = 0b0,  // Adaptive Gain Modulation is disabled
    NPNZ_AGC_ENABLED = 0b1      // Adaptive Gain Modulation is enabled
}NPNZ_CONTROL_AGC_ENABLE_e;     // Adaptive Gain Modulation control bits
extern volatile NPNZ_CONTROL_AGC_ENABLE_e npnzEnumControlAgcEnable;

typedef enum {
    NPNZ_SOURCE_DEFAULT = 0b0,  // Controller source ports are not swapped, primary source is active input
    NPNZ_SOURCE_SWAPED = 0b1    // Controller source ports are swapped, alternate source is active input
}NPNZ_CONTROL_SOURCE_SWAP_e;    // NPNZ Source Port Swap Control bits
extern volatile NPNZ_CONTROL_SOURCE_SWAP_e npnzEnumControlSourceSwap;

typedef enum {
    NPNZ_TARGET_DEFAULT = 0b0,  // Controller target ports are not swapped, primary source is active output
    NPNZ_TARGET_SWAPED = 0b1    // Controller target ports are swapped, alternate target is active output
}NPNZ_CONTROL_TARGET_SWAP_e;    // NPNZ Target Port Swap Control bits
extern volatile NPNZ_CONTROL_TARGET_SWAP_e npnzEnumControlTargetSwap;

typedef enum {
    NPNZ_INPUT_DEFAULT = 0b0,   // Controller error value is not inverted
    NPNZ_INPUT_INVERTED = 0b1   // Controller error value is inverted
}NPNZ_CONTROL_INPUT_INV_e;      // NPNZ Error Value Inversion Control bit
extern volatile NPNZ_CONTROL_INPUT_INV_e npnzEnumControlInputInversion;

typedef enum {
    NPNZ_DISABLED = 0b0,        // Controller error value is not inverted
    NPNZ_ENABLED = 0b1          // Controller error value is inverted
}NPNZ_CONTROL_ENABLE_e;         // NPNZ Controller Enable Control bit
extern volatile NPNZ_CONTROL_ENABLE_e npnzEnumControlEnable;

/* Controller status data structure */
typedef struct NPNZ_STATUS_s {
    union {
    struct {
        volatile bool lower_saturation_event : 1; // Bit 0: control loop is clamped at minimum output level
        volatile bool upper_saturation_event : 1; // Bit 1: control loop is clamped at maximum output level
        volatile unsigned : 1; // Bit 2: reserved
        volatile unsigned : 1; // Bit 3: reserved
        volatile unsigned : 1; // Bit 4: reserved
        volatile unsigned : 1; // Bit 5: reserved
        volatile unsigned : 1; // Bit 6: reserved
        volatile unsigned : 1; // Bit 7: reserved
        volatile unsigned : 1; // Bit 8: reserved
        volatile unsigned : 1; // Bit 9: reserved
        volatile unsigned : 1; // Bit 11: reserved
        volatile bool agc_enabled: 1; // Bit 11: when set, Adaptive Gain Control Modulation is enabled
        volatile bool swap_target: 1; // Bit 12: when set, AltTarget is used as data output of controller
        volatile bool swap_source: 1; // Bit 13: when set, AltSource is used as data input to controller
        volatile bool invert_input: 1; // Bit 14: when set, most recent error input value to controller is inverted
        volatile bool enabled : 1; // Bit 15: enables/disables control loop execution
    } __attribute__((packed))bits;    // Controller status bit-field for direct bit access
    volatile uint16_t value;          // Controller status full register access
    };
} __attribute__((packed)) NPNZ_STATUS_t; // Controller status data structure

/*@@NPNZ_PORTS_t data object
 * *************************************************************************************************
 * Summary:
 * Filter Coefficient Arrays, Number Format Handling and Input/Output History Parameters
 *
 * Description:
 * The NPNZ_PORTS_t data object holds a list of nested NPNZ_PORT_t data objects, each
 * defining an individual controller input or output port. The NPNZ16b_t data objects defines
 * up to two input and two output ports of type struct NPNZ_PORT_t and one additional
 * pointer to an external, user-defined 16-bit reference source variable. Each port of type
 * struct NPNZ_PORT_t declares its individual source/target memory address, normalization
 * scaler and offset:
 *
 *  - Primary Source:   common feedback input object
 *  - Alternate Source: additional, alternate feedback input object (optional)
 *  - Primary Target:   common control output target object
 *  - Alternate Target: additional, alternate control output target object (optional)
 *  - Control Reference: pointer to external 16-bit reference source variable
 *
 * *************************************************************************************************/

/* Controller Input/Output Port */
typedef struct NPNZ_PORT_s{
    volatile uint16_t* ptrAddress; // Pointer to register or variable where the value is read from (e.g. ADCBUFx) or written to (e.g. PGxDC)
    volatile int16_t   NormScaler; // Bit-shift scaler of the Q15 normalization factor
    volatile fractional NormFactor; // Q15 normalization factor
    volatile int16_t   Offset; // Value/signal offset of this port
} __attribute__((packed)) NPNZ_PORT_t;

/* List of Controller Input/Output Ports */
typedef struct NPNZ_PORTS_s{

    volatile struct NPNZ_PORT_s Source; // Primary data input port declaration
    volatile struct NPNZ_PORT_s AltSource; // Secondary data input port declaration
    volatile struct NPNZ_PORT_s Target; // Primary data output port declaration
    volatile struct NPNZ_PORT_s AltTarget; // Secondary data output port declaration
    volatile uint16_t* ptrControlReference; // Pointer to global variable of input register holding the controller reference value (e.g. uint16_t my_ref)

} __attribute__((packed)) NPNZ_PORTS_t; // Controller block input and output port definitions

/*@@NPNZ_FILTER_PARAMS_t data object
 * *************************************************************************************************
 * Summary:
 * Filter Coefficient Arrays, Number Format Handling and Input/Output History Parameters
 *
 * Description:
 * The NPNZ_FILTER_PARAMS_t data object holds all configuration parameters of the compensation
 * filter. These parameters include pointers to external arrays of filter coefficients, error
 * and control history as well as number format normalization parameters like pre- and post-
 * scalers.
 *
 * *************************************************************************************************/

typedef struct NPNZ_FILTER_PARAMS_s{

    volatile int32_t* ptrACoefficients; // Pointer to A coefficients located in X-space
    volatile int32_t* ptrBCoefficients; // Pointer to B coefficients located in X-space
    volatile fractional* ptrControlHistory; // Pointer to n delay-line samples located in Y-space with first sample being the most recent
    volatile fractional* ptrErrorHistory; // Pointer to n+1 delay-line samples located in Y-space with first sample being the most recent

    // Array size information
    volatile uint16_t ACoefficientsArraySize; // Size of the A coefficients array in X-space
    volatile uint16_t BCoefficientsArraySize; // Size of the B coefficients array in X-space
    volatile uint16_t ControlHistoryArraySize; // Size of the control history array in Y-space
    volatile uint16_t ErrorHistoryArraySize; // Size of the error history array in Y-space

    // Feedback scaling Input/Output Normalization
    volatile int16_t normPreShift; // Normalization of ADC-resolution to Q15 (R/W)
    volatile int16_t normPostShiftA; // Normalization of A-term control output to Q15 (R/W)
    volatile int16_t normPostShiftB; // Normalization of B-term control output to Q15 (R/W)
    volatile int16_t normPostScaler; // Control output normalization factor (Q15) (R/W)

    // P-Term Coefficients (for plant measurements only)
    volatile int16_t PTermScaler; // Q15 P-Term Coefficient Bit-Shift Scaler (R/W)
    volatile int16_t PTermFactor; // Q15 P-Term Coefficient Factor (R/W)
} __attribute__((packed)) NPNZ_FILTER_PARAMS_t; // Filter parameters such as pointer to history and coefficient arrays and number scaling

/*@@NPNZ_LIMITS_t data object
 * *************************************************************************************************
 * Summary:
 * System Anti-Windup (Output Clamping) Thresholds
 *
 * Description:
 * The NPNZ_LIMITS_t data object holds all parameters required to automatically clamp the
 * most recent control output to user-defined thresholds. This data type allows the
 * definition of individual output value minima and maxima for the NPNZ controller primary
 * and alternate output port.
 *
 * This feature is optional and needs to be enabled, configured and managed manually in
 * user code.
 *
 * *************************************************************************************************/

typedef struct NPNZ_LIMITS_s{

    volatile int16_t MinOutput; // Minimum output value used for clamping (R/W)
    volatile int16_t MaxOutput; // Maximum output value used for clamping (R/W)
    volatile int16_t AltMinOutput; // Alternate minimum output value used for clamping (R/W)
    volatile int16_t AltMaxOutput; // Alternate maximum output value used for clamping (R/W)

} __attribute__((packed)) NPNZ_LIMITS_t; // Input and output clamping values

/*@@NPNZ_ADC_TRGCTRL_t data object
 * *************************************************************************************************
 * Summary:
 * Automated ADC Trigger handling
 *
 * Description:
 * The NPNZ_ADC_TRGCTRL_t data object holds all parameters required to automatically position
 * ADC triggers based on the most recent control output. This feature is used in voltage or
 * average current mode control to automatically track average values in triangular feedback
 * signal waveforms.
 *
 * This feature is optional and needs to be enabled, configured and managed manually in
 * user code.
 *
 * *************************************************************************************************/

typedef struct NPNZ_ADC_TRGCTRL_s{

    volatile uint16_t* ptrADCTriggerARegister; // Pointer to ADC trigger #1 register (e.g. TRIG1)
    volatile uint16_t ADCTriggerAOffset; // ADC trigger #1 offset to compensate propagation delays
    volatile uint16_t* ptrADCTriggerBRegister; // Pointer to ADC trigger #2 register (e.g. TRIG2)
    volatile uint16_t ADCTriggerBOffset; // ADC trigger #2 offset to compensate propagation delays

} __attribute__((packed)) NPNZ_ADC_TRGCTRL_t; // Automatic ADC trigger placement options for ADC Trigger A and B

/*@@NPNZ_DATA_PROVIDERS_t data object
 * *************************************************************************************************
 * Summary:
 * Data Provider Target Memory Addresses
 *
 * Description:
 * The NPNZ_DATA_PROVIDERS_t data object holds pointers to external, user-defined, global
 * variables allowing the NPNZ controller to push internal data to external, user-defined,
 * global variables during the execution of the NPNZ controller, resulting in an automated
 * updated of user-code variable values during runtime.
 *
 * This feature is optional and needs to be enabled, configured and managed manually in
 * user code.
 *
 * *************************************************************************************************/

typedef struct NPNZ_DATA_PROVIDERS_s{

    volatile uint16_t* ptrDProvControlInput; // Pointer to external data buffer of most recent, raw control input
    volatile uint16_t* ptrDProvControlInputCompensated; // Pointer to external data buffer of most recent, compensated control input
    volatile uint16_t* ptrDProvControlError; // Pointer to external data buffer of most recent control error
    volatile uint16_t* ptrDProvControlOutput; // Pointer to external data buffer of most recent control output

} __attribute__((packed)) NPNZ_DATA_PROVIDERS_t; // Automated data sources pushing data points to user-defined variables

/*@@NPNZ_CASCADE_TRG_t data object
 * *************************************************************************************************
 * Summary:
 * Cascaded Function Call Parameters
 *
 * Description:
 * The NPNZ_CASCADE_TRG_t data object holds all parameters required to perform cascaded
 * function call triggers. When enabled, the NPNZ controller can automatically call any
 * user-defined function after the execution of the control loop. This data object declares
 * a function pointer <ptrCascadedFunction> and one additional, 16-bit wide function parameter
 * <CascadedFunctionParam>.
 *
 * This feature is optional and needs to be enabled, configured and managed manually in
 * user code.
 *
 * *************************************************************************************************/

typedef struct NPNZ_CASCADE_TRG_s{

    volatile uint16_t ptrCascadedFunction; // Pointer to Function which should be called at the end of the control loop
    volatile uint16_t CascadedFunctionParam; // Parameter of function called (can be a pointer to a data structure)

} __attribute__((packed)) NPNZ_CASCADE_TRG_t; // Cascade triggers with parameters for next function call

/*@@NPNZ_GAIN_CONTROL_t data object
 * *************************************************************************************************
 * Summary:
 * Adaptive Gain Control Modulation Parameters
 *
 * Description:
 * The NPNZ_GAIN_CONTROL_t data object holds all parameters required to perform real-time
 * gain modulation of the z-domain feedback loop. The loop gain is modulated by multiplying
 * the result of the NPNZ controller B-term with an additional scaling factor. This scaling
 * factor is represented by a fast floating point number, consisting of a factional factor
 * <AgcFactor> between -1 and 1 and an integer bit-shift scaler <AgcScaler>.
 *
 * This feature is optional and needs to be enabled, configured and managed manually in
 * user code.
 *
 * *************************************************************************************************/

typedef struct NPNZ_GAIN_CONTROL_s{

    volatile uint16_t AgcScaler; // Bit-shift scaler of Adaptive Gain Modulation factor
    volatile fractional AgcFactor; // Q15 value of Adaptive Gain Modulation factor
    volatile fractional AgcMedian; // Q15 value of Adaptive Gain Modulation nominal operating point
    volatile uint16_t ptrAgcObserverFunction; // Function Pointer to Observer function updating the AGC modulation factor

} __attribute__((packed)) NPNZ_GAIN_CONTROL_t; // Parameter section for advanced control options

/*@@NPNZ_USER_DATA_BUFFER_t data object
 * *************************************************************************************************
 * Summary:
 * User Data Space for Advanced Control Functions
 *
 * Description:
 * The NPNZ_USER_DATA_BUFFER_t data object reserves four word of additional data space for
 * user parameters. These parameters may be handled by user code and are not assigned to any
 * specific, pre-defined functions.
 *
 * *************************************************************************************************/

typedef struct NPNZ_USER_DATA_BUFFER_s{

    volatile uint16_t usrParam1; // generic 16-bit wide, user-defined parameter #1 for advanced control options
    volatile uint16_t usrParam2; // generic 16-bit wide, user-defined parameter #2 for advanced control options
    volatile uint16_t usrParam3; // generic 16-bit wide, user-defined parameter #3 for advanced control options
    volatile uint16_t usrParam4; // generic 16-bit wide, user-defined parameter #4 for advanced control options

} __attribute__((packed)) NPNZ_USER_DATA_BUFFER_t; // Parameter section for advanced control options

/*@@NPNZ16b_t data structure
 * *************************************************************************************************
 * Summary:
 * Global NPNZ controller data object
 *
 * Description:
 * The NPNZ16b_t data object holds all configuration, status, control and monitoring values
 * of a z-domain lead-lag compensator based controller. All data types of this data object,
 * including floating, are scaled to a 16 bit number space, optimized for code execution on
 * Microchip dsPIC33 family of devices. Please refer to the description of nested data
 * structures above for more information about nested data objects.
 *
 * *************************************************************************************************/

typedef struct NPNZ16b_s {

    volatile struct NPNZ_STATUS_s status; // Control Loop Status and Control flags
    volatile struct NPNZ_PORTS_s Ports; // Controller input and output ports
    volatile struct NPNZ_FILTER_PARAMS_s Filter; // Filter parameters such as pointer to history and coefficient arrays and number scaling
    volatile struct NPNZ_LIMITS_s Limits; // Input and output clamping values
    volatile struct NPNZ_ADC_TRGCTRL_s ADCTriggerControl; // Automatic ADC trigger placement options for ADC Trigger A and B
    volatile struct NPNZ_DATA_PROVIDERS_s DataProviders; // Automated data sources pushing recent data points to user-defined variables
    volatile struct NPNZ_CASCADE_TRG_s CascadeTrigger; // Cascade triggers with parameters for next function call
    volatile struct NPNZ_GAIN_CONTROL_s GainControl; // Parameter section for advanced control options
    volatile struct NPNZ_USER_DATA_BUFFER_s Advanced; // Parameter section for advanced user control options

} __attribute__((packed)) NPNZ16b_t; // Generic nPnZ Controller Object with 16-bit fixed point coefficients, data input and data output

/* ********************************************************************************/
#endif  // end of __SPECIAL_FUNCTION_LAYER_LIB_NPNZ16B_H__ header file section


//**********************************************************************************
// Download latest version of this tool here: https://areiter128.github.io/DCLD
//**********************************************************************************

