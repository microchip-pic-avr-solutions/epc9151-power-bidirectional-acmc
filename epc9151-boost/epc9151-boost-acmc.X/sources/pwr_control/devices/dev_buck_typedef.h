/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   dev_buck_typedef.h
 * Author: M91406
 * Comments: Type definitions for the multiphase BUCK converter data object
 * Revision history: 
 * 1.0  initial release
 * 1.1  restructured phase arrays in data object to optimize code execution and stay with unified API
 *      with regards to other power converter object libraries
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef BUCK_CONVERTER_TYPE_DEF_H
#define	BUCK_CONVERTER_TYPE_DEF_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdint.h> // include standard integer number data types
#include <stdbool.h> // include standard boolean data types (true/false)
#include <stddef.h> // include standard definition data types

#include "pwr_control/drivers/npnz16b.h"
#include "config/epc9151_r10_hwdescr.h"


#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

/*!BUCK_MODE_STATUS_t
 * ***************************************************************************************************
 * Summary:
 * Generic power controller status word
 * 
 * Description:
 * The power controller status/control word contains status (low-byte) and control bits (high-byte). 
 * 1) Status Bits:
 *      - ADC_ACTIVE: ADC is active and running (read only)
 *      - PWM_STARTED: PWM is active and running generating ADC triggers (read only)
 *      - POWERSOURCE_DETECTED: A valid power source has been detected allowing the converter to run (read only)
 *      - CS_READ: Current sense feedback ready (read only)
 *      - FORCED_SHUT_DOWN: Control(Status bit for external sowftware components forcing the converter to stay off
 *      - BUSY: Converter is currently going through an internal process (e.g. ramp up/down) (read only)
 * 
 * 2) Control Bits
 *      - ENABLE: Enables/Disables the power converter
 *      - AUTORUN: When set, the power converter will automatically start up once all status bits are set accordingly
 *      - GO: Control bit to manually start the power converter if (AUTOSTART=0)
 *  
 * *************************************************************************************************** */

#define MPHBUCK_NO_OF_PHASES            BUCK_NO_OF_PHASES

// Controller Status Bits
#define BUCK_STAT_READY                  0b0000000000000001
#define BUCK_STAT_ADC_ACTIVE             0b0000000000000010
#define BUCK_STAT_PWM_ACTIVE             0b0000000000000100
#define BUCK_STAT_POWERSOURCE_DETECTED   0b0000000000001000
#define BUCK_STAT_CS_SENSE_READY         0b0000000000010000
    
#define BUCK_STAT_FORCED_SHUT_DOWN       0b0000000010000000
#define BUCK_STAT_BUSY                   0b0000000100000000

// Controller Control Bits
#define BUCK_STAT_GO                     0b0010000000000000
#define BUCK_STAT_AUTORUN                0b0100000000000000
#define BUCK_STAT_NO_AUTORUN             0b0000000000000000

#define BUCK_STAT_ENABLED                0b1000000000000000
#define BUCK_STAT_DISABLED               0b0000000000000000

typedef union 
{
    struct{
        volatile bool ready:1;          // Bit #0: status bit, indicating buck converter is initialized and ready to run
        volatile bool adc_active:1;	    // Bit #1: indicating that ADC has been started and samples are taken
        volatile bool pwm_active:1;     // Bit #2: indicating that PWM has been started and ADC triggers are generated
        volatile bool power_source_detected:1;	// Bit #3:  indicating that a valid power source was detected
        volatile bool cs_calib_complete :1; // Bit #4: indicating that current sensor calibration has completed
        volatile unsigned :1;           // Bit #5: (reserved)
        volatile unsigned :1;           // Bit #6: (reserved)
        volatile bool fault_active :1;  // Bit #7: Flag bit indicating system is in enforced shut down mode (usually due to a fault condition)

        volatile bool busy :1;      // Bit #8:  Flag bit indicating that the state machine is executing a process (e.g. startup-ramp)
        volatile bool cs_calib :1;  // Bit #9:  Flag bit indicating that current sensors need to calibrated
        volatile unsigned :1;       // Bit #10: (reserved)
        volatile unsigned :1;       // Bit #11: (reserved)
        volatile unsigned :1;       // Bit #12: (reserved)
        volatile bool GO :1;        // Bit #13: When set, the GO-bit fires up the power supply
        volatile bool autorun :1;   // Bit #14: Control bit determining if charger is starting automatically or on command (using the GO bit)
        volatile bool enabled :1;    // Bit #15: Control bit enabling/disabling the charger port
    } __attribute__((packed)) bits; // data structure for single bit addressing operations

	volatile uint16_t value; // buffer for 16-bit word read/write operations
    
} BUCK_CONVERTER_STATUS_t;


