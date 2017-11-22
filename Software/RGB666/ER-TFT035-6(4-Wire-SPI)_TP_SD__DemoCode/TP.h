

/* touch panel interface define */
sbit DCLK	   =    P2^0;
sbit TPCS        =    P2^1;
sbit DIN       =    P2^2;	
sbit BUSY	   =	P2^3;
sbit DOUT      =    P2^4;
sbit IRQ       =    P2^5;   //检测触摸屏响应信号

//定义按键端口
sbit KEY = P2^7;

unsigned int TP_X,TP_Y;	   //当前触控坐标


//**********************************************************
void spistar()                                     //SPI开始
{
TPCS=1;
DCLK=1;
DIN=1;
DCLK=1;
}
//**********************************************************
void WriteCharTo7843(unsigned char num)          //SPI写数据
{
unsigned char count=0;
DCLK=0;
for(count=0;count<8;count++)
{
num<<=1;
DIN=CY;
DCLK=0; _nop_();_nop_();_nop_();                //上升沿有效
DCLK=1; _nop_();_nop_();_nop_();
}
}
//**********************************************************
unsigned int ReadFromCharFrom7843()             //SPI 读数据
{
unsigned char count=0;
unsigned int Num=0;
for(count=0;count<12;count++)
{
Num<<=1;
DCLK=1; _nop_();_nop_();_nop_();                //下降沿有效
DCLK=0; _nop_();_nop_();_nop_();
if(DOUT) Num++;
}
return(Num);
}



void inttostr(int dd,unsigned char *str)
{
	str[0]=dd/10000+48;
	str[1]=(dd/1000)-((dd/10000)*10)+48;
	str[2]=(dd/100)-((dd/1000)*10)+48;
	str[3]=(dd/10)-((dd/100)*10)+48;
	str[4]=dd-((dd/10)*10)+48;
	str[5]=0;
}
void AD7843(void)              //外部中断0 用来接受键盘发来的数据
{
 TPCS=0;
delayms(1);                     //中断后延时以消除抖动，使得采样数据更准确
//while(BUSY);                //如果BUSY信号不好使可以删除不用
delayms(1);
WriteCharTo7843(0x90);        //送控制字 10010000 即用差分方式读X坐标 详细请见有关资料
//while(BUSY);               //如果BUSY信号不好使可以删除不用
delayms(1);
DCLK=1; _nop_();_nop_();_nop_();_nop_();
DCLK=0; _nop_();_nop_();_nop_();_nop_();
TP_Y=ReadFromCharFrom7843();
WriteCharTo7843(0xD0);       //送控制字 11010000 即用差分方式读Y坐标 详细请见有关资料
DCLK=1; _nop_();_nop_();_nop_();_nop_();
DCLK=0; _nop_();_nop_();_nop_();_nop_();
TP_X=ReadFromCharFrom7843();
TPCS=1;
}



//在指定位置显示一个字符(8*12大小)
//dcolor为内容颜色，gbcolor为背静颜色
void showzifu(unsigned int x,unsigned int y,unsigned char value,unsigned int dcolor,unsigned int bgcolor)	
{  
	unsigned char i,j;
	unsigned char *temp=zifu;    
  	 LCD_SetPos(x,x+7,y,y+11);  
	temp+=(value-32)*12;
	for(j=0;j<12;j++)
	{   
		for(i=0;i<8;i++)
		{ 		     
		 	if((*temp&(1<<(7-i)))!=0)
			{
				Write_Data(dcolor>>8,dcolor);
			} 
			else
			{
				Write_Data(bgcolor>>8,bgcolor);
			}   
		}
		y+=1;
		temp++;
	 }
}
//在指定位置显示一个字符串(8*12大小)
//dcolor为内容颜色，gbcolor为背静颜色
void showzifustr(unsigned int x,unsigned int y,unsigned char *str,unsigned int dcolor,unsigned int bgcolor)	  
{  
	unsigned int x1,y1;
	x1=x;
	y1=y;
	while(*str!='\0')
	{	
		showzifu(x1,y1,*str,dcolor,bgcolor);
		x1+=7;
		str++;
	}	
}



void TPTEST(void)
{
	unsigned char ss[6];	
	unsigned int lx,ly,k,h;
	spistar();  //模拟spi初始化
	CS =0;  //打开LCD片选使能	
 	ClearScreen(0xffff);	//清屏					
    showzifustr(30,5,"HELLOW!PLEASE TOUCH ME!",0,0xffff);	//显示字符串 
    showzifustr(80,18,"TP TEST!",0,0xffff);	//显示字符串 
	while(KEY)
	{
		if (IRQ==0)
		{  	
			AD7843();
			inttostr(TP_X,ss);
			showzifustr(10,205,"X:",0xf800,0xffff);
			showzifustr(25,205,ss,0xf800,0xffff);	//显示字符串 
			inttostr(TP_Y,ss);
			showzifustr(80,205,"Y:",0xf800,0xffff);
			showzifustr(95,205,ss,0xf800,0xffff);	//显示字符串 
			lx=((TP_X-220)/11);
			ly=((TP_Y-150)/8);

			for(k=0;k<2;k++)
			{
			    for(h=0;h<2;h++)
				{
					 LCD_SetPos(lx+h,lx+1,ly+k,ly+1);
			         Write_Data(0xf1,0x00);
				 }
			}
 
		}

    }



}













