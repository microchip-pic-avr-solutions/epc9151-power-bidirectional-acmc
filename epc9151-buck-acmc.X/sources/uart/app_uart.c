/*
 * File:   faults.c
 * Author: M91406
 *
 * Created on March 12, 2020, 11:38 AM
 */

#include <stddef.h>

#include "app_uart.h"
#include "config/epc9143_r40_hwdescr.h"
#include "pwr_control/app_power_control.h"


// Define uart object
volatile UART_OBJECT_t uartobj_Buck;

volatile uint16_t uart_check(volatile UART_OBJECT_t* uartobj) {
    volatile char ReceivedChar;
    volatile uint16_t _i=0;
    // If the uart object is not initialized, exit here with error
    if (uartobj == NULL)
        return(0);

    // If FAULT CHECK is disabled, exit here
    if (!uartobj->status.bits.enabled) {
        uartobj->counter = 0;                       // Clear Counter
        uartobj->status.bits.rx_active = false;     // Clear rx active flag
        uartobj->status.bits.rx_status = false;     // Clear rx status flag
        uartobj->status.bits.tx_active = false;     // Clear tx active flag
        uartobj->status.bits.tx_status = false;     // Clear tx status flag        
        return(1);  // Return success
    }
    
    // If the source data is not initialized, exit here with error
    if ( (uartobj->data1 == NULL) ||
         (uartobj->data2 == NULL) ||
         (uartobj->data3 == NULL) ||
         (uartobj->data4 == NULL))
        return(0);    
    
    // If not transmitting, check for receiving
    if (uartobj->status.bits.tx_active == false) {
        /* Check for receive errors */
        if(U1STAbits.FERR == 1)
        {
            return(0);
        }
        /* Must clear the overrun error to keep UART receiving */
        if(U1STAbits.OERR == 1)
        {
            U1STAbits.OERR = 0;
            return(1);
        }
        /* Get the data */
        if(!(U1STAHbits.URXBE == 1))
        {
            ReceivedChar = U1RXREG;
            if (uartobj->status.bits.rx_active == false) { // receiving first character
                if (ReceivedChar == 'A') { // acquire data
                    *uartobj->tx_data = (*uartobj->data1 & 0xFF); // low byte
                    *(uartobj->tx_data + 1) = (*uartobj->data1 >> 8); // high byte
                    *(uartobj->tx_data + 2) = (*uartobj->data2 & 0xFF); // low byte
                    *(uartobj->tx_data + 3) = (*uartobj->data2 >> 8);
                    *(uartobj->tx_data + 4) = (*uartobj->data3 & 0xFF);
                    *(uartobj->tx_data + 5) = (*uartobj->data3 >> 8);             
                    *(uartobj->tx_data + 6) = (*uartobj->data4 & 0xFF);
                    *(uartobj->tx_data + 7) = (*uartobj->data4 >> 8);
                    *(uartobj->tx_data + 8) = 0;
                    *(uartobj->tx_data + 9) = 0;
                    *(uartobj->tx_data + 10) = 0;
                    *(uartobj->tx_data + 11) = 0;
                    *(uartobj->tx_data + 12) = 0;
                    *(uartobj->tx_data + 13) = 0;
                    *(uartobj->tx_data + 14) = 0;
                    *(uartobj->tx_data + 15) = 0;

                    // start 1st group transmission 
                    uartobj->status.bits.tx_active = true;
                    uartobj->counter = 1;

                    for (_i=0; _i<8; _i++) 
                    {
                        U1TXREG = *(uartobj->tx_data + _i);
                    }
                }
                else if (ReceivedChar == 'C') { // send firmware version number
                    U1TXREG = 'v';
                    U1TXREG = (FIRMWARE_VER_NUM0 & 0xFF);
                    U1TXREG = (FIRMWARE_VER_NUM0 >> 8);
                    U1TXREG = (FIRMWARE_VER_NUM1 & 0xFF);
                    U1TXREG = (FIRMWARE_VER_NUM1 >> 8);
                    U1TXREG = (FIRMWARE_VER_NUM2 & 0xFF);
                    U1TXREG = (FIRMWARE_VER_NUM2 >> 8);
                    U1TXREG = FIRMWARE_RET_CKSUM;
                }
                else if (ReceivedChar == 'K') { // set buck voltage regulation
                    // start rx sequence by setting rx_active
                    *uartobj->rx_data = ReceivedChar;
                    uartobj->status.bits.rx_active = true;
                    uartobj->counter = 1;                
                    uartobj->mode = BUCK_VOLTAGE_REG;

                }
                else if (ReceivedChar == 'T') { // set boost voltage regulation
                    *uartobj->rx_data = ReceivedChar;
                    uartobj->status.bits.rx_active = true;
                    uartobj->counter = 1;                
                    uartobj->mode = BOOST_VOLTAGE_REG;
                }
            }  else  { // rx is active, keep receiving more data
                *(uartobj->rx_data + uartobj->counter) = ReceivedChar;
                uartobj->counter++;
                if (uartobj->counter > 3) { // received total 4 bytes
                    uartobj->status.bits.rx_active = false;
                    uartobj->counter = 0;
                    // calculate checksum
                    uart_calc_checksum(&uartobj_Buck);
                    if ( (uartobj->rx_checksum) == *(uartobj->rx_data + 3) ) {
                        // checksum is correct, decode integer
                        uartobj->rx_decoded = *(uartobj->rx_data + 1) + (*(uartobj->rx_data + 2) << 8);
                        // send acknowledgment data
                        *uartobj->tx_data = *uartobj->rx_data; // low byte
                        *(uartobj->tx_data + 1) = (uartobj->rx_decoded & 0xFF); // high byte
                        *(uartobj->tx_data + 2) = (uartobj->rx_decoded >> 8); // high byte
                        *(uartobj->tx_data + 3) = '0';
                        *(uartobj->tx_data + 4) = '0';
                        *(uartobj->tx_data + 5) = 'a';             
                        *(uartobj->tx_data + 6) = 'c';
                        *(uartobj->tx_data + 7) = 'k';                    
                        for (_i=0; _i<8; _i++) 
                        {
                            U1TXREG = *(uartobj->tx_data + _i);
                        }                               
                        // set converter mode
                        switch (uartobj->mode) {
                            case BUCK_VOLTAGE_REG:
                                // switch over to buck if not already in buck
                                // update vout reference
                                buck.set_values.v_ref = uartobj->rx_decoded;
                                break;
                            case BUCK_CURRENT_REG: // future support
                                // switch over to constant current output mode 
                                // disable vloop, set i_ref
                                break;
                            case BOOST_VOLTAGE_REG:
                                // switch over to boost if not already in boost
                                // update vout reference
                                break;
                            case BOOST_CURRENT_REG: // future support
                                // disable vloop, set i_ref
                                break;
                        }
                    }
               
                }

            }
        }
    } else { // transmission is active
        if(U1STAbits.TRMT == 1) { // TX buffer empty, we can write
            if (uartobj->counter == 1) { // transmit the 2nd group
                for (_i=8; _i<16; _i++) 
                {
                    U1TXREG = *(uartobj->tx_data + _i);
                }
                
                // finish transmission
                uartobj->status.bits.tx_active = false;
                uartobj->counter = 0;
            }
        }
    }
    return (1);
}

volatile uint16_t uart_calc_checksum(volatile UART_OBJECT_t* uartobj) {
    volatile uint16_t _sum = 0;
    _sum = *(uartobj->rx_data) + *(uartobj->rx_data + 1) + *(uartobj->rx_data + 2);
    uartobj->rx_checksum = (_sum & 0xFF);
    return (1);
}


volatile uint16_t appUart_Initialize(void) 
{
    // Initialize buck uart object
    uartobj_Buck.data1 = &buck.data.v_out;      // Set pointer to variable
    uartobj_Buck.data2 = &buck.data.v_in;       // Set pointer to variable
    uartobj_Buck.data3 = &buck.data.i_sns[0];   // Set pointer to variable
    uartobj_Buck.data4 = &buck.data.i_sns[1];   // Set pointer to variable
    
    uartobj_Buck.counter = 0;
    uartobj_Buck.status.bits.rx_active = 0;
    uartobj_Buck.status.bits.tx_active = 0;

    uartobj_Buck.status.bits.enabled = true;    // Enable uart 

    return(1);
}

volatile uint16_t appUart_Dispose(void) 
{
    
    return(1);
}


volatile uint16_t appUart_Execute(void) 
{
    volatile uint16_t fres=1;
    
    // Call uart check
    fres &= uart_check(&uartobj_Buck);
 
    return (fres);
}
