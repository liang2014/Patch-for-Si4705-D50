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

void main() {
	while(1) {
		//AfValid will be set once the AF List is ready.
		//CurBandRSSI is the RSSI value of current station
		//here you also can use SNR instead of RSSI
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