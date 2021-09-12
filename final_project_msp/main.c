#include <msp430.h>
#include <intrinsics.h>// Intrinsic functions
#include <stdio.h>
#define B1 BIT3
#define UART_TXD 0x02 // TXD on P1.1 (Timer0_A.OUT0)
#define UART_RXD 0x04 // RXD on P1.2 (Timer0_A.CCI1A)
#define UART_TBIT_DIV_2     (1000000 / (9600 * 2))
#define UART_TBIT           (1000000 / 9600)

unsigned int txData;  // UART internal TX variable
unsigned char rxBuffer[100]; // Received UART character
unsigned char input;//input from user.
unsigned int cnt; //count the number of character.
unsigned int print_flag;
unsigned char str[10];

#define LED1 BIT0
#define LED2 BIT6

void TimerA_UART_init(void);
void TimerA_UART_tx(unsigned char byte);
void TimerA_UART_print(char *string);

//int print_num;
//int flag;
int mode;
int str_cnt = -1;

void main(void) {
  WDTCTL = WDTPW + WDTHOLD;  // Stop watchdog timer
  DCOCTL = 0x00;             // Set DCOCLK to 1MHz
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;

  P1OUT = 0x00;       // Initialize all GPIO
  P1SEL = UART_TXD + UART_RXD; // Use TXD/RXD pins
  P1DIR = 0xFF & ~UART_RXD; // Set pins to output

  TA1CCTL0 = CCIE; // Enable interrupts
  TA1CTL = MC_1|ID_0|TASSEL_1|TACLR;

  P1DIR|= (LED1|LED2);

  //P1OUT |= (LED1|LED2);
  P1OUT |= B1;
  //P1DIR = LED1|LED2;
  P1DIR &= ~B1;
  P1REN = B1; //Set button.
  P1IE |= B1;                    // P1.3 interrupt enabled
  P1IES |= B1;                  // P1.3 Hi/lo edge 1->Hi to low.
  P1IFG &= ~B1;               // P1.3 IFG cleared

  cnt = 0;
  //flag = 0;
  //print_flag = 0;
  //print_num = 0;
  mode = 0;

  __enable_interrupt();
  TimerA_UART_init();     // Start Timer_A UART
  TimerA_UART_print("G2xx3 TimerA UART\r\n");
  TimerA_UART_print("READY.\r\n");

  for (;;) {
    // Wait for incoming character
    /*if(flag==1){
        flag = 0;
        TimerA_UART_print("HELLO\r\n");
        flag = 0;
    }*/

    if(rxBuffer[0] == 'R'){
       mode = 1;
    }

    if(mode == 1){
        TimerA_UART_tx(rxBuffer);
        str_cnt++;

        if (rxBuffer == 13){
            str[str_cnt] = '\0';
            /*sprintf(print_cnt, "%d", str_cnt);
            TimerA_UART_print("\r\n");
            TimerA_UART_print(print_cnt);
            TimerA_UART_print("\r\n");*/
            TimerA_UART_print(str);
            TimerA_UART_print("\r\n");
            str_cnt = -1;
        }
        else{
            str[str_cnt] = rxBuffer;
        }
    }

    TimerA_UART_print(rxBuffer);
    memset(rxBuffer,0,100); //Clear buffer.
    cnt=0;
}

 //   __bis_SR_register(LPM0_bits);
    // Echo received character
}


void TimerA_UART_print(char *string) {
  while (*string) TimerA_UART_tx(*string++);
}

void TimerA_UART_init(void) {
  TA0CCTL0 = OUT;   // Set TXD idle as '1'
  TA0CCTL1 = SCS + CM1 + CAP + CCIE; // CCIS1 = 0
  // Set RXD: sync, neg edge, capture, interrupt
  TA0CTL = TASSEL_2 + MC_2; // SMCLK, continuous mode
}

void TimerA_UART_tx(unsigned char byte) {
  while (TA0CCTL0 & CCIE); // Ensure last char TX'd
  TA0CCR0 = TA0R;      // Current count of TA counter
  TA0CCR0 += UART_TBIT; // One bit time till 1st bit
  TA0CCTL0 = OUTMOD0 + CCIE; // Set TXD on EQU0, Int
  txData = byte;       // Load char to be TXD
  txData |= 0x100;    // Add stop bit to TXData
  txData <<= 1;       // Add start bit
}

#pragma vector = TIMER0_A0_VECTOR  // TXD interrupt
__interrupt void Timer_A0_ISR(void) {
  static unsigned char txBitCnt = 10;
  TA0CCR0 += UART_TBIT; // Set TA0CCR0 for next intrp
  if (txBitCnt == 0) {  // All bits TXed?
    TA0CCTL0 &= ~CCIE;  // Yes, disable intrpt
    txBitCnt = 10;      // Re-load bit counter
  } else {
    if (txData & 0x01) {// Check next bit to TX
      TA0CCTL0 &= ~OUTMOD2; // TX '1�� by OUTMODE0/OUT
    } else {
      TA0CCTL0 |= OUTMOD2;} // TX '0��
    txData >>= 1;        txBitCnt--;
  }

}

#pragma vector = TIMER0_A1_VECTOR // RXD interrupt
__interrupt void Timer_A1_ISR(void) {
  static unsigned char rxBitCnt = 8;
  static unsigned char rxData = 0;
  switch (__even_in_range(TA0IV, TA0IV_TAIFG)) {
    case TA0IV_TACCR1:     // TACCR1 - UART RXD
      TA0CCR1 += UART_TBIT;// Set TACCR1 for next int
     if (TA0CCTL1 & CAP) { // On start bit edge
        TA0CCTL1 &= ~CAP;   // Switch to compare mode
        TA0CCR1 += UART_TBIT_DIV_2;// To middle of D0
      } else {             // Get next data bit
        rxData >>= 1;
        if (TA0CCTL1 & SCCI) { // Get bit from latch
          rxData |= 0x80; }
        rxBitCnt--;
        if (rxBitCnt == 0) {  // All bits RXed?
          rxBuffer[cnt] = rxData;  // Store in global
          if(rxData == 10){ //enterj. print num.
//              rxBuffer[cnt+1] = '\r';
//              rxBuffer[cnt+1] =  '\n';
              //print_num = 1;
              //print_flag = 1;
          }
          else{
          cnt++;              //one more character.
          }
          rxBitCnt = 8;       // Re-load bit counter
          TA0CCTL1 |= CAP;     // Switch to capture
     //     __bic_SR_register_on_exit(LPM0_bits);
          // Clear LPM0 bits from 0(SR)
        }
      }
      break;
    }
}

/*#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {
  flag = 1;
//  P1OUT ^= LED1;
  P1IFG &= ~B1;             // P1.3 IFG cleared
}*/
