#include <lpc21xx.h>
#include <stdio.h>
#include <stdlib.h>
#include <rtl.h>
//basetimevalue=5ms 
// for 1ms  count(PR)=(pclk*basetimevalue)-1    
//			 =(12MHz *0.001 sec)-1
//			= 11999
#define DESIRED_COUNT	5000			// for 5sec
#define PRESCALER	11999
void InitTimer0(void);       //Timer Initialization
void checkdelivery(void);    //To check if order is delivered within time
void serial(void);          //Initialize UART
void nextline(void);        //For nextline in uart
void disp(unsigned int);    //Displaying selected dish
void command(unsigned int);  //Commands for lcd
void buzz(void);         //Buzzer when food is delivered
void data(unsigned int);  //sending data to lcd
void delay(unsigned int);
void __swi(8)  Cancel(void);
void __swi(9)  Discount(void);

OS_TID t1,t2,t3,t4,t5;
unsigned char mg;
unsigned int sum=0,sum1=0;   //To calculate total bill
int num[10];
unsigned char dish1[]={"0.Roti  Rs.40\r\n"};   //Menu
unsigned char dish2[]={"1.Chapati  Rs.40\r\n"};
unsigned char dish3[]={"2.Gravy  Rs.120\r\n"};
unsigned char dish4[]={"3.Rice  Rs.200\r\n"};
unsigned char dish5[]={"4.Biryani Rs.350\r\n"};
unsigned char dish6[]={"5.IceCream Rs.75\r\n"};
unsigned char dish7[]={"6.Cake  Rs.45\r\n"};
unsigned char dish8[]={"7.Rolls  Rs.35\r\n"};
unsigned char dish9[]={"8.Puffs  Rs.20\r\n"};
unsigned char dish10[]={"9.Salad  Rs.150\r\n"};
unsigned char dish11[]={"* Ordering Completed\r\n"};
unsigned char dish12[]={"# Cancel order\r\n"};
unsigned char cancel[]={"Order cancelled\r\n"};
unsigned int c[]={0x30,0x30,0x20,0x20,0x28,0x01,0x06,0x0e,0x80};  //LCD commands
int k=0,p;
unsigned char value;
unsigned int i,j,temp=0;
unsigned char message[]={"Select Menu-\r\n"};
unsigned char message1[]={"Total Bill--\r\n"};
unsigned char message3[]={"Wishlist\r\n"};
unsigned char message2[]={"ON-Got order\r\n"};
unsigned char msg[]={"In 15 min\r\n"};
unsigned char next[]={"\r\n"};
unsigned char rec[]={"Press 1 if order received within 1 second"};
unsigned char message4[]={"Order received"};
unsigned char message5[]={"Order not received"};


/*--------------------------------------
Mail box
----------------------------------------*/
os_mbx_declare (MsgBox, 100);                /* Declare an RTX mailbox  100 msgs with name MsgBox*/    
_declare_box(mpool,20,32);	 /* Reserve a memory for 32 blocks of 20 bytes  */


void data(unsigned int x);
__task void initialize(void);
__task void displayMenu(void);
__task void selectMenu(void);
__task void billing(void);
__task void checkDelivery(void);


int main()
{
	PINSEL0=0x00000000;
	PINSEL1=0x00000001;
	PINSEL2=0x00ff0000;
	IO0DIR=0x000040fc;   //p0.2 to p0.7 output lines for lcd and p0.14 for buzzer
	IO1DIR   = 0XFFF0FFFF;	//set rows as output and colomn as input
	_init_box (mpool, sizeof(mpool), sizeof(U32));
	os_sys_init(initialize);
	while(1);
}


/*------------------------------------------------
Initializing hardware
------------------------------------------------*/
__task void initialize(void)
{
	serial();
	t2=os_tsk_self();
	for(i=0;i<9;i++)  //Setting lcd
	{
		command(c[i]);
		delay(2000);
	}
	command(0x80);
	delay(2000);
	t1=os_tsk_create(displayMenu,1);
	t3=os_tsk_create(selectMenu,1);
	os_tsk_delete(t2);
	
}


/*------------------------------------------------
Initialize uart
------------------------------------------------*/
void serial()
{
  PINSEL0 = 0x00000005; 
  IODIR1= 0x00ff0000;	
  U0LCR = 0x83; 		
  U0DLL = 0x61; 		
  U0LCR = 0x03; 		
}


/*------------------------------------------------
Initialize Timer
------------------------------------------------*/
void InitTimer0(void)
{
	T0PR=PRESCALER;
	T0MR0=DESIRED_COUNT;	//interrupt every 5 sec for interval = 5000
	T0MCR=3;		//interrupt and reset when counter=match
	T0TCR=2;		 //	reset timer 
}


