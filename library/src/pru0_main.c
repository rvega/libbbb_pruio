#include "registers.h"

/////////////////////////////////////////////////////////////////////
// UTIL
//
#define HWREG(x) (*((volatile unsigned int *)(x)))

/////////////////////////////////////////////////////////////////////
// DECLARATIONS
//

volatile unsigned int* shared_ram;
volatile register unsigned int __R31;

void init_ocp();
void init_gpio();
void init_iep_timer();
inline void wait_for_timer();
inline void process_values();
void init_buffer();
inline void buffer_write(unsigned int *message);

/////////////////////////////////////////////////////////////////////
// ANALYZE VALUES 
//

unsigned int digital_value;
unsigned int counter;

inline void process_values(){
   unsigned int val;
   // Process values only every 8 cycles
   if(counter > 6){
      // Get valur of P9_11, GPIO0[30]
      val = ((HWREG(GPIO0+GPIO_DATAIN)&(1<<30)) != 0);
      if(val != digital_value){
         digital_value = val;   
         buffer_write(&val);
      }
   }
   counter++;
}

void init_values(){
   digital_value = 2;
   counter = 0;
}

/////////////////////////////////////////////////////////////////////
// MAIN
//

int main(int argc, const char *argv[]){
   init_values();
   init_ocp();
   init_buffer();
   init_gpio();
   init_iep_timer();

   // TODO: exit condition
   while(1){
      process_values();
      wait_for_timer(); // Timer resets itself after this
   }

   __halt();
   return 0;
}

void init_ocp(){
   // Enable OCP so we can access the whole memory map for the
   // device from the PRU. Clear bit 4 of SYSCFG register
   HWREG(PRU_ICSS_CFG + PRU_ICSS_CFG_SYSCFG) &= ~(1 << 4);

   // Pointer to shared memory region
   shared_ram = (volatile unsigned int *)0x10000;
}

/////////////////////////////////////////////////////////////////////
// RING BUFFER
//

// Communication with ARM processor is througn a ring buffer in the
// PRU shared memory area.
// shared_ram[0] to shared_ram[1023] is the buffer data.
// shared_ram[1024] is the start (read) pointer.
// shared_ram[1025] is the end (write) pointer.
//
// Messages are 32 bit unsigned ints. The 16 MSbits are the channel 
// number and the 16 LSbits are the value.
// 
// Read these:
// * http://en.wikipedia.org/wiki/Circular_buffer#Use_a_Fill_Count
// * https://groups.google.com/forum/#!category-topic/beagleboard/F9JI8_vQ-mE

unsigned int buffer_size;
volatile unsigned int *buffer_start;
volatile unsigned int *buffer_end;

void init_buffer(){
   // data in shared_ram[0] to shared_ram[127]
   buffer_size = 1024; 
   buffer_start = &(shared_ram[1024]);
   buffer_end = &(shared_ram[1025]);
   *buffer_start = 0;
   *buffer_end = 0;
}

inline void buffer_write(unsigned int *message){
   // Note that if buffer is full, messages will be dropped
   unsigned int is_full = (*buffer_end == (*buffer_start^buffer_size)); // ^ is orex
   if(!is_full){
      shared_ram[*buffer_end & (buffer_size-1)] = *message;
      // Increment buffer end, wrap around 2*size
      *buffer_end = (*buffer_end+1) & (2*buffer_size - 1);
   }
}

/////////////////////////////////////////////////////////////////////
// TIMER
//
void init_iep_timer(){
   // We'll count 83.333 micro seconds with compare0 register and 45uSec
   // with compare1 register.
   // clock is 200MHz, use increment value of 5, 
   // compare values are then 83333 and 45000

   // 1. Initialize timer to known state
   // 1.1 Disable timer counter
   HWREG(IEP + IEP_TMR_GLB_CFG) &= ~(1); 
   // 1.2 Reset counter (write 1 to clear)
   HWREG(IEP + IEP_TMR_CNT) = 0xffffffff; 
   // 1.3 Clear overflow status
   HWREG(IEP + IEP_TMR_GLB_STS) = 0; 
   // 1.4Clear compare status (write 1 to clear)
   HWREG(IEP + IEP_TMR_CMP_STS) = 0xf; 


   // 2. Set compare values 
   HWREG(IEP + IEP_TMR_CMP0) = 83333; 
   // 2.1 Compare register 1 to 45000
   /* HWREG(IEP + IEP_TMR_CMP1) = 45000; // Used when debugging timing */ 

   // 3. Enable compare events and reset counter when 
   // compare 0 event happens
   HWREG(IEP + IEP_TMR_CMP_CFG) = (1 << 1) | 1; // Compare event 0 only
   /* HWREG(IEP + IEP_TMR_CMP_CFG) = (1 << 2) | (1 << 1) | 1; // Compare evts 0 and 1 */ 
   
   // 4. Set increment value (5)
   HWREG(IEP + IEP_TMR_GLB_CFG) |= 5<<4; 

   // 5. Set compensation value (not needed now)
   HWREG(IEP + IEP_TMR_COMPEN) = 0; 
   
   // 6. Enable counter
   HWREG(IEP + IEP_TMR_GLB_CFG) |= 1; 
}