/*!BUCK_MODE_STATE_t
 * ***************************************************************************************************
 * Summary:
 * Generic power controller state-machine states
 * 
 * Description:
 * This enumeration is listing all defined states supported by the power controller state-machine.
 * The state machine handles the initialization of the power controller, stand-by, start up procedure
 * including Power-On-Delay, Ramp-Up and Power Good Delay until it ends up in a continuous operating
 * state. When reference values are changed while running, the state machine will tune into the new
 * reference values using the slew rates defined for the startup phase. 
 *
 * For more information on each state, please read the related sections.
 *  
 * *************************************************************************************************** */
typedef enum {
    BUCK_STATE_INITIALIZE     = 0,  // power converter control state #0: initialize variables and hijack controller reference
    BUCK_STATE_RESET          = 1,  // power converter control state #1: Initializing variable but bypassing delays
    BUCK_STATE_STANDBY        = 2,  // power converter control state #2: standing by, ready to launch, waiting for GO (no action)
    BUCK_STATE_POWER_ON_DELAY = 3,  // power converter control state #3: power on delay (no action)
    BUCK_STATE_LAUNCH_V_RAMP  = 4,  // power converter control state #4: turn on PWM outputs and enable controller
    BUCK_STATE_V_RAMP_UP      = 5,  // power converter control state #5: perform output voltage ramp up based on parameters and system response 
    BUCK_STATE_I_RAMP_UP      = 6,  // power converter control state #6: perform output current ramp up based on parameters and system response (average current mode only)
    BUCK_STATE_PWRGOOD_DELAY  = 7,  // power converter control state #7: Output reached regulation point but waits until things have settled
    BUCK_STATE_ONLINE         = 8,  // power converter control state #8: Output in regulation and power is OK (normal continuous operation)
    BUCK_STATE_SUSPEND        = 9   // power converter control state #9: state machine will be reset without POD and PG delays
} BUCK_MODE_STATE_e;


/*!BUCK_STARTUP_SETTINGS_t
 * ***************************************************************************************************
 * Summary:
 * Generic power controller startup settings
 * 
 * Description:
 * This data structure is used to set the startup settings such as power on delay, power good delay
 * and ramp up time. It further covers private values like startup counters and reference values
 * for voltage and current, which are used internally by the controller (read only) but are still
 * accessible for external code modules for monitoring, diagnostics and fault handling purposes.
 * 
 * *************************************************************************************************** */

typedef struct {
    volatile uint16_t counter; // Soft-Start Execution Counter (read only)
    volatile uint16_t period;  // Soft-Start Period (POD, RAMP PERIOD, PGD, etc.)
    volatile uint16_t reference; // Internal dummy reference used to increment/decrement controller reference
    volatile uint16_t ref_inc_step; // Size/value of one reference increment/decrement or this period
} BUCK_STARTUP_PERIOD_HANDLER_t; // Power converter soft-start auxiliary variables

typedef struct {
    volatile BUCK_STARTUP_PERIOD_HANDLER_t power_on_delay;
    volatile BUCK_STARTUP_PERIOD_HANDLER_t power_good_delay;
    volatile BUCK_STARTUP_PERIOD_HANDLER_t i_ramp;
    volatile BUCK_STARTUP_PERIOD_HANDLER_t v_ramp;
} BUCK_CONVERTER_STARTUP_t; // Power converter start-up settings and variables

// ==============================================================================================
// BUCK converter runtime data object 
// ==============================================================================================

typedef struct {
    volatile uint16_t i_sns[MPHBUCK_NO_OF_PHASES];    // BUCK output current
    volatile uint16_t i_out;    // BUCK common output current
    volatile uint16_t v_in;     // BUCK input voltage
    volatile uint16_t v_out;    // BUCK output voltage
    volatile uint16_t temp;     // BUCK board temperature
}BUCK_CONVERTER_DATA_t;         // BUCK runtime data

/*!BUCK_CONTROL_t
 * ***************************************************************************************************
 * Summary:
 * Generic power controller control settings
 * 
 * Description:
 * This data structure is used to set the overall settings to allow external software instances 
 * to control the power control object, such as voltage and current references.
 *  
 * *************************************************************************************************** */
typedef enum {
    BUCK_CONTROL_MODE_VMC = 0,  // Voltage Mode Control
//    BUCK_CONTROL_MODE_PCMC = 1, // Peak Current Mode Control (not supported yet)
    BUCK_CONTROL_MODE_ACMC = 2  // Average Current Mode Control
} BUCK_CONTROL_MODE_e;

