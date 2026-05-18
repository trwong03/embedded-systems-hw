//mpaland_printf_services.c wmh 2023-11-12 : custom functions to create message strings for devices
//  MoT devices wishing to use mpaland's printf()/sprintf()/snprintf/vsnprintf() functions will create a custom functions here.
//
//MoTdevice_printfDemo.c wmh 2023-11-07 : helper function for MoTdevice_printfDemo.S 
#include "mpaland_printf.h"
#include <stdint.h>	//for uint8_t, uint16_t, uint32_t  etc. 
#include <stddef.h> 
#include <string.h>
//#include "MoTstructures.h"			


//extern MoT_printbuffer_t device7_printbuf1;	//in MoTdevice7_printfDemo.S
/*****  from  MoTstructures.h wmh 2023-11-12
typedef struct MoT_printbuffer {
	uint32_t	size;		//_count_ of characters in MoT printbuffer ( contents of \bufname\()_size)
	char  *	data;			//_address_ of data in MoT printbuffer is at \bufname\()_data)
} MoT_printbuffer_t;
*****/


/* I think the below are from D:\_umd\2026-01-01\ENEE452\_work\0306_myRTOS\MoT_Nucleo-G491_ws3i1a_Copy
uint32_t device7_print2buf1 (char * buf)
{
	extern uint32_t hexintvar,decintvar; extern float floatvar;

	sprintf(buf,"device7: hexintvar= 0x%08X, decintvar= %i, floatvar=%f\n",hexintvar,decintvar,floatvar); //create string for MoT message in MoTdevice7_printfDemo01.S
	return strlen(buf);
}

uint32_t device9_printMSG1(char * buf)
{
	extern int16_t ADC1_avg; //defined in main, updated in DMA1_Channel1_IRQHandler, to be reported by device9

	sprintf(buf,"average of ADC1 readings= %d\n", ADC1_avg);
	return strlen(buf);
}
*/
