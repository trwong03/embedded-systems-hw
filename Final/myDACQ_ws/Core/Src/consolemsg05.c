/* consolemsg05.c wmh 2026-03-23 : polling-only console interface
 msgSend(List) and msgPost() functions which run in main()'s while(1)


 note: we're trying to stay close to how MoT's messaging method works. A future goal is to integrate them.
*/
#include "main.h"
//#include ".\mpaland_printf.h"
#include "stm32l4xx_hal_uart.h"
#include "usart.h"
#include <string.h>
#include <stdarg.h>
#include ".\mpaland_printf.h"

typedef struct listLink {	//non-MoT version
	struct listLink *next;	//absolute address of next link in list (points to itself if at end of list
	char *addr;			//absolute address of message data
	uint32_t count;			//maintain compatibility with MoT listLink definition
	uint32_t info;			// ""
	} msgLink_type;

//from current MoTstructures.h (see for example  D:\_umd\2026-01-01\ENEE440\_work\0305_class\MoT_Nucleo-G491_ws1\Project\Inc\) 
typedef struct listAnchor {		// creates device-local data structures for list of messages or commands
	msgLink_type *listheadp;		// offset 0	in link holds address of head link in a linked-list design
	msgLink_type *listtailp;		// offset 4	in link holds address of tail link in a linked-list design
	char * byteptr;			// holds current play-position in the buffer of the head entry of the list
	uint32_t bytecount;			// holds remains-to-play count of the buffer of head entry of list
	//we probably should have some way to report list status (empty/ready/...?)
} msgList_type;

typedef enum	
{
	LINK_IS_BUSY = -3,
	LINK_SNPRINTF_FAILED,
	LINK_MSG_TOO_LONG,
	LINK_IS_INSTALLED	/* should = 0 */
} CONSOLEMSG_returncodes;

typedef enum
{
	UART_TX_FAILED = -1,	//*values -1 to +4 */
	UART_TX_OK,
	UART_TX_IS_BUSY,
	UART_LIST_WAS_UPDATED,
	UART_LIST_IS_EMPTY,
	UART_LIST_DEFAULT_RETURN	/* should never occur */
} MSGSEND_RETURNCODES;

int msgSend(msgList_type *pList) 
{
	msgLink_type *ptemp;
	if(pList->listtailp == NULL) return UART_LIST_IS_EMPTY;
	if(pList->bytecount >0) { //more to send from current message
		if( HAL_UART_GetState(&huart1) == HAL_UART_STATE_BUSY_TX) return UART_TX_IS_BUSY; // stm32l4xx_hal_uart.h line 351
		else { //here to send the next character from the current Link
			if (HAL_UART_Transmit (&huart1, (uint8_t *) pList->byteptr, (uint16_t) 1, (uint32_t) 1) != HAL_OK) return UART_TX_FAILED;
		}
		//here if a character was sent from current link
		if( --(pList->bytecount) >0 ) {	//current Link is not finished sending
			pList->byteptr++;
			return UART_TX_OK;
		}
		else { //current Link is exhausted
			if( pList->listheadp == pList->listtailp ) { // current Link was last on List
				pList->listtailp->next = NULL;	//free Link for reuse
				pList->listheadp = NULL;		//mark List as empty
				pList->listtailp = NULL;		// ..
				return UART_LIST_IS_EMPTY;
			}
			else { //List has more Links to send
				ptemp = pList->listheadp->next;				//next Link from List
				pList->listheadp->next = NULL;				//free current Link for reuse
				pList->listheadp = ptemp;					//new current Link
				pList->byteptr = pList->listheadp->addr;
				pList->bytecount = pList->listheadp->count;
				return UART_LIST_WAS_UPDATED;
			}
		}
	}
	return 	UART_LIST_DEFAULT_RETURN;	/* should never occur */
}

int snprintf_status;
//mpaland: int vsnprintf_(char* buffer, size_t count, const char* format, va_list va);

