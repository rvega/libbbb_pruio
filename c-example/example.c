/* Lib BBB Pruio 
 * 
 * Copyright (C) 2014 Rafael Vega <rvega@elsoftwarehamuerto.org> 
 * Copyright (C) 2014 Miguel Vargas <miguelito.vargasf@gmail.com> 
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <bbb_pruio.h>
#include <bbb_pruio_pins.h>

/////////////////////////////////////////////////////////////////////
unsigned int finished = 0;
void signal_handler(int signal){
   finished = 1;
}

/////////////////////////////////////////////////////////////////////
/* static pthread_t monitor_thread; */

/* static void* monitor_inputs(void* param){ */
/*    #<{(| #<{(| // We're getting data for 14 channels at 1500 Samples/sec. |)}># |)}># */
/*    #<{(| // Separate memory for 10 seconds of data. |)}># */
/*    #<{(| unsigned int data[15000][14]; |)}># */
/*    #<{(| int i,j; |)}># */
/*    #<{(| for(i=0; i<15000; i++) { |)}># */
/*    #<{(|    for(j=0; j<14; j++) { |)}># */
/*    #<{(|       data[i][j] = 0; |)}># */
/*    #<{(|    } |)}># */
/*    #<{(| } |)}># */
/*    #<{(|  |)}># */
/*    #<{(| unsigned int message = 0; |)}># */
/*    #<{(| int row_counter=0; |)}># */
/*    #<{(| int sample_counter=0; |)}># */
/*    #<{(| int channel_number, gpio_number, value; |)}># */
/*    #<{(| while(!finished){ |)}># */
/*    #<{(|    while(bbb_pruio_messages_are_available()){ |)}># */
/*    #<{(|       bbb_pruio_read_message(&message); |)}># */
/*    #<{(|  |)}># */
/*    #<{(|       // Message from gpio |)}># */
/*    #<{(|       if((message & (1<<31)) == 0){ |)}># */
/*    #<{(|          gpio_number = message & 0xFF; |)}># */
/*    #<{(|          value = (message>>8) & 1; |)}># */
/*    #<{(|          printf("\nDigital: 0x%X %i %i", message, gpio_number, value); |)}># */
/*    #<{(|       } |)}># */
/*    #<{(|       // Message from adc |)}># */
/*    #<{(|       else{ |)}># */
/*    #<{(|          channel_number = message & 0xF; |)}># */
/*    #<{(|          value = (0xFFF0 & message)>>4; |)}># */
/*    #<{(|  |)}># */
/*    #<{(|          data[row_counter][channel_number] = value; |)}># */
/*    #<{(|  |)}># */
/*    #<{(|          // Print every 1000th row to stdout |)}># */
/*    #<{(|          #<{(| if(row_counter%1000==0 && sample_counter==13){ |)}># |)}># */
/*    #<{(|          #<{(|    printf("\nAnalog: "); |)}># |)}># */
/*    #<{(|          #<{(|    for(j=0; j<14; j++) { |)}># |)}># */
/*    #<{(|          #<{(|       printf("%4X ", data[row_counter][j]); |)}># |)}># */
/*    #<{(|          #<{(|    } |)}># |)}># */
/*    #<{(|          #<{(| } |)}># |)}># */
/*    #<{(|  |)}># */
/*    #<{(|          sample_counter++; |)}># */
/*    #<{(|          if(sample_counter > 13){ |)}># */
/*    #<{(|             sample_counter=0; |)}># */
/*    #<{(|             row_counter++; |)}># */
/*    #<{(|             if(row_counter>14999){ |)}># */
/*    #<{(|                row_counter=0; |)}># */
/*    #<{(|             } |)}># */
/*    #<{(|          } |)}># */
/*    #<{(|       } |)}># */
/*    #<{(|    } |)}># */
/*    #<{(|    usleep(1000);  |)}># */
/*    #<{(| } |)}># */
/*    #<{(|  |)}># */
/*    #<{(| // Print everything to stdout |)}># */
/*    #<{(| #<{(| for(i=0; i<15000; i++) { |)}># |)}># */
/*    #<{(| #<{(|    printf("%i: ", i); |)}># |)}># */
/*    #<{(| #<{(|    for(j=0; j<14; j++) { |)}># |)}># */
/*    #<{(| #<{(|       printf("%4X ", data[i][j]); |)}># |)}># */
/*    #<{(| #<{(|    } |)}># |)}># */
/*    #<{(| #<{(|    printf("\n"); |)}># |)}># */
/*    #<{(| #<{(| } |)}># |)}># */
/*    #<{(|  |)}># */
/*    #<{(| // Save everything to out.txt file |)}># */
/*    #<{(| #<{(| FILE* f = fopen("./out.txt", "w"); |)}># |)}># */
/*    #<{(| #<{(| if(f==NULL){ |)}># |)}># */
/*    #<{(| #<{(|    printf("Could not open output file\n"); |)}># |)}># */
/*    #<{(| #<{(|    return 1; |)}># |)}># */
/*    #<{(| #<{(| } |)}># |)}># */
/*    #<{(| #<{(| for(i=0; i<15000; i++) { |)}># |)}># */
/*    #<{(| #<{(|    fprintf(f, "%i: ", i); |)}># |)}># */
/*    #<{(| #<{(|    for(j=0; j<14; j++) { |)}># |)}># */
/*    #<{(| #<{(|       fprintf(f, "%4X ", data[i][j]); |)}># |)}># */
/*    #<{(| #<{(|    } |)}># |)}># */
/*    #<{(| #<{(|    fprintf(f,"\n"); |)}># |)}># */
/*    #<{(| #<{(| } |)}># |)}># */
/*    #<{(| #<{(| fclose(f); |)}># |)}># */
/*    #<{(| return 0; |)}># */
/* } */
/*  */
/* static int start_monitor_thread(){ */
/*    #<{(| // TODO: set real time priority to this thread |)}># */
/*    #<{(| pthread_attr_t attr; |)}># */
/*    #<{(| if(pthread_attr_init(&attr)){ |)}># */
/*    #<{(|    return 1; |)}># */
/*    #<{(| } |)}># */
/*    #<{(| if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)){ |)}># */
/*    #<{(|    return 1; |)}># */
/*    #<{(| } |)}># */
/*    #<{(| if(pthread_create(&monitor_thread, &attr, &monitor_inputs, NULL)){ |)}># */
/*    #<{(|    return 1; |)}># */
/*    #<{(| } |)}># */
/*    #<{(|  |)}># */
/*    #<{(| return 0; |)}># */
/* } */
/*  */
/* static void stop_monitor_thread(){ */
/*    #<{(| while(pthread_cancel(monitor_thread)){} |)}># */
/* } */