inline void wait_for_timer(){
   // Wait for compare 0 status to go high
   while((HWREG(IEP+IEP_TMR_CMP_STS) & 1) == 0){
      // nothing 
   }

   // Clear compare 0 status (write 1)
   HWREG(IEP+IEP_TMR_CMP_STS) |= 1;
}

inline void wait_for_short_timer(){
   // Wait for compare 1 status to go high
   while((HWREG(IEP+IEP_TMR_CMP_STS) & (1<<1)) == 0){
      // nothing 
   }

   // Clear compare 1 status (write 1)
   HWREG(IEP+IEP_TMR_CMP_STS) |= (1<<1);
}

/////////////////////////////////////////////////////////////////////
// GPIO (Mux control)
//
void init_gpio(){
   // See BeagleboneBlackP9HeaderTable.pdf from derekmolloy.ie
   // Way easier to read than TI's manual

   // Enable GPIO Module.
   HWREG(GPIO0 + GPIO_CTRL) = 0x00;

   // Enable clock for GPIO0 module. 
   HWREG(CM_WKUP + CM_WKUP_GPIO0_CLKCTRL) = (0x02) | (1<<18);

   // Set debounce time for GPIO0 module
   // time = (DEBOUNCINGTIME + 1) * 31uSec
   HWREG(GPIO0 + GPIO_DEBOUNCINGTIME) = 255;

   // P9_11 pin as an input, GPIO0[30], pull down. 
   HWREG(CONTROL_MODULE + CONF_P9_11) = 0x27;
   HWREG(GPIO0 + GPIO_OE) |= (1<<30);

   // Enable debounce for P9_11, GPIO0[30]
   HWREG(GPIO0 + GPIO_DEBOUNCENABLE) |= (1 << 30);
}

/////////////////////////////////////////////////////////////////////
// Analog Digital Conversion
//
inline void wait_for_adc(){
   // Wait for irqstatus[1] to go high
   while((HWREG(ADC_TSC + ADC_TSC_IRQSTATUS) & (1<<1)) == 0){
      // nothing 
   }

   // Clear status (write 1)
   HWREG(ADC_TSC + ADC_TSC_IRQSTATUS) |= (1<<1);
}

inline void adc_start_sampling(){
   // Enable steps 1 to 7
   HWREG(ADC_TSC + ADC_TSC_STEPENABLE) = 0xfe;
}

void init_adc(){
   // Enable clock for adc module.
   HWREG(CM_WKUP + CM_WKUP_ADC_TSK_CLKCTL) = 0x02;

   // Disable ADC module temporarily.
   HWREG(ADC_TSC + ADC_TSC_CTRL) &= ~(0x01);

   // To calculate sample rate:
   // fs = 24MHz / (CLK_DIV*2*Channels*(OpenDly+Average*(14+SampleDly)))
   // We want 48KHz. (Compromising to 50KHz)
   unsigned int clock_divider = 1;
   unsigned int open_delay = 0;
   unsigned int average = 0;       // can be 0 (no average), 1 (2 samples), 
                                   // 2 (4 samples),  3 (8 samples) 
                                   // or 4 (16 samples)
   unsigned int sample_delay = 0;

   // Set clock divider (set register to desired value minus one). 
   HWREG(ADC_TSC + ADC_TSC_CLKDIV) = clock_divider - 1;

   // Set values range from 0 to FFF.
   HWREG(ADC_TSC + ADC_TSC_ADCRANGE) = (0xfff << 16) & (0x000);

   // Disable all steps. STEPENABLE register
   HWREG(ADC_TSC + ADC_TSC_STEPENABLE) &= ~(0xff);

   // Unlock step config register.
   HWREG(ADC_TSC + ADC_TSC_CTRL) |= (1 << 2);

   // Set config and delays for step 1: 
   // Sw mode, one shot mode, fifo0, channel 0.
   HWREG(ADC_TSC + ADC_TSC_STEPCONFIG1) = 0 | (0<<26) | (0<<19) | (0<<15) | (average<<2) | (0);
   HWREG(ADC_TSC + ADC_TSC_STEPDELAY1)  = 0 | (sample_delay - 1)<<24 | open_delay;

   // Set config and delays for step 2: 
   // Sw mode, one shot mode, fifo0, channel 1.
   HWREG(ADC_TSC + ADC_TSC_STEPCONFIG2) = 0 | (0x0<<26) | (0x01<<19) | (0x01<<15) | (average<<2) | (0x00);
   HWREG(ADC_TSC + ADC_TSC_STEPDELAY2)  = 0 | (sample_delay - 1)<<24 | open_delay;

   // Set config and delays for step 3: 
   // Sw mode, one shot mode, fifo0, channel 2.
   HWREG(ADC_TSC + ADC_TSC_STEPCONFIG3) = 0 | (0x0<<26) | (0x02<<19) | (0x02<<15) | (average<<2) | (0x00);
   HWREG(ADC_TSC + ADC_TSC_STEPDELAY3)  = 0 | ((sample_delay - 1)<<24) | open_delay;

   // Set config and delays for step 4: 
   // Sw mode, one shot mode, fifo0, channel 3.
   HWREG(ADC_TSC + ADC_TSC_STEPCONFIG4) = 0 | (0x0<<26) | (0x03<<19) | (0x03<<15) | (average<<2) | (0x00);
   HWREG(ADC_TSC + ADC_TSC_STEPDELAY4)  = 0 | ((sample_delay - 1)<<24) | open_delay;

   // Set config and delays for step 5: 
   // Sw mode, one shot mode, fifo0, channel 4.
   HWREG(ADC_TSC + ADC_TSC_STEPCONFIG5) = 0 | (0x0<<26) | (0x04<<19) | (0x04<<15) | (average<<2) | (0x00);
   HWREG(ADC_TSC + ADC_TSC_STEPDELAY5)  = 0 | ((sample_delay - 1)<<24) | open_delay;

   // Set config and delays for step 6: 
   // Sw mode, one shot mode, fifo0, CHANNEL 6!
   HWREG(ADC_TSC + ADC_TSC_STEPCONFIG6) = 0 | (0x0<<26) | (0x06<<19) | (0x06<<15) | (average<<2) | (0x00);
   HWREG(ADC_TSC + ADC_TSC_STEPDELAY6)  = 0 | ((sample_delay - 1)<<24) | open_delay;

   // Set config and delays for step 7: 
   // Sw mode, one shot mode, fifo0, CHANNEL 5!
   HWREG(ADC_TSC + ADC_TSC_STEPCONFIG7) = 0 | (0x0<<26) | (0x05<<19) | (0x05<<15) | (average<<2) | (0x00);
   HWREG(ADC_TSC + ADC_TSC_STEPDELAY7)  = 0 | ((sample_delay - 1)<<24) | open_delay;

   // Enable tag channel id. Samples in fifo will have channel id bits ADC_CTRL register
   HWREG(ADC_TSC + ADC_TSC_CTRL) |= (1 << 1);

   // Clear End_of_sequence interrupt
   HWREG(ADC_TSC + ADC_TSC_IRQSTATUS) |= (1<<1);

   // Enable End_of_sequence interrupt
   HWREG(ADC_TSC + ADC_TSC_IRQENABLE_SET) |= (1 << 1);
   
   // Lock step config register. ACD_CTRL register
   HWREG(ADC_TSC + ADC_TSC_CTRL) &= ~(1 << 2);
   
   // Clear FIFO0 by reading from it.
   unsigned int count = HWREG(ADC_TSC + ADC_TSC_FIFO0COUNT);
   unsigned int data, i;
   for(i=0; i<count; i++){
      data = HWREG(ADC_TSC + ADC_TSC_FIFO0DATA);
   }

   // Clear FIFO1 by reading from it.
   count = HWREG(ADC_TSC + ADC_TSC_FIFO1COUNT);
   for(i=0; i<count; i++){
      data = HWREG(ADC_TSC + ADC_TSC_FIFO1DATA);
   }
   shared_ram[500] = data; // just remove unused value warning;

   // Enable ADC Module. ADC_CTRL register
   HWREG(ADC_TSC + ADC_TSC_CTRL) |= 1;
}