/*------------------------------------------------
Commands for lcd
------------------------------------------------*/
void command(unsigned int value) //Command function for lcd
{
unsigned int y;
y=value;
y=y & 0xf0;
IOCLR0=0x000000fc;
IOCLR0=0X00000004;
IOSET0=y;

IOSET0=0x00000008;
delay(10);
IOCLR0=0x00000008;

y=value;
y=y & 0x0f;
y=y<<4;
IOCLR0=0x000000fc;
IOCLR0=0X00000004;
IOSET0=y; 

IOSET0=0x00000008;
delay(10);
IOCLR0=0x00000008;

}


/*------------------------------------------------
SWI interrupts
8--> cancelling the order
9--> receiving 10% discount
------------------------------------------------*/
 void __SWI_8(void) 
{
	for(j=0;cancel[j]!='\0';j++)
	{
		while(!(U0LSR & 0x20));		
      U0THR = cancel[j];
			delay(150);
	}
	os_tsk_delete(t1);
	os_tsk_delete(t3);
}

 void __SWI_9(void) 
{
	U32 *rptr;
	char dis[]={"Hurray you got 10% discount \r\n Your bill is: "};
	int newb=0;
	os_mbx_wait (MsgBox, (void**)&rptr, 0xffff);      /* Wait for the message to arrive. */
  temp= rptr[0];
	for(j=0;dis[j]!='\0';j++)
	{
		while(!(U0LSR & 0x20));		
      U0THR = dis[j];
			delay(150);
	}
	newb =temp * 0.1;
	newb = temp-newb;
	k=0;
	while(newb>0)
{
  num[k]=newb%10;
  newb=newb/10;
  k++;
}
k--;
command(0x80);
delay(2000);
for (p=k;p>=0;p--)
{
			data(num[p]+48);
	    delay(20000);
	    while(!(U0LSR & 0x20));		
			U0THR = num[p]+48;
			delay(15000);
}
}

/*------------------------------------------------
Delay
------------------------------------------------*/
void delay(unsigned x)
{
	for(i=0;i<x;i++);
}


