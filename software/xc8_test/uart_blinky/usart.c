/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#include "usart.h"

void init_usart (void) {
    // USART configuration
    TXSTAbits.TX9  = 0;		// 8-bit transmission
    TXSTAbits.TXEN = 1;		// transmit enabled
    TXSTAbits.SYNC = 0;		// asynchronous mode
    TXSTAbits.BRGH = 1;		// high speed
    RCSTAbits.SPEN = 1;		// enable serial port (configures RX/DT and TX/CK pins as serial port pins)
    RCSTAbits.RX9  = 0;		// 8-bit reception
    RCSTAbits.CREN = USE_BRGH;	// enable receiver
    BAUDCON1bits.BRG16 = USE_BRG16; // 8-bit baud rate generator

    SPBRG = SBRG_VAL;		// calculated by defines
    
    TRISCbits.TRISC6 = 0;	// make the TX pin a digital output
    TRISCbits.TRISC7 = 1;	// make the RX pin a digital input
    
    /* don' use interrupts at the moment 
    // interrupts / USART interrupts configuration
    RCONbits.IPEN   = 0; // disable interrupt priority
    INTCONbits.GIE  = 1; // enable interrupts
    INTCONbits.PEIE = 1; // enable peripheral interrupts.
    PIE1bits.RCIE   = 1; // enable USART receive interrupt
    PIE1bits.TXIE   = 0; // disable USART TX interrupt
    */
    PIR1bits.RCIF = 0;
}

char putchar(unsigned char c) {
    if ( !PIE1bits.TX1IE ) {
	TXREG = c;
	PIE1bits.TX1IE = 1; // we are sending /* TODO */
        return 0;
    }
    return 1;
}

void puts(unsigned char *s) {
    char c;
    while ( ( c = *s++ ) ) {
	putchar( c );
    }
}

/* put next char onto USART */
/* TODO: check if local var tail speed up function */
char fifo_putchar(struct serial_buffer *fifo) {
    unsigned char tail=fifo->tail;
    if (fifo->head != tail) {
	if (!putchar(fifo->data[tail])) {
	    tail++;
	    tail&=SERIAL_BUFFER_SIZE_MASK;	/* wrap around if neededd */
	    fifo->tail=tail;
	    return 0;
	}
    }
    return -1;
}

/* print into circular buffer */
/* TODO: check if local var head speed up function */

char print_fifo(const unsigned char *s, struct serial_buffer *fifo) {
    unsigned char head=fifo->head;
    char c;
    while ( ( c = *s++ ) ) {
	head++;
	head&=SERIAL_BUFFER_SIZE_MASK;		/* wrap around if neededd */
	if (head != fifo->tail) {		/* space left ? */ 
	    fifo->data[head]=c;
	} else {
	    return -1;
	}
    }
    fifo->head=head;				/* only store new pointer if all is OK */
    return 0;
}