int main(int argc, const char *argv[]){
   // Listen to SIGINT signals (program termination)
   signal(SIGINT, signal_handler);

   bbb_pruio_start();

   /* start_monitor_thread(); */

   // Initialize 2 pins as outputs
   if(bbb_pruio_init_gpio_pin(P9_12, BBB_PRUIO_OUTPUT_MODE)){
      fprintf(stderr, "%s\n", "Could not initialize pin P9_12");
   }
   if(bbb_pruio_init_gpio_pin(P9_14, BBB_PRUIO_OUTPUT_MODE)){
      fprintf(stderr, "%s\n", "Could not initialize pin P9_14");
   }

   // Check if library is returning adequately when trying to 
   // re-initialize a pin.
   if(!bbb_pruio_init_gpio_pin(P9_12, BBB_PRUIO_INPUT_MODE)){
      fprintf(stderr, "%s\n", "P9_12 was already initialized, should have returned error");
      exit(1);
   }
   if(bbb_pruio_init_gpio_pin(P9_14, BBB_PRUIO_OUTPUT_MODE)){
      fprintf(stderr, "%s\n", "P9_14 was already initialized as output so it's okay, should have not returned error");
      exit(1);
   }

   // Blink 2 outputs
   while(!finished){
      bbb_pruio_set_pin_value(P9_12, 0);
      bbb_pruio_set_pin_value(P9_14, 1);
      sleep(3);
      bbb_pruio_set_pin_value(P9_12, 1);
      bbb_pruio_set_pin_value(P9_14, 0);
      sleep(3);
   }

   bbb_pruio_stop();

   return 0;
}
