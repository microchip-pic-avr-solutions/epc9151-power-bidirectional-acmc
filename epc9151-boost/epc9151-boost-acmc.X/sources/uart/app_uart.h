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
 * File:   app_uart.h
 * Author: 
 * Comments: global uart handler application layer API
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef APPLICATION_LAYER_UART_HEADER_H
#define	APPLICATION_LAYER_UART_HEADER_H 

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdint.h> // include standard integer data types
#include <stdbool.h> // include standard boolean data types
#include <stddef.h> // include standard definition data types



#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */
    
typedef enum {
    BUCK_VOLTAGE_REG    = 0,  // buck mode, voltage regulation
    BUCK_CURRENT_REG    = 1,  // buck mode, constant current output (no voltage regulation)
    BOOST_VOLTAGE_REG   = 2,  // boost mode, voltage regulation
    BOOST_CURRENT_REG   = 3,  // boost mode, constant current output (no voltage regulation)
} MODE_COMMAND_e;

typedef union{

	struct {
		volatile bool rx_status : 1;         // Bit 0: Flag bit indicating if data receiving has been completed
		volatile bool rx_active : 1;         // Bit 1: Flag bit indicating if data receiving is in process
		volatile bool tx_status : 1;         // Bit 0: Flag bit indicating if data transmitting has been completed
		volatile bool tx_active : 1;         // Bit 1: Flag bit indicating if data transmitting is in process
		volatile unsigned : 4;					// Bit <7:4>: (reserved)
//		volatile FLT_COMPARE_TYPE_e type: 3;	// Bit <10:8>: Fault check comparison type control bits
		volatile unsigned : 7;					// Bit <14:8> (reserved)
		volatile bool enabled : 1;              // Bit 15: Control bit enabling/disabling monitoring of the fault object
	} __attribute__((packed)) bits; // Uart object status bit field for single bit access  

	volatile uint16_t value;		// Uart object status word  

} UART_OBJECT_STATUS_t;	// Uart object status

typedef struct {
	volatile UART_OBJECT_STATUS_t status; // Status word of this uart object
    volatile MODE_COMMAND_e mode;   // Request operation mode
	volatile uint16_t* data1;       // Pointer to the 1st variable for transmitting
	volatile uint16_t* data2;       // Pointer to the 2nd variable for transmitting
	volatile uint16_t* data3;       // Pointer to the 3rd variable for transmitting    
	volatile uint16_t* data4;       // Pointer to the 4th variable for transmitting

	volatile uint8_t tx_data[16];   // Encoded data for transmission
    volatile uint8_t rx_data[4];    // Encoded received data
    volatile uint8_t rx_checksum;   // Calculated received data checksum
    
    volatile uint16_t rx_decoded;   // Decoded rx data value
	volatile uint16_t counter;		// TX/RX byte counter (for more than 1 byte)

} UART_OBJECT_t;

// Public Function Prototypes
extern volatile uint16_t uart_calc_checksum(volatile UART_OBJECT_t* uartobj);
extern volatile uint16_t uart_check(volatile UART_OBJECT_t* uartobj);

// Public Variable Declaration
extern volatile UART_OBJECT_t uartobj_Buck;


// PUBLIC FUNCTION PROTOTYPE DECLARATIONS
extern volatile uint16_t appUart_Initialize(void);
extern volatile uint16_t appUart_Execute(void);
extern volatile uint16_t appUart_Dispose(void);

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* APPLICATION_LAYER_UART_HEADER_H */