typedef struct {
    volatile BUCK_CONTROL_MODE_e control_mode; // Fundamental control mode 
    volatile uint16_t v_ref; // User reference setting used to control the power converter controller
    volatile uint16_t i_ref; // User reference setting used to control the power converter controller
    volatile uint16_t phases; // number of converter phases
} BUCK_CONVERTER_CONTROL_t;


/*!MPHBUCK_LOOP_SETTINGS_t
 * ***************************************************************************************************
 * Summary:
 * Generic power control loop settings
 * 
 * Description:
 * This data structure is used to set the control loop settings such as pointers to controller 
 * objects and its function calls as well as basic user settings such as reference, feedback
 * signal offsets, trigger delays and minimum/maximum output clamping values.
 * 
 * *************************************************************************************************** */
typedef struct {
    // Properties (user settings)
    volatile uint16_t reference; // Control loop reference variable
    volatile uint16_t feedback_offset; // Feedback offset value for calibration or bi-direction feedback signals
    volatile uint16_t trigger_offset; // ADC trigger offset value for trigger fine-tuning
    volatile int16_t  minimum; // output clamping value (minimum)
    volatile uint16_t maximum; // output clamping value (maximum)
    // Control Loop Object
    volatile NPNZ16b_t* controller; // pointer to control loop object data structure
    // Function pointers
    volatile uint16_t (*ctrl_Initialization)(volatile NPNZ16b_t*); // Function pointer to INIT routine
    void (*ctrl_Reset)(volatile NPNZ16b_t*); // Function pointer to RESET routine
    void (*ctrl_Update)(volatile NPNZ16b_t*); // Function pointer to UPDATE routine
    void (*ctrl_Precharge)(volatile NPNZ16b_t*, volatile fractional, volatile fractional); // Function pointer to PRECHARGE routine
} BUCK_LOOP_SETTINGS_t; // User defined settings for control loops; 

/*!MPHBUCK_SWITCH_NODE_SETTINGS_t
 * ***************************************************************************************************
 * Summary:
 * Generic power converter switch-node specifications
 * 
 * Description:
 * This data structure is used to set the converter switch-node specifications declaring which
 * PWM channel is used as well as its switching frequency, phase-shift, dead times and duty ratio
 * limits.
 * 
 * *************************************************************************************************** */
typedef struct {
    volatile uint16_t pwm_instance; // number of the PWM channel used
    volatile uint16_t gpio_instance; // GPIO instance of the selected PWM generator
    volatile uint16_t gpio_high; // GPIO port pin-number of PWMxH of the selected PWM generator
    volatile uint16_t gpio_low; // GPIO port pin-number of PWMxL of the selected PWM generator
    volatile bool     master_period; // Selecting MASTER or Individual period register
    volatile uint16_t period; // Switching period
    volatile uint16_t phase; // Switching signal phase-shift
    volatile uint16_t duty_ratio_init; // Initial duty cycle when the PWM module is being turned on
    volatile uint16_t duty_ratio_min; // Absolute duty cycle minimum during normal operation
    volatile uint16_t duty_ratio_max; // Absolute duty cycle maximum during normal operation
    volatile uint16_t dead_time_rising; // Dead time setting at rising edge of a half-bridge drive
    volatile uint16_t dead_time_falling; // Dead time setting at falling edge of a half-bridge drive
    volatile uint16_t leb_period; // Leading-Edge Blanking period
    volatile uint16_t trigger_scaler; // PWM triggers for ADC will be generated every n-th cycle
    volatile uint16_t trigger_offset;  // PWM triggers for ADC will be offset by n cycles
} BUCK_SWITCH_NODE_SETTINGS_t; // Switching signal timing settings

/*!MPHBUCK_FEEDBACK_SETTINGS_t
 * ***************************************************************************************************
 * Summary:
 * Generic power converter feedback specifications
 * 
 * Description:
 * This data structure is used to set the converter feedback specifications declaring which
 * ADC channels are used including the individual AD input configuration such as trigger mode,
 * input mode, result format and value normalization.
 * 
 * *************************************************************************************************** */

typedef struct {
    volatile int16_t factor; // Fractional scaling factor (range -1 ... 0.99969)
    volatile int16_t scaler; // Feedback number scaler used for number normalization
    volatile int16_t offset; // Signal offset as signed integer to be subtracted from ADC input
} BUCK_ADC_INPUT_SCALING_t; // ADC input signal scaling = (ADCBUF - <offset>) * <factor> >> 2^<scaler>