int msgPost ( msgList_type *pList, msgLink_type *pLink, char *bufp, size_t bufsize, const char *fmt, ... )
//IMPORTANT: msgPost() needs to be protected against interrupt by USART_TX in order to avoid breaking things.
//initializes msLinks, adds them to some msgList. 
//return status codes like LINK_IS_BUSY, etc.
{	
	int strlen= 0;
	//check if link is available
	if( pLink->next != NULL ) return LINK_IS_BUSY;	//'BUSY' means this Link is already in a List; changing it will break that List 
	else {	//compose message from msgPost() arguments, install it in a Link, then append the Link to message List  
		va_list args;	// see https://linuxjedi.co.uk/nested-variadic-functions-in-c/
		va_start(args, fmt);
		strlen = vsnprintf_( bufp, bufsize, fmt, args  );
		va_end(args);
		// mpaland_printf.h says about snprintf(): 
		 // * \return The number of characters that COULD have been written into the buffer, not counting the terminating
		 // *         null character. A value equal or larger than count indicates truncation. Only when the returned value
		 // *         is non-negative and less than count, the string has been completely written.
		// 
		if(strlen  <= 0 ) return LINK_SNPRINTF_FAILED; //some failure occurred -- snprintf_status has the failure code
		if(strlen  >= bufsize ) return LINK_MSG_TOO_LONG;  //message has been truncated (won't send it)
		// here if message generation was successful and message string fits buffer;
		//populate Link with message descriptors
		pLink->addr = bufp; 
		pLink->count = strlen;
		//add Link to List
		if ( pList->listtailp == NULL ) { //there are no messages on the List
			pList->listheadp = pList->listtailp = pLink;	// so make this Link first and last
			pList->byteptr = pList->listheadp->addr;		// and initialize List's content playback parameters
			pList->bytecount = pList->listheadp->count;		//  ..
		}
		else { //add the new link the end of the list
			pList->listtailp->next = pLink; //link the new link to the current end of the list
			pList->listtailp = pLink; 		//and append to the List
		}
		//mark the new link as the List-end Link
		pLink->next = pLink;
	}
	//here with link installed
	// TODO if this Link is being installed at the origin of the List then the List's
	return LINK_IS_INSTALLED;
}



//Some data for initial testing
char msg1[]= "Hello from msg1\n\r";
char msg2[]= "Hello from msg2\n\r";
char msg3[]= "Hello from msg3\n\r";
//ready-made list and list anchor for initial testing msgSend()
msgLink_type Link3 = { &Link3,msg3,strlen("Hello from msg3\n\r"),0};
msgLink_type Link2 = { &Link3,msg2,strlen("Hello from msg2\n\r"),0};
msgLink_type Link1 = { &Link2,msg1,strlen("Hello from msg1\n\r"),0};
msgList_type msgList = { &Link1, &Link3, msg1, strlen("Hello from msg1\n\r")};


int test_msgSend()
{
	int retval;
//	while( (retval=msgSend(&msgList)) >= 0);
	if( (retval=msgSend(&msgList)) < 0)  {	//stupid way to provide breakpoint targets for debugging
//		for(delaycount=0;delaycount<50000;delaycount++);
		return retval;
	}
	else {
//		for(delaycount=0;delaycount<50000;delaycount++);

		return retval;
	}
}

msgLink_type Link4 = {NULL,NULL,0,0};
char msgbuf4[100];

int test_msgPost()
{
	return msgPost( &msgList, &Link4, msgbuf4,100, "Hello from msg%d\n\r", 4 );
}

msgLink_type Link5 = {NULL,NULL,0,0};
char msgbuf5[100];

int test2_msgPost()
{
	return msgPost( &msgList, &Link5, msgbuf5, 100, "Hello from msg5: int %d, char %c, string %s, hex %04X \n\r", 1234, 'A', "My string", 0x123 );
}



int snprintf_testcall() {
	int length;
	length = snprintf(msgbuf4,100,"msg5: int %x,char %c, string %s, hex %04X \n\r", 1234, 'A', "My string", 0x123);
	return length;
}