/*------------------------------------------------
Display Menu
------------------------------------------------*/
__task void displayMenu(void)
{
	while(1)
	{
		for(j=0;j<12;j++)   //Displaying menu on lcd and uart
		{
			data(message[j]);
			delay(200000);
			while(!(U0LSR & 0x20));		
      U0THR = message[j];
		}
		delay(200000);
		command(0x01);
		delay(2000);
		command(0x80);
		delay(2000);
		for(j=0;dish1[j]!='\0';j++)
		{
			data(dish1[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish1[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish2[j]!='\0';j++)
		{
			data(dish2[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish2[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish3[j]!='\0';j++)
		{
			data(dish3[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish3[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish4[j]!='\0';j++)
		{
			data(dish4[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish4[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish5[j]!='\0';j++)
		{
			data(dish5[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish5[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish6[j]!='\0';j++)
		{
			data(dish6[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish6[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish7[j]!='\0';j++)
		{
			data(dish7[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish7[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish8[j]!='\0';j++)
		{
			data(dish8[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish8[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish9[j]!='\0';j++)
		{
			data(dish9[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish9[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish10[j]!='\0';j++)
		{
			data(dish10[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish10[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish11[j]!='\0';j++)
		{
			data(dish11[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish11[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		for(j=0;dish12[j]!='\0';j++)
		{
			data(dish12[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
      U0THR = dish12[j];
			delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
		os_tsk_prio(t3,2);
	}
}


/*------------------------------------------------
Display lcd
------------------------------------------------*/
void data(unsigned int dat)  //data function for lcd
{
unsigned int y;
y=dat;
y=y & 0xf0;
IOCLR0=0x000000fc;
   IOSET0=0X00000004;
IOSET0=y; 
 
IOSET0=0x00000008;
delay(10);
IOCLR0=0x00000008;

y=dat;
y=y & 0x0f;
y=y<<4;
 IOCLR0=0x000000fc;	
  IOSET0=0X00000004;
IOSET0=y;

IOSET0=0x00000008;
delay(10);
IOCLR0=0x00000008;
}

/*------------------------------------------------
Select menu
------------------------------------------------*/
__task void selectMenu(void)
{
	while(1)
	{
		delay(20000);
		while(!(U0LSR & 0x01));
		value= U0RBR;
		while(!(U0LSR & 0x20));		
    U0THR =value;
		nextline();
		delay(20000);
		if(value=='*')
		{
      t4=os_tsk_create(billing,3);			
			break;
		}
    if(value=='#')
		{
			Cancel();
		}			
		disp(value-48);
	}
}


/*------------------------------------------------
Displaying menu selected and finding total bill
------------------------------------------------*/
void disp(unsigned int x)  //disp function to display selected menu
{
   unsigned int j;
	 command(0x80);
	 delay(20000);
	 if(x==0)
	 {
		 sum=sum+40;
		 for(j=0;dish1[j]!='\0';j++)
		 {
				data(dish1[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish1[j];
				delay(150);
		 }
	 }
	 else if(x==1)
	 {
		 sum=sum+40;
	    for(j=0;dish2[j]!='\0';j++)
		 {
				data(dish2[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish2[j];
				delay(150);
		 }
	 }
	 else if(x==2)
	 {
		 sum=sum+120;
	    for(j=0;dish3[j]!='\0';j++)
		 {
				data(dish3[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish3[j];
				delay(150);
		 }
	 }
	 else if(x==3)
	 {
		 sum=sum+200;
	    for(j=0;dish4[j]!='\0';j++)
		 {
				data(dish4[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish4[j];
				delay(150);
		 }
	 }
	 else if(x==4)
	 {
		 sum=sum+350;
	    for(j=0;dish5[j]!='\0';j++)
		 {
				data(dish5[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish5[j];
				delay(150);
		 }
	 }
	 else if(x==5)
	 {
		 sum=sum+75;
	    for(j=0;dish6[j]!='\0';j++)
		 {
				data(dish6[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish6[j];
				delay(150);
		 }
	 }
	 else if(x==6)
	 {
		 sum=sum+45;
	    for(j=0;dish7[j]!='\0';j++)
		 {
				data(dish7[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish7[j];
				delay(150);
		 }
	 }
	 else if(x==7)
	 {
		 sum=sum+35;
	    for(j=0;dish8[j]!='\0';j++)
		 {
				data(dish8[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish8[j];
				delay(150);
		 }
	 }
	 else if(x==8)
	 {
		 sum=sum+20;
	    for(j=0;dish9[j]!='\0';j++)
		 {
				data(dish9[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish9[j];
				delay(150);
		 }
	 }
	 else if(x==9)
	 {
		 sum=sum+150;
	    for(j=0;dish10[j]!='\0';j++)
		 {
				data(dish10[j]);
				delay(2000);
				while(!(U0LSR & 0x20));		
				U0THR = dish10[j];
				delay(150);
		 }
	 }
		delay(200000);
		command(0x01);
		delay(2000);
}


/*------------------------------------------------
Billing
------------------------------------------------*/
__task void billing(void)
{
	U32 *mptr;
  os_mbx_init (MsgBox, sizeof(MsgBox));
  mptr = _alloc_box (mpool);                
	  command(0x80);
		delay(2000);
		for(j=0;j<14;j++)
		{
			data(message1[j]);
			delay(2000);
			while(!(U0LSR & 0x20));		
							U0THR = message1[j];
							delay(150);
		}
		delay(2000);
		command(0x01);
		delay(2000);
sum1=sum;
while(sum>0)
{
  num[k]=sum%10;
  sum=sum/10;
  k++;
}
k--;
command(0x80);
delay(2000);
for (p=k;p>=0;p--)
{
			data(num[p]+48);
	    delay(20000);
	    while(!(U0LSR & 0x20));		
			U0THR = num[p]+48;
			delay(15000);
}
nextline();
if(sum1>500) //if bill is greater than 500 send bill through mail box for discounting 
{
	mptr[0] = sum1;
  os_mbx_send (MsgBox, mptr, 0xffff);
	Discount();
}
delay(150);
command(0x80);
	i=0;
	delay(20000);
	os_tsk_prio(t3,0);
	os_tsk_prio(t1,0);
	os_tsk_prio_self(0);
  t5=os_tsk_create(checkDelivery,1);
}


/*------------------------------------------------
Next line
------------------------------------------------*/
void nextline(void)
{
	for(i=0;i<3;i++)
	{
		while(!(U0LSR & 0x20));		
      U0THR = next[i];
	}
}

/*------------------------------------------------
Check delivery : to ceck if order is received within 1s timer that is set
------------------------------------------------*/
__task void checkDelivery(void)
{
	unsigned char t;
	delay(1000);
	nextline();
	for(j=0;rec[j]!='\0';j++)
	{
		while(!(U0LSR & 0x20));		
      U0THR = rec[j];
		delay(1000);
	}
	nextline();
  InitTimer0();// initialise timer0
	T0TCR = 0x01;		// start timer 
	while(!( T0IR==0x01))
	{
		t=U0RBR;
		delay(2000);
		if(t== '1')
		{
			for(j=0;message4[j]!='\0';j++)
			{
				while(!(U0LSR & 0x20));		
					U0THR = message4[j];
				delay(1000);
			}
			nextline();
			delay(1000);
			os_tsk_delete(t1);
			os_tsk_delete(t2);
			os_tsk_delete(t3);
			os_tsk_delete(t4);
			os_tsk_delete(t5);
		}
	}
	for(j=0;message5[j]!='\0';j++)
			{
				while(!(U0LSR & 0x20));		
					U0THR = message5[j];
				delay(1000);
			}
			nextline();
			os_tsk_delete(t1);
			os_tsk_delete(t2);
			os_tsk_delete(t3);
			os_tsk_delete(t4);
			os_tsk_delete(t5);
			
}