typedef struct {
    volatile bool enabled; // input channel enable bit
    volatile uint16_t* adc_buffer; // pointer to ADC result buffer
    volatile uint16_t gpio_instance; // GPIO instance of the selected PWM generator
    volatile uint8_t adc_input; // number of the ADC input channel used
    volatile uint8_t adc_core; // number of the ADC core connected to the selected channel
    volatile uint8_t trigger_source; // input channel trigger source
    volatile bool interrupt_enable; // input channel interrupt enable bit
    volatile bool early_interrupt_enable; // input channel early interrupt enable bit
    volatile bool differential_input; // input channel differential mode enable bit
    volatile bool signed_result; // input channel singed result mode enable bit
    volatile bool level_trigger; // input channel level trigger mode enable bit
    volatile BUCK_ADC_INPUT_SCALING_t scaling; // normalization scaling settings
} BUCK_ADC_INPUT_SETTINGS_t; // ADC input channel configuration

typedef struct {
    volatile BUCK_ADC_INPUT_SETTINGS_t ad_vin;  // ADC input sampling input voltage
    volatile BUCK_ADC_INPUT_SETTINGS_t ad_vout; // ADC input sampling output voltage
    volatile BUCK_ADC_INPUT_SETTINGS_t ad_isns[MPHBUCK_NO_OF_PHASES]; // ADC input sampling phase current
    volatile BUCK_ADC_INPUT_SETTINGS_t ad_temp; // ADC input sampling temperature
} BUCK_FEEDBACK_SETTINGS_t;

/*!MPHBUCK_GPIO_SETTINGS_t
 * ***************************************************************************************************
 * Summary:
 * Generic power converter GPIO specifications
 * 
 * Description:
 * This data structure is used to set the converter GPIO specifications declaring which
 * if and which additional GPIOs are used by the converter controller, such as POWER_GOOD.
 * 
 * *************************************************************************************************** */
typedef struct {
    volatile bool enabled; // Specifies, if this IO is used or not
    volatile uint16_t port; // GPIO port instance number (0=A, 1=B, 2=C, etc.)
    volatile uint16_t pin; // GPIO port pin number
    volatile uint16_t polarity; // Output polarity, where 0=ACTIVE HIGH, 1=ACTIVE_LOW
    volatile uint16_t io_type; // Input/Output definition (0=output, 1=input)
} BUCK_GPIO_INSTANCE_t; // GPIO instance of the converter control GPIO

typedef struct {
    volatile BUCK_GPIO_INSTANCE_t Enable; // External ENABLE input
    volatile BUCK_GPIO_INSTANCE_t PowerGood; // Power Good Output
} BUCK_GPIO_SETTINGS_t; // GPIO instance of the converter control GPIO

// ==============================================================================================
// BUCK converter state machine data structure and defines
// ==============================================================================================

typedef struct {
    
    volatile BUCK_CONVERTER_STATUS_t status; // BUCK operation status bits
    volatile BUCK_MODE_STATE_e mode; // BUCK state machine state
    volatile BUCK_CONVERTER_STARTUP_t startup; // BUCK startup timing settings 
    volatile BUCK_CONVERTER_CONTROL_t set_values; // Control field for global access to references
    volatile BUCK_CONVERTER_DATA_t data;     // BUCK runtime data
    volatile BUCK_FEEDBACK_SETTINGS_t feedback; // BUCK converter feedback settings

    volatile BUCK_SWITCH_NODE_SETTINGS_t sw_node[MPHBUCK_NO_OF_PHASES]; // BUCK converter switch node settings
    volatile BUCK_GPIO_SETTINGS_t gpio; // BUCK converter additional GPIO specification
    
    volatile BUCK_LOOP_SETTINGS_t v_loop; // BUCK voltage control loop object
    volatile BUCK_LOOP_SETTINGS_t i_loop[MPHBUCK_NO_OF_PHASES]; // BUCK Current control loop objects
    
} BUCK_POWER_CONTROLLER_t; // BUCK control & monitoring data structure


/*!BUCK_POWER_CONTROLLER_t data structure
 * *************************************************************************************************
 * Summary:
 * Global data object for the BUCK CONVERTER 
 * 
 * Description:
 * the 'buck' data object holds all status, control and monitoring values of the BUCK power 
 * controller. The BUCK_POWER_CONTROLLER_t data structure is defined in drv_buck_converter.h.
 * Please refer to the comments on top of this file for further information.
 *  
 * *************************************************************************************************/
extern volatile BUCK_POWER_CONTROLLER_t buck;

#else
    #pragma message "Warning: dev_buck_typedef.h inclusion bypassed"
#endif	/* BUCK_CONVERTER_TYPE_DEF_H */

