/***************************************************************************************
                  Silicon Laboratories Broadcast Si47XX Example Code

   EVALUATION AND USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS AND CONDITIONS OF
     THE SOFTWARE LICENSE AGREEMENT IN THE DOCUMENTATION FILE CORRESPONDING
     TO THIS SOURCE FILE.
   IF YOU DO NOT AGREE TO THE LIMITED LICENSE AND CONDITIONS OF SUCH AGREEMENT,
     PLEASE RETURN ALL SOURCE FILES TO SILICON LABORATORIES.

  (C) Copyright 2014, Silicon Laboratories, Inc. All rights reserved.

****************************************************************************************/
/**************************************************************
This function translate the MJD calendar to the Gregorian calendar
THE reference date is 2008-01-01 whose MJD date is 54466
mjd:mjd date relative to 2008-1-1
sign: the date before 2008 or behind 2008
pcal:pointer to Calendar struct which save the gregorian calendar
******************************************************************/
extern const u8 code T_MonthDay[13];
void TransMJD(u16 mjd,CalendarStru * pcal)
{
	u8 leap;
	u8 date;
	
	if(!(mjd&0x8000))//behind 2008 year
	{
		pcal->Year = 2008;
		pcal->Week = (mjd+2)%7;
		if(!(pcal->Week))
			pcal->Week = 7;
		//caculate the Year	
		leap = 1;
		while( mjd >= leap+365)
		{	
			mjd -= (leap+365);
			pcal->Year++;
			if(!(pcal->Year%4)&&(pcal->Year%100)||!(pcal->Year%400))
				leap = 1;
			else
				leap = 0;
		}
	}
	else //before 2008 year
	{
		mjd = (~mjd) + 1;
		
		pcal->Week = (9-mjd%7)%7;
		if(!(pcal->Week))
			pcal->Week = 7;
		leap = 0;
		pcal->Year = 2007;
		while( mjd >= leap+365)
		{	
			mjd -= (leap+365);
			pcal->Year--;
			if(!(pcal->Year%4)&&(pcal->Year%100)||!(pcal->Year%400))
				leap = 1;
			else
				leap = 0;
		}
		mjd = leap+365-mjd;
	}
	//caculate the month
	for(pcal->Mon = 1;pcal->Mon < 12;pcal->Mon++)
	{
		if(pcal->Mon == 2)
			date = T_MonthDay[pcal->Mon]+leap;
		else
			date = T_MonthDay[pcal->Mon];
		if(mjd < date)
			break;
		mjd -= date;
	}
	pcal->Date = mjd+1;
}
/******************************************************************************
void  RDS_Update_Alt_Freq(u16 current_alt_freq)
extract the alternative Frequence from the RDS group data tpye 0x0a.
there are two motheds to transmit the message in group type 0a:
method a:
	every group have two alternate frequence or count,the format is:
	count,af0
	af1,  af2
	af3,  af4
	..    ..
	..    ..
method b:
	the group have the pair of tuning frequence and alternate frequence
	the format is:
	count,tune frequence
	tf,af;af>tf,programme same
	af,tf;tf<af,variant of region
both ascending and descending afs are saved ,but only the afs which have same PI code or hte 
second element of Pi code is either of I, N, S are available ; 
******************************************************************************/
#define AF_COUNT_MIN 224
#define AF_COUNT_MAX (AF_COUNT_MIN + 25)
#define AF_FREQ_MIN 0
#define AF_FREQ_MAX 204
#define AF_FREQ_TO_U16F(freq) (8750+((freq-AF_FREQ_MIN)*10))
static void  RDS_Update_Alt_Freq(u16 current_alt_freq)
{
	u8 dat;
    u16 freq;
    // the top 8 bits is either the AF Count or AF Data
    dat = (u8)(current_alt_freq >> 8);
    // look for the AF Count indicator
    if ((dat > AF_COUNT_MIN) && (dat <= AF_COUNT_MAX))
    {
    	// init_alt_freq();  // clear the alternalte frequency list
       	AF.afCount = (dat - AF_COUNT_MIN); // set the count
        dat = (u8)current_alt_freq;
        freq = AF_FREQ_TO_U16F(dat);//this maybe the current tune frequence
	if(freq <= FmSeekTop && freq >= FmSeekBottem) {
		AF.afList[0] = freq;
       	 	AF.afIndex = 1;
       		AfValid = 1;
        	Lcd_RDS_AF();
	}
    }
    // look for the AF Data, because the "regional off ",so all method A and method b which don't care ascending or 
    //descending afs will be save ,and the af will be available if the pi code is same or the second element is either of I, S, N 
    else if (dat && dat <= AF_FREQ_MAX)//the frequence range is 1~AF_FREQ_MAX
    {
    	freq = AF_FREQ_TO_U16F(dat);
	if(freq <= FmSeekTop && freq >= FmSeekBottem) {
    		if(AF.afIndex < AF.afCount)//because the max AF.afCount is 25,so the array will not be overlap 
    		{
    			if(freq != AF.afList[0])
    	   		{
    	   			AF.afList[AF.afIndex] = freq ;
    	   			AF.afIndex++;
    	   		}
    		}
	}
    	dat = (u8)current_alt_freq;
    	freq = AF_FREQ_TO_U16F(dat);
	if(freq <= FmSeekTop && freq >= FmSeekBottem) {
    		if(AF.afIndex < AF.afCount)//because the max AF.afCount is 25,so the array will not be overlap 
    		{
    			if(freq != AF.afList[0])
    	   		{
    	   			AF.afList[AF.afIndex] = freq ;
    	   			AF.afIndex++;
    	   		}
    		}
	}
    }
}
/*************************************************************
u8  RSD_QuicklyCheckAf(u16 Af)
get the rssi of af as soon as possible
if the RSQ of the turned frequence  have been worse,then test 
the af to find a better rsq
*************************************************************/
u8  RSD_QuicklyCheckAf(u16 Af)
{
	u8 rsp[8];
	u8 rssi;

	si47xx_TuneFreq(FM_WORKMODE,Af);
    Si47xx_Wait_STC();
    si47xx_TuneStatus(FM_WORKMODE,0, 1,rsp);
   	rssi = rsp[4];
   	
	si47xx_TuneFreq(FM_WORKMODE,FmBand);
    Si47xx_Wait_STC();//while (!(si47xx_getIntStatus() & STCINT));
	return rssi;
}
void RDS_ProcessRDS()
{
		u8 group_type;  // bits 4:1 = type,  bit 0 = version
    	u8 addr;
    	u16 mjd;
    	//u8 abflag;
    	// Update pi code.
    	if((errorFlags & RDS_BLOCKA_ERRORMAP) < RDS_BLOCKA_ERRORMAP)
        	RDS_Update_PI (BlockA);
        if((errorFlags & RDS_BLOCKB_ERRORMAP) < (RDS_BLOCKB_ERRORMAP-0x10))
        {
        	group_type = BlockB>>11;  // upper five bits are the group type and version
    		//check the group version
    		if (group_type & 0x01)
    		{
    			if((errorFlags & RDS_BLOCKC_ERRORMAP) < RDS_BLOCKC_ERRORMAP)
        			RDS_Update_PI(BlockC);// Update pi code.  Version B formats always have the pi code in words A and C
    		}
	  		RDS_Update_PTY((BlockB>>5) & 0x1f);// update pty code.
	    	switch (group_type) 
	    	{
	        	case RDS_TYPE_0A:
	        		if((errorFlags & RDS_BLOCKC_ERRORMAP) < RDS_BLOCKC_ERRORMAP)
	        			RDS_Update_Alt_Freq(BlockC);
	            	// fallthrough,because all of group type 0a and group type 0b have 
	            	//the ps information in block d 
	        	case RDS_TYPE_0B:
	        		if((errorFlags & RDS_BLOCKD_ERRORMAP) < RDS_BLOCKD_ERRORMAP)
	            	{
	            		addr = (BlockB&0x3)*2;//BlockB0~1 bit is the order of ps,and two character in blockd of a group
	                	RDS_Update_PS(addr, (u8)(BlockD>>8));
	                	RDS_Update_PS(addr+1, (u8)BlockD);
	                }
	            	break;
	        	case RDS_TYPE_2A://Radio text 64 characters,block c and d a radio text
	            		addr = (BlockB & 0xf) * 4;
	            		//abflag = (BlockB & 0x0010) >> 4;
	            		if((errorFlags & RDS_BLOCKC_ERRORMAP) < RDS_BLOCKC_ERRORMAP)
	            		{
	            			RDS_Update_RT(addr,(u8)(BlockC>>8));
	            			RDS_Update_RT(addr+1,(u8)(BlockC));
	            		}
						addr += 2;
						if((errorFlags & RDS_BLOCKD_ERRORMAP) < RDS_BLOCKD_ERRORMAP)
	            		{
	            			RDS_Update_RT(addr,(u8)(BlockD>>8));
	            			RDS_Update_RT(addr+1,(u8)(BlockD));
	            		}
	            		break;
	        	case RDS_TYPE_2B://Radio text 32 characters,only block d is radio text
	        			if((errorFlags & RDS_BLOCKD_ERRORMAP) < RDS_BLOCKD_ERRORMAP)
	        	    	{
	        	    		addr = (BlockB & 0xf) * 2;
	        	    		//abflag = (BlockB & 0x0010) >> 4;
	        	    		RDS_Update_RT(addr,(u8)(BlockD>>8));
	            			RDS_Update_RT(addr+1,(u8)(BlockD));
	        	    	}
	        	    	break;
	        	case RDS_TYPE_4A:// Clock Time and Date
	        	    	mjd = (BlockB<<15)+(BlockC>>1);
	        	    	if(BlockB&0x02)//highest bit of mjd
	        	    		mjd	+= (0xffff-RDS_MJD_BASE+1);
	        	    	else
	        	    		mjd	-= RDS_MJD_BASE;
	        	    	//caculate the utc and local time offset
	        	    	RDSCal.Hour = ((BlockD>>12)+(BlockC<<4))&0x1f;
	        	 		RDSCal.Minute = (BlockD>>6)&0x3f;
	        	 		if(!(BlockD & 0x20)) //check the local offset sign
	        	 		{
	        	 			if(BlockD&0x01)
	        	 			{
	        	 				if(RDSCal.Minute>=30)
	        	 				{
	        	 					RDSCal.Minute-= 30;
									RDSCal.Hour++;
								}else	        	 						
	        	 					RDSCal.Minute += 30;
	        	 			}
	        	 			RDSCal.Hour += 	(BlockD>>1)	& 0x0f;
	        	 			if(RDSCal.Hour>=24)
	        	 			{
	        	 				RDSCal.Hour-=24;
	        	 				mjd++;
	        	 			}
	        	 		}else{
	        	 			if(BlockD&0x01)
	        	 			{
	        	 				if(RDSCal.Minute>=30)
	        	 					RDSCal.Minute -= 30;
	        	 				else
	        	 				{
	        	 					RDSCal.Minute+= 30;
									RDSCal.Hour--;
								}	        	 						
	        	 					
	        	 			}
	        	 			if(RDSCal.Hour >((BlockD>>1)& 0x0f))
	        	 				RDSCal.Hour-=((BlockD>>1)& 0x0f);
	        	 			else
	        	 			{
	        	 				RDSCal.Hour+=(24-((BlockD>>1)& 0x0f));
	        	 				mjd--;
	        	 			}
	        	 		}
	        	 		TransMJD(mjd,&RDSCal);
	        	 		CTReceived = 1;
//	        	 		Year = RDSCal.Year;
//	        	 		Month = RDSCal.Mon;
//	        	 		Week = RDSCal.Week;
//        	 			Day  = RDSCal.Date;
//	        	 		Hour  = RDSCal.Hour;
//	        	 		Minute = RDSCal.Minute;
	        	 		Lcd_Disp_CT();
	        	    	break;
	        	default:
	        	    break;
	    }
	}
}
void main() {
	while(1) {
		//AfValid will be set once the AF List is ready.
		//CurBandRSSI is the RSSI value of current station
		//here you also can use SNR instead of RSSI
		RDS_ProcessRDS();
		if(AfValid)	{
			if((CurBandRSSI <  FmSeekRSSI)){
					//low than threshold,check another alternate frequence	
				if(RSD_QuicklyCheckAf(AF.afList[AfIndex]) > FmSeekRSSI){
					FmBand = AF.afList[AfIndex];
					si47xxFMRX_tune(FmBand);
					Lcd_DispSignal();
					Lcd_DispBand();
				}
				AfIndex++;
				if(AfIndex == AF.afIndex){
					AfIndex = 0;
				}	
			}
		}
	}