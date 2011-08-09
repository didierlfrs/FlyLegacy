/*
 * RadioSubsystems.cpp
 *
 * Part of Fly! Legacy project
 *
 * Copyright 2003-2005  Chris Wallace
 * Copyright 2007       Jean Sabatier
 *
 * Fly! Legacy is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 * Fly! Legacy is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *   along with Fly! Legacy; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "../Include/Subsystems.h"
#include "../Include/Globals.h"
#include "../Include/Database.h"
#include "../Include/Fui.h"
#include "../Include/FuiUser.h"

using namespace std;
//===================================================================================
// CExtSource:   An externnal radio source used to drive the radio
//===================================================================================
void CExtSource::SetSource(CmHead *src,ILS_DATA *ils,U_INT frm)
{	active	= 1;
	qAct		= src->GetActiveQ();					// Store active queue
	strncpy(sidn,src->GetIdent(),5);			// store ident
	strncpy(snam,src->GetName(),64);			// Store name
	spos		= src->GetPosition();					// Geo position
	smag		= src->GetMagDev();						// Magnetic deviation
	signal  = (ils)?(SIGNAL_ILS):(SIGNAL_VOR);	// Station type
	//--- Any ILS to set --------------------------------------
	ilsD		= ils;
	vdev		= 0;
	opoP		= (ils)?(&ils->opoP):(0);
	refD    = (ils)?(ils->lnDIR):(0);
	//--- compute feet factor at given latitude ---------------
	double lr   = TC_RAD_FROM_ARCS(spos.lat);					//DegToRad  
  nmFactor = cos(lr) / 60;                          // 1 nm at latitude lr
	//--- Refresh distance and direction -----------------------
	Refresh(frm);
	return;
}
//--------------------------------------------------------------------------
//	Refresh distance and direction to aircraft
//--------------------------------------------------------------------------
void CExtSource::Refresh(U_INT fram)
{	//----compute WPT relative position -------------
	SPosition *acp  = mveh->GetAdPosition();
	SPosition *ref  = (ilsD)?(&ilsD->refP):(&spos);
  SVector	v	      = GreatCirclePolar(acp, ref);
  radial  = Wrap360((float)v.h - smag);
	nmiles  = (float)v.r * MILE_PER_FOOT;
  dsfeet  =  v.r;
	//--- compute vertical deviation ----------------
	if (0 == ilsD)		return;
	double alr  = ilsD->refP.alt;
	double vH		= dsfeet * ilsD->gTan;
	vdev  = (acp->alt - vH - alr) / dsfeet;
	ilsD->errG  = vdev;			  
	return;
}
//===================================================================================
// CRadio
//===================================================================================
CRadio::CRadio (void)
{ TypeIs (SUBSYSTEM_RADIO);
  hwId    = HW_RADIO;
  sinc    = 0;
  test    = false;
  nState  = 0;
  cState  = 0;
  VOR     = 0;
  ILS     = 0;
  COM     = 0;
  OBS     = 0;
  Radio.iAng = 0;
  Radio.gDEV = 0;
  Radio.hREF = 0;
  Radio.hDEV = 0;
  Radio.mdev = 0;
  Radio.mdis = 0;
  Radio.fdis = 0;
  Radio.aDir = 0;
  memset(&Radio,0,sizeof(BUS_RADIO));
  Radio.ntyp = SIGNAL_OFF;
  Radio.flag = VOR_SECTOR_OF;
  ActCom.freq   = 0;
  ActCom.fract  = 0;
  ActCom.whole  = 0;
}
//------------------------------------------------------------------
//  All parameters are read.  Register master radio (radio #1)
//-------------------------------------------------------------------
void CRadio::ReadFinished()
{ CDependent::ReadFinished();
	EXT.SetVEH(mveh);
  return;
}
//------------------------------------------------------------------
//  Free all stations
//-------------------------------------------------------------------
CRadio::~CRadio()
{ FreeRadios(1);
}
//------------------------------------------------------------------
//  Primary radio answer to datatag 'Radi' unit 1
//-------------------------------------------------------------------
bool CRadio::MsgForMe (SMessage *msg)
{ if (msg) {
    bool matchRadio = (msg->group == 'Radi') & (msg->user.u.unit == 1);
    bool matchGroup = (msg->group == unId) | matchRadio;
    bool hwNull     = (msg->user.u.hw == HW_UNKNOWN);
    bool hwMatch    = (msg->user.u.hw == (unsigned int) hwId);
    bool unitNull   = (msg->user.u.unit == 0);
    bool unitMatch  = (msg->user.u.unit == uNum);
    return matchGroup && (hwNull || hwMatch) && (unitNull || unitMatch);
  }
  return false;
}
//------------------------------------------------------------------
//  Store initial frequency
//-------------------------------------------------------------------
void  CRadio::StoreFreq(short(wp),short(fp),CHFREQ &freq)
{ freq.whole = wp; 
  freq.fracp = fp;
  _snprintf(freq.wText,8,"%3u.",  freq.whole);
  _snprintf(freq.fText,8,"%02u", (freq.fracp / 10));
  return;
}
//--------------------------------------------------------------------
//  Edit Autopilot data
//--------------------------------------------------------------------
void CRadio::Probe(CFuiCanva *cnv)
{ char edt[256];
  sprintf_s(edt,63,"Active:     %d",active);
  cnv->AddText(1,edt,1);
  sprintf_s(edt,63,"Freq:%d=>   %03u.%02u",Radio.rnum,
                                      ActCom.whole,
                                      ActCom.fract);
  cnv->AddText(1,edt,1);

  sprintf_s(edt,63,"%03d",Radio.xOBS);
  cnv->AddText( 1,"xOBS");
  cnv->AddText(10,edt,1);

  sprintf_s(edt,63,"%.05f",Radio.hREF);
  cnv->AddText( 1,"hREF");
  cnv->AddText(10,edt,1);

  sprintf_s(edt,63,"%.05f",Radio.radi);
  cnv->AddText( 1,"Radial");
  cnv->AddText(10,edt,1);

  sprintf_s(edt,63,"%.05f",Radio.hDEV);
  cnv->AddText( 1,"hDEV");
  cnv->AddText(10,edt,1);

  sprintf_s(edt,63,"%.05f",Radio.gDEV);
  cnv->AddText( 1,"gDEV");
  cnv->AddText(10,edt,1);

  sprintf_s(edt,63,"%.05f",Radio.mdis);
  cnv->AddText( 1,"nMile");
  cnv->AddText(10,edt,1);

  sprintf_s(edt,63,"%.05f",Radio.aDir);
  cnv->AddText( 1,"aDir");
  cnv->AddText(10,edt,1);

  sprintf_s(edt,63,"%.05f",Radio.iAng);
  cnv->AddText( 1,"iAng");
  cnv->AddText(10,edt,1);

  sprintf_s(edt,63,"%d",Radio.ntyp);
  cnv->AddText( 1,"Type");
  cnv->AddText(10,edt,1);

  sprintf_s(edt,63,"%d",Radio.flag);
  cnv->AddText( 1,"flag");
  cnv->AddText(10,edt,1);
  return;
}
//---------------------------------------------------------------------------
//  Make a float frequency
//---------------------------------------------------------------------------
void CRadio::MakeFrequency(RADIO_FRQ *loc)
{ float ff = (float)(loc->whole) + ((float)loc->fract / 1000);
  loc->freq = ff;
  return;
}
//--------------------------------------------------------------------------
//  Store a given frequency
//  Note the fractional part is multiplied by a 10 factor for the 25 KHZ increment
//--------------------------------------------------------------------------
void CRadio::StoreFreq(RADIO_FRQ *loc, float fq)
{ U_SHORT wp = int(fq);
  float fp    = ((fq - wp) * 10000) + 1;
  loc->whole  = wp;
  loc->fract  = short(fp / 10);
  loc->freq   = fq;
  return;
}
//--------------------------------------------------------------------------
//  Increment Nav whole part(108-117 MHZ)
//  by Mouse direction 
//--------------------------------------------------------------------------
void CRadio::ModifyWholeNav(short No,RADIO_FRQ *loc,short inc)
{ short nf  = loc->whole + inc;
  if (nf < 108) nf = 117;
  if (nf > 117) nf = 108;
  loc->whole  = nf;
  SetField(navTAB,No,"%03u",nf);
  MakeFrequency(loc);
  return;
}
//--------------------------------------------------------------------------
//  Increment Nav fract part(118-136 MHZ)
//  by Mouse direction and 50Khz
//--------------------------------------------------------------------------
void CRadio::ModifyFractNav(short No,RADIO_FRQ *loc,short inc)
{ short dta = inc * 50;
  short nf  = loc->fract + dta;
  if (nf < 0)     nf = 950;
  if (nf > 990)   nf = 0;
  loc->fract  = nf;
  SetField(navTAB,No,".%02u",nf / 10);
  MakeFrequency(loc);
  return;
}
//--------------------------------------------------------------------------
//  Swap NAV
//--------------------------------------------------------------------------
void CRadio::SwapNav(U_CHAR opt)
{ tempf   = SbyNav;
  SbyNav  = ActNav;
  ActNav  = tempf;
  SetField(navTAB,RADIO_FD_ANAV_WP,"%03u",ActNav.whole);
  SetField(navTAB,RADIO_FD_ANAV_FP,".%02u",ActNav.fract / 10);
  MakeFrequency(&ActNav);
  if (0 == opt)    return;
  SetField(navTAB,RADIO_FD_SNAV_WP,"%03u",SbyNav.whole);
  SetField(navTAB,RADIO_FD_SNAV_FP,".%02u",SbyNav.fract / 10);
  MakeFrequency(&SbyNav);
  return;
}

//--------------------------------------------------------------------------
//  Init com field table
//  -data is the char field
//  -No is the display field number
//--------------------------------------------------------------------------
void CRadio::InitTable(RADIO_FLD *tab,short No,char * data, short cf)
{ tab[No].sPos   = cf;
  tab[No].data   = data;
  tab[No].state  = RAD_ATT_INACT;
  return;
}
//--------------------------------------------------------------------------
//  Inhibit all display fields
//--------------------------------------------------------------------------
void CRadio::InhibitFields(RADIO_FLD *tab,short dim)
{ short inx = 0;
  while (inx < dim)  tab[inx++].state = RAD_ATT_INACT;
  return;
}
//--------------------------------------------------------------------------
//  Inhibit the given numeric field
//--------------------------------------------------------------------------
void CRadio::OffField(RADIO_FLD *tab,short No)
{ tab[No].state = RAD_ATT_INACT;
  return;
}

//--------------------------------------------------------------------------
//  Activate the given numeric field
//--------------------------------------------------------------------------
void CRadio::SetField(RADIO_FLD *tab,short No,char *msk,U_SHORT val)
{ tab[No].state = RAD_ATT_ACTIV;
  if (msk)  sprintf(tab[No].data,msk,val);
  return;
}
//--------------------------------------------------------------------------
//  Activate the given text field
//--------------------------------------------------------------------------
void CRadio::SetField(RADIO_FLD *tab,short No,char *msk,char *txt)
{ tab[No].state = RAD_ATT_ACTIV;
  if (msk) sprintf(tab[No].data,msk,txt);
  return;
}
//--------------------------------------------------------------------------
//  Activate the given char field
//--------------------------------------------------------------------------
void CRadio::SetField(RADIO_FLD *tab,short No,char k)
{ tab[No].state = RAD_ATT_ACTIV;
  tab[No].data[0] = k;
  tab[No].data[1] = 0;
  return;
}
//--------------------------------------------------------------------------
//  Activate the given char field
//--------------------------------------------------------------------------
void CRadio::SetField(RADIO_FLD *tab,short No,char state,char k)
{ tab[No].state = state;
  tab[No].data[0] = k;
  tab[No].data[1] = 0;
  return;
}
//--------------------------------------------------------------------------
//  Activate the given char field
//--------------------------------------------------------------------------
void CRadio::SetField(RADIO_FLD *tab,short No,char state,char *s)
{ tab[No].state = state;
  strcpy(tab[No].data,s);
  return;
}
//--------------------------------------------------------------------------
//  Activate the given text field
//--------------------------------------------------------------------------
void CRadio::RazField(RADIO_FLD *tab,short No)
{ tab[No].state = RAD_ATT_ACTIV;
  tab[No].data  = "";
  return;
}
//--------------------------------------------------------------------------
//  Free navigation systems
//--------------------------------------------------------------------------
void CRadio::FreeRadios(char opt)
{	if (VOR)	{VOR->DecUser(); VOR = 0;}
	if (ILS)	{ILS->DecUser(); ILS = 0;}
	if (0 == opt)							return;
	if (COM)  {COM->DecUser(); COM = 0;}
	return;
}
//--------------------------------------------------------------------------
//  Enter/leave external mode:  ROBOT/GPS interface
//	Synchronize radio with type of source
//--------------------------------------------------------------------------
void CRadio::ModeEXT(CmHead *src,ILS_DATA *ils)
{	//--- Back to normal mode. Synchro radio with nav ------
	if (0 == src)	
	{	EXT.Stop(); 
	  VOR	= globals->dbc->GetTunedNAV(VOR,Frame,ActNav.freq);     // Refresh VOR
    ILS = globals->dbc->GetTunedILS(ILS,Frame,ActNav.freq);     // Refresh ILS
	}
	else
	//--- Update external bus ---------------------------
	{	EXT.SetSource(src,ils,Frame);
		FreeRadios(0);
	}
	Synchronize();
	//TRACE("EXT set %s radi=%.2f hDEV=%.4f", src->GetIdent(), Radio.radi,Radio.hDEV);
	return;
}
//--------------------------------------------------------------------------
//  Change OBS from external
//--------------------------------------------------------------------------
int CRadio::IncXOBS(short inc)
{ short obs  = Radio.xOBS;
  obs += inc;
  if (  0 > obs) obs  = 359;
  if (359 < obs) obs  = 0;
	Radio.SetOBS(obs);
  return obs;
}
//--------------------------------------------------------------------------
//  Change source position
//--------------------------------------------------------------------------
void CRadio::ChangePosition(SPosition *p)
{ if (!EXT.IsActive())			return;
	EXT.SetPosition(p);
	return;
}
//--------------------------------------------------------------------------
//  Change Reference direction from external
//--------------------------------------------------------------------------
void CRadio::ChangeRefDirection(float d)
{	CmHead *sys = Radio.nav;
	if (sys)	sys->SetRefDirection(d);
	Radio.SetOBS(d);
	Synchronize();
//	TRACE("RADIO: Ref dir=%.2f",Radio.hREF);
}
//--------------------------------------------------------------------------
//  Maintain all values
//	Values are updated in a specific structure used for interface
//	with all gauges and subsystems such as autopilot
//--------------------------------------------------------------------------
void CRadio::TimeSlice (float dT,U_INT FrNo)
{ CDependent::TimeSlice(dT,FrNo);
	Frame				= FrNo;
  return Synchronize();
}
//------------------------------------------------------------------
//  Resynchronize radio
//------------------------------------------------------------------
void	CRadio::Synchronize()
{ float   rad = 0;
	CmHead *sys = 0;
	if (EXT.IsActive())	{sys = &EXT;}
	if (VOR)						{sys = VOR;}
	if (ILS)						{sys = ILS;}
	Radio.nav  = sys;
	if (sys)
	{		sys->SetNavOBS(Radio.xOBS);
			rad = sys->GetRadial(); 
      Radio.ntyp = sys->SignalType();							
      Radio.mdis = sys->GetNmiles();
      Radio.mdev = sys->GetMagDev();
      Radio.hREF = sys->GetRefDirection();				//Radio.xOBS;
      Radio.hDEV = ComputeDeviation(Radio.hREF,rad,&Radio.flag,sPower);
      Radio.gDEV = sys->GetVrtDeviation();				//0;
      Radio.radi = rad;
      Radio.fdis = sys->GetFeetDistance();
      Radio.sens = sys->Sensibility();						//10;
    }
	  else 
  {   Radio.hREF = 0;
      Radio.flag = VOR_SECTOR_OF;
      Radio.ntyp = SIGNAL_OFF;
      Radio.gDEV = 0;
      Radio.hDEV = 0;
  }
  //---Compute angle between reference and aircraft heading --
  Radio.aDir = Wrap360(mveh->GetDirection() - Radio.mdev);
  Radio.iAng = Wrap360(Radio.hREF - Radio.aDir);
	return;
}
//--------------------------------------------------------------
//	Set glide deviation in the message
//--------------------------------------------------------------
void CRadio::SetGlide(SMessage *msg)
{	msg->sender       = unId;
  msg->realData	    = 0;
  msg->user.u.unit  = 0;
  if (0 == ILS)     return;  
  msg->user.u.unit  = 1;
  Radio.gDEV        = ILS->GetGlide();
  msg->realData	    = Radio.gDEV;
	return;
}
//--------------------------------------------------------------------------
//  Increment com whole part(118-136 MHZ)
//  by Mouse direction 
//--------------------------------------------------------------------------
void CRadio::ModifyWholeCom(short No,RADIO_FRQ *loc,short inc)
{ short nf  = loc->whole + inc;
  if (nf < 118) nf = 136;
  if (nf > 136) nf = 118;
  loc->whole  = nf;
  SetField(comTAB,No,"%03u",nf);
  MakeFrequency(loc);
  return;
}
//--------------------------------------------------------------------------
//  Increment com fract part(118-136 MHZ)
//  by Mouse direction and 50Khz
//--------------------------------------------------------------------------
void CRadio::ModifyFractCom(short No,RADIO_FRQ *loc,short inc)
{ short inx = inc + 1;
  short nf  = loc->fract + comINC[inx];
  if (nf < 0)     nf = 1000 + comINC[0];
  if (nf >= 1000) nf = 0;
  loc->fract  = nf;
  SetField(comTAB,No,".%02u",nf / 10);
  MakeFrequency(&ActCom);
  return;
}
//--------------------------------------------------------------------------
//  Swap com
//--------------------------------------------------------------------------
void CRadio::SwapCom()
{ tempf   = SbyCom;
  SbyCom  = ActCom;
  ActCom  = tempf;
  SetField(comTAB,K155_ACOM_WP,"%03u",ActCom.whole);
  SetField(comTAB,K155_ACOM_FP,".%02u",ActCom.fract / 10);
  SetField(comTAB,K155_SCOM_WP,"%03u",SbyCom.whole);
  SetField(comTAB,K155_SCOM_FP,".%02u",SbyCom.fract / 10);
  MakeFrequency(&ActCom);
  return;
}
//-------------------------------------------------------------------------
//  Tune Active NAV to frequency
//-------------------------------------------------------------------------
void CRadio::TuneNavTo(float freq,U_CHAR opt)
{ ActNav.freq  = freq;
  SbyNav  = ActNav;
  StoreFreq(&ActNav,freq);
  SetField(navTAB,RADIO_FD_ANAV_WP,"%03u", ActNav.whole);
  SetField(navTAB,RADIO_FD_ANAV_FP,".%02u",ActNav.fract / 10);
  if (0 == opt)   return;
  SetField(navTAB,RADIO_FD_SNAV_WP,"%03u", SbyNav.whole);
  SetField(navTAB,RADIO_FD_SNAV_FP,".%02u",SbyNav.fract / 10);
  return;
}
//-------------------------------------------------------------------------
//  Tune Active COM to frequency
//-------------------------------------------------------------------------
void CRadio::TuneComTo(float freq,U_CHAR opt)
{ ActCom.freq  = freq;
  SbyCom  = ActCom;
  StoreFreq(&ActCom,freq);
  SetField(comTAB,RADIO_FD_ACOM_WP,"%03u", ActCom.whole);
  SetField(comTAB,RADIO_FD_ACOM_FP,".%02u",ActCom.fract / 10);
  if (0 == opt)   return;
  SetField(comTAB,RADIO_FD_SCOM_WP,"%03u", SbyCom.whole);
  SetField(comTAB,RADIO_FD_SCOM_FP,".%02u",SbyCom.fract / 10);
  return;
}
//-------------------------------------------------------------------------
//  Tune Active NAV or COM to frequency
//-------------------------------------------------------------------------
void CRadio::TuneTo(float frq,U_CHAR opt)
{ if (GoodNAVfrequency(frq)) return TuneNavTo(frq,opt);
  if (GoodCOMfrequency(frq)) return TuneComTo(frq,opt);
  return;
}
//-------------------------------------------------------------------
int CRadio::Read (SStream *stream, Tag tag)
{ float f1;
  int rc = TAG_IGNORED;

  switch (tag) {
  case 'tune':
    ReadFloat (&f1, stream);
    rc = TAG_READ;
    break;
  case 'lowf':
    ReadFloat (&f1, stream);
    rc = TAG_READ;
    break;
  case 'high':
    ReadFloat (&f1, stream);
    rc = TAG_READ;
    break;
  case 'seek':
    ReadFloat (&sinc, stream);
    rc = TAG_READ;
    break;
  }

  if (rc != TAG_READ) {
    // See if the tag can be processed by the parent class type
    rc = CDependent::Read (stream, tag);
  }

  return rc;
}
//--------------------------------------------------------------
//  Radio receive a message
//--------------------------------------------------------------
EMessageResult CRadio::ReceiveMessage (SMessage *msg)
{
  EMessageResult rc = MSG_IGNORED;

    switch (msg->id) {
    case MSG_GETDATA:
      switch (msg->user.u.datatag) {
      //--Return external OBS deviation -------
      case 'xDev':
        msg->realData    = Radio.hDEV;
        msg->user.u.unit = Radio.flag;
        return MSG_PROCESSED;
      //----Glide slope ------------------
      case 'glid':
		    SetGlide(msg);
        return MSG_PROCESSED;
       //----Get external OBS ------------------
      case 'obs_':
        msg->intData  = Radio.xOBS;
        return MSG_PROCESSED;
      case 'tune':
        msg->realData = 0;
        return MSG_PROCESSED;
      case 'hfrq':
        msg->realData = 0;
        return MSG_PROCESSED;

      case 'lfrq':
        msg->realData = 0;
        return MSG_PROCESSED;

      case 'sinc':
        msg->realData = sinc;
        return MSG_PROCESSED;

      case 'test':
        msg->realData = test ? 1 : 0;
        return MSG_PROCESSED;

      case 'getr':
        msg->voidData = &Radio;
        return MSG_PROCESSED;

			case 'gets':
        msg->voidData = this;
        return MSG_PROCESSED;

      }
      break;

    case MSG_SETDATA:
      switch (msg->user.u.datatag) {
      //----Set Active frequency -------
      case 'tune':
        TuneTo(msg->realData,1);
        return MSG_PROCESSED;
      case 'hfrq':
        return MSG_PROCESSED;

      case 'lfrq':
        return MSG_PROCESSED;

      case 'sinc':
        sinc = (float)msg->realData;
        return MSG_PROCESSED;

      case 'test':
        test = true;
        return MSG_PROCESSED;
       //----Set external OBS ------------------
      case 'obs_':
        msg->intData  = IncXOBS(short(msg->realData));
        return MSG_PROCESSED;

      }
	}

  return CDependent::ReceiveMessage (msg);

}
//=====================================================================================
//  NAV hit events
//====================================================================================
//-------------------------------------------------------------------------
//  Power OFF
//-------------------------------------------------------------------------
RADIO_HIT Nav_POF[] = {
  {0,0},
};
//-------------------------------------------------------------------------
// Power ON
//-------------------------------------------------------------------------
RADIO_HIT Nav_PON[] = {
  {RADIO_CA01,RADIO_EV_ANAV_WP},                      // CK01: Active Whole digit
  {RADIO_CA02,RADIO_EV_ANAV_FP},                      // CK02: Active fract part
  {RADIO_CA03,RADIO_EV_SNAV_WP},                      // CK03: Standby whole part
  {RADIO_CA04,RADIO_EV_SNAV_FP},                      // CK04: Standby fract part
  {RADIO_CA05,RADIO_EV_TUNE_NV},                      // CK05: Freq tune
  {RADIO_CA06,RADIO_EV_TRSF_NV},                      // CK06: Freq transfer
  {0,0},
};
//------------------------------------------------------------------------------------
//  NAV RADIO HIT TABLE
//  One Set of field table per state
//-------------------------------------------------------------------------------------
RADIO_HIT *CNavRadio::RnavHIT[] = {
  Nav_POF,                                // Power off
  Nav_PON,                                // Power on
};

//=============================================================================
// CNavRadio:  Generic NAV radio
//=============================================================================
CNavRadio::CNavRadio (void)
{ TypeIs (SUBSYSTEM_NAV_RADIO);
  hwId  = HW_RADIO;
  sinc  =   0.050f;
  test  = false;
  mskFS = 0xFF;
  //-----Init field table ----------------------------
  InitTable(navTAB,RADIO_FD_ANAV_WP,nDat1,RADIO_CA01);
  InitTable(navTAB,RADIO_FD_ANAV_FP,nDat2,RADIO_CA02);
  InitTable(navTAB,RADIO_FD_SNAV_WP,nDat3,RADIO_CA03);
  InitTable(navTAB,RADIO_FD_SNAV_FP,nDat4,RADIO_CA04);
  StoreFreq(&ActNav,108.95f);
  StoreFreq(&SbyNav,110.45f);
  MakeFrequency(&ActNav);

}
//--------------------------------------------------------------
//  Free all resources
//--------------------------------------------------------------
CNavRadio::~CNavRadio (void)
{}
//--------------------------------------------------------------------
//  Read parameters
//--------------------------------------------------------------------
int CNavRadio::Read (SStream *stream, Tag tag)
{
  int rc = TAG_IGNORED;

  switch (tag) {
  case 'stby':
    ReadFloat (&stby, stream);
    rc = TAG_READ;
    break;
  }

  if (rc != TAG_READ) {
    // See if the tag can be processed by the parent class type
    rc = CRadio::Read (stream, tag);
  }
  
  return rc;
}
//------------------------------------------------------------------
//  Event dispatcher
//  Event are dispatched only in power state ON
//------------------------------------------------------------------
void CNavRadio::Dispatcher(U_CHAR evn)
{ switch (evn)  {
  case RADIO_EV_ANAV_WP:
    ModifyWholeNav(RADIO_FD_ANAV_WP,&ActNav,mDir);
    return;
  case RADIO_EV_ANAV_FP:
    ModifyFractNav(RADIO_FD_ANAV_FP,&ActNav,mDir);
    return;
  case RADIO_EV_SNAV_WP:
    ModifyWholeNav(RADIO_FD_SNAV_WP,&SbyNav,mDir);
    return;
  case RADIO_EV_SNAV_FP:
    ModifyFractNav(RADIO_FD_SNAV_FP,&SbyNav,mDir);
    return;
  case RADIO_EV_TUNE_NV:
    { if (-1 == mDir) ModifyWholeNav(RADIO_FD_ANAV_WP,&ActNav,+1);
      if (+1 == mDir) ModifyFractNav(RADIO_FD_ANAV_FP,&ActNav,-1);
      return;
    }
  case RADIO_EV_TRSF_NV:
    SwapNav(1);
    return;
  }
  return;
}

//------------------------------------------------------------------
//  Check power status
//------------------------------------------------------------------
void CNavRadio::PowerStatus()
{ //-----Check for power ON ---------------
  if ((0 == nState) && (active))
  { nState = 1;
    SetField(navTAB,RADIO_FD_ANAV_WP,"%03u", ActNav.whole);
    SetField(navTAB,RADIO_FD_ANAV_FP,".%02u",ActNav.fract / 10);
    SetField(navTAB,RADIO_FD_SNAV_WP,"%03u", SbyNav.whole);
    SetField(navTAB,RADIO_FD_SNAV_FP,".%02u",SbyNav.fract / 10);
    return;
  }
  //---- Check for power off ---------------
  if ((1 == nState) && (0 == active))
  { nState = 0;
		FreeRadios(1);
    return;
  }
}
//------------------------------------------------------------------
//  Time slice
//------------------------------------------------------------------
void  CNavRadio::TimeSlice (float dT,U_INT FrNo)
{ CRadio::TimeSlice (dT,FrNo);
  PowerStatus();
  if (0 == nState)  return;
  //----Refresh nav stations ---------------------------------------
  VOR	= globals->dbc->GetTunedNAV(VOR,FrNo,ActNav.freq);     // Refresh VOR
  if (VOR)          return;
  ILS = globals->dbc->GetTunedILS(ILS,FrNo,ActNav.freq);     // Refresh ILS
  return;
}

//------------------------------------------------------------------
//  Receive a message
//------------------------------------------------------------------
EMessageResult CNavRadio::ReceiveMessage (SMessage *msg)
{ switch (msg->id) {
    case MSG_GETDATA:
      switch (msg->user.u.datatag) {
      //-- Send a pointer to this radio -----
      case 'gets':
        msg->voidData = this;
        return MSG_PROCESSED;
      //--- Return active freqency ----------
      case 'tune':
        msg->realData = ActNav.freq;
        return MSG_PROCESSED;

      }
      return CRadio::ReceiveMessage (msg);
    //---------------------------------------
    case MSG_SETDATA:
      switch (msg->user.u.datatag) {
      case 'stby':
        stby = (float)msg->realData;
        return MSG_PROCESSED;
      case 'tune':
        TuneNavTo(msg->realData,1);
        return MSG_PROCESSED;
      }
      return CRadio::ReceiveMessage (msg);
  }
  return CRadio::ReceiveMessage (msg);
}
//================================================================================
// CRnavRadio
//
CRnavRadio::CRnavRadio (void)
{
  TypeIs (SUBSYSTEM_RNAV_RADIO);
}

//=====================================================================================
//  COM hit events
//====================================================================================
//-------------------------------------------------------------------------
//  Power OFF
//-------------------------------------------------------------------------
RADIO_HIT Com_POF[] = {
  {0,0},
};
//-------------------------------------------------------------------------
// Power ON
//-------------------------------------------------------------------------
RADIO_HIT Com_PON[] = {
  {RADIO_CA01,RADIO_EV_ACOM_WP},                      // CK01: Active Whole digit
  {RADIO_CA02,RADIO_EV_ACOM_FP},                      // CK02: Active fract part
  {RADIO_CA03,RADIO_EV_SCOM_WP},                      // CK03: Standby whole part
  {RADIO_CA04,RADIO_EV_SCOM_FP},                      // CK04: Standby fract part
  {RADIO_CA05,RADIO_EV_TUNE_CM},                      // CK05: Freq tune
  {RADIO_CA06,RADIO_EV_TRSF_CM},                      // CK06: Freq transfer
  {0,0},
};
//------------------------------------------------------------------------------------
//  NAV RADIO HIT TABLE
//  One Set of field table per state
//-------------------------------------------------------------------------------------
RADIO_HIT *CComRadio::RcomHIT[] = {
  Com_POF,                                // Power off
  Com_PON,                                // Power on
};

//================================================================================
// CComRadio
//================================================================================
CComRadio::CComRadio (void)
{ TypeIs (SUBSYSTEM_COMM_RADIO);
  hwId  = HW_RADIO;
  sinc  =   0.025f;
  mskFS = 0xFF;
  //-----Init field table ----------------------------
  InitTable(comTAB,RADIO_FD_ACOM_WP,nDat1,RADIO_CA01);    // Active comm whole
  InitTable(comTAB,RADIO_FD_ACOM_FP,nDat2,RADIO_CA02);    // Active comm fractional
  InitTable(comTAB,RADIO_FD_SCOM_WP,nDat3,RADIO_CA03);    // Standby whole
  InitTable(comTAB,RADIO_FD_SCOM_FP,nDat4,RADIO_CA04);    // Standby fractional
  StoreFreq(&ActCom,118.95f);
  StoreFreq(&SbyCom,125.00f);
  MakeFrequency(&ActCom);
  comINC[0] = -50;
  comINC[2] = +50;
}

//--------------------------------------------------------------
//  Free all resources
//--------------------------------------------------------------
CComRadio::~CComRadio (void)
{}
//------------------------------------------------------------------
//  Event dispatcher
//  Event are dispatched only in power state ON
//------------------------------------------------------------------
void CComRadio::Dispatcher(U_CHAR evn)
{ switch (evn)  {
  case RADIO_EV_ACOM_WP:
    ModifyWholeCom(RADIO_FD_ACOM_WP,&ActCom,mDir);
    return;
  case RADIO_EV_ACOM_FP:
    ModifyFractCom(RADIO_FD_ACOM_FP,&ActCom,mDir);
    return;
  case RADIO_EV_SCOM_WP:
    ModifyWholeCom(RADIO_FD_SCOM_WP,&SbyCom,mDir);
    return;
  case RADIO_EV_SCOM_FP:
    ModifyFractCom(RADIO_FD_SCOM_FP,&SbyCom,mDir);
    return;
  case RADIO_EV_TUNE_NV:
    { if (-1 == mDir) ModifyWholeCom(RADIO_FD_ACOM_WP,&ActCom,+1);
      if (+1 == mDir) ModifyFractCom(RADIO_FD_ACOM_FP,&ActCom,-1);
      return;
    }
  case RADIO_EV_TRSF_CM:
    SwapCom();
    return;
  }
  return;
}
//------------------------------------------------------------------
//  Check power status
//------------------------------------------------------------------
void CComRadio::PowerStatus()
{ //-----Check for power ON ---------------
  if ((0 == cState) && (active))
  { cState = 1;
    SetField(comTAB,RADIO_FD_ACOM_WP,"%03u", ActCom.whole);
    SetField(comTAB,RADIO_FD_ACOM_FP,".%02u",ActCom.fract / 10);
    SetField(comTAB,RADIO_FD_SCOM_WP,"%03u", SbyCom.whole);
    SetField(comTAB,RADIO_FD_SCOM_FP,".%02u",SbyCom.fract / 10);
    if (1 == uNum)  mveh->RegisterCOM(unId);
    return;
  }
  //---- Check for power off ---------------
  if ((1 == cState) && (0 == active))
  { cState = 0;
    if (COM)  COM->DecUser();
    if (1 == uNum)  mveh->RegisterCOM(0);
    globals->rdb->TuneTo(0);
    COM = 0;
    return;
  }
}
//------------------------------------------------------------------
//  Receive a message
//------------------------------------------------------------------
EMessageResult CComRadio::ReceiveMessage (SMessage *msg)
{
  EMessageResult rc = MSG_IGNORED;

    switch (msg->id) {
    case MSG_GETDATA:
      switch (msg->user.u.datatag) {
      //--Pointer to this radio -------------
      case 'gets':
        msg->voidData = this;
        return MSG_PROCESSED;
      //--Active frequency ------------------
      case 'tune':
        msg->realData = ActCom.freq;
        return MSG_PROCESSED;
      }
      break;
    //----------------------------------------------------------
    case MSG_SETDATA:
      switch (msg->user.u.datatag) {
      //----Set Standby frequency ----------
      case 'stby':
        stby = (float)msg->realData;
       return MSG_PROCESSED;
      //----Set Active com frequency -------
      case 'tune':
        TuneTo(msg->realData,1);
        return MSG_PROCESSED;
      }
    }

    if (rc == MSG_IGNORED) {
      // See if the message can be processed by the parent class
      rc = CRadio::ReceiveMessage (msg);
    }
  return rc;
}
//------------------------------------------------------------------
//  Time slice
//------------------------------------------------------------------
void  CComRadio::TimeSlice (float dT,U_INT FrNo)
{ CRadio::TimeSlice (dT,FrNo);
  PowerStatus();
  if (0 == cState)  return;
  //----Refresh com stations ---------------------------------------
  COM	= globals->dbc->GetTunedCOM(COM,FrNo,ActCom.freq);     // Refresh COM
  globals->rdb->TuneTo(COM);
  return;
}

//================================================================================
// CHFCommRadio
//
CHFCommRadio::CHFCommRadio (void)
{
  TypeIs (SUBSYSTEM_HF_COMM_RADIO);
}

//=========================================================================
//  CTimer for subsytems
//=========================================================================
//------------------------------------------------------------------------
//  Init
//------------------------------------------------------------------------
CTimer::CTimer(void)
{
}
//------------------------------------------------------------------------
//  Update timer
//------------------------------------------------------------------------
void  CTimer::Update(float dT)
{ if (0 == state)   return;
  click += dT;
  if (1 >  click)   return;
  click -= 1.0; 
  sec++;
  if (60 == sec)  {sec = 0; min++; }
  if (99 == min)  {min = 0; }
  _snprintf(timText,8,"%02u:%02u",min,sec); 
  return;
}
//------------------------------------------------------------------------
//  Arm or disarm the timer
//------------------------------------------------------------------------
void  CTimer::Change(char sta)
{ state   = sta;
  click   = 0;
  min     = 0;
  sec     = 0;
  strcpy(timText,"00:00");
  return;
}
//------------------------------------------------------------------------
//  Set Initial Time
//------------------------------------------------------------------------
void CTimer::SetTime(int time)
{ min  = time % 60;
  sec  = time % 60;
  if (min > 99) min = 99;
  _snprintf(timText,8,"%02u:%02u",min,sec);
  return;
}
//------------------------------------------------------------------------
//  Toggle the state
//------------------------------------------------------------------------
int CTimer::Toggle()
{ state ^= 1;
  Change(state);
  return state;
}
//========================================================================
// CTransponderRadio:  Event tables
//=========================================================================
//-------------------------------------------------------------------------
//  Power OFF
//-------------------------------------------------------------------------
RADIO_HIT Xpd_POF[] = {
  {RADIO_CA09,XPDR_MODE},                     // Change mode
  {0,0},
};
//-------------------------------------------------------------------------
// Power ON
//-------------------------------------------------------------------------
RADIO_HIT Xpd_PON[] = {
  {RADIO_CA01,XPDR_ADG1},                      // CK01: Active digit 1
  {RADIO_CA02,XPDR_ADG2},                      // CK02: Active digit 2
  {RADIO_CA03,XPDR_ADG3},                      // CK03: Active digit 3
  {RADIO_CA04,XPDR_ADG4},                      // CK04: AActive digit 4
  {RADIO_CA05,XPDR_SDG1},                      // CK05: Standby digit 1
  {RADIO_CA06,XPDR_SDG2},                      // CK06: Standby digit 2
  {RADIO_CA07,XPDR_SDG3},                      // CK07: Standby digit 3
  {RADIO_CA08,XPDR_SDG4},                      // CK08: Standby digit 4
  //------------------------------------------------------------------------
  {RADIO_CA09,XPDR_MODE},                      // CK09: Change mode
  {0,0},
};
//---------------------------------------------------------------------------
RADIO_HIT *CTransponderRadio::XdtHIT[] = {
  Xpd_POF,                                      // State 0 Power OFF
  Xpd_POF,                                      // Start up 1
  Xpd_POF,                                      // Start up 2
  Xpd_PON,                                      // Standby
  Xpd_PON,                                      // Test (temporary)
  Xpd_PON,                                      // Power ON
  Xpd_PON,                                      // ALT mode (temporary)
};
//========================================================================
// CTransponderRadio:  Advance mode
//=========================================================================
U_CHAR  CTransponderRadio::XpdFWD[] =
{   XPDR_STANDBY,                         // 0 = >From OFF to Standby
    0,                                    // 1  Not used
    0,                                    // 2  Not used
    XPDR_TEST,                            // 3 => From Stadby to test 
    XPDR_ON,                              // 4 => From Test to ON
    XPDR_ALT,                             // 5 From ON to ALT
    XPDR_END,                             // 6 No More
};
//========================================================================
// CTransponderRadio:  Backward mode
//=========================================================================
U_CHAR  CTransponderRadio::XpdBKW[] =
{   XPDR_END,                             // 0 No more
    0,                                    // 1  Not used
    0,                                    // 2  Not used
    XPDR_OFF,                             // 3 => From standby to OFF 
    XPDR_STANDBY,                         // 4 => From Test to Standby
    XPDR_TEST,                            // 5 From ON to Test
    XPDR_ON,                              // 6 From ALT to ON
};
//========================================================================
// CTransponderRadio:  Mode state
//=========================================================================
U_CHAR CTransponderRadio::XpdBTM[] = 
{   0,                                    // 0 OFF
    0,                                    // Start up 1
    0,                                    // Start up 2
    1,                                    // Standby
    2,                                    // Test
    3,                                    // ON
    4,                                    // ALT
};
//========================================================================
// CTransponderRadio
//=========================================================================
CTransponderRadio::CTransponderRadio (void)
{
  TypeIs (SUBSYSTEM_TRANSPONDER_RADIO);

  //----------------------------------------------------------
  State = XPDR_OFF;
  strcpy (airc, "");
  lspd = uspd = 0;
  //-----------------------------------------------------------
  aCode = 1200;
  sCode = 1200;
  _snprintf(aIDN,6,"%04u",aCode);
  _snprintf(sIDN,6,"%04u",sCode);
  //------Init active digit fields -------------------------
  InitTable(Fields,0,0,0);
  InitTable(Fields,1,aIDN+0,RADIO_CA01);      // Active digit 1
  InitTable(Fields,2,aIDN+1,RADIO_CA02);      // Active digit 2
  InitTable(Fields,3,aIDN+2,RADIO_CA03);      // Active digit 3
  InitTable(Fields,4,aIDN+3,RADIO_CA04);      // Active digit 4
  //-----Init standby digit fields ----------------------------
  InitTable(Fields,5,sIDN+0,RADIO_CA05);      // Active digit 1
  InitTable(Fields,6,sIDN+1,RADIO_CA06);      // Active digit 2
  InitTable(Fields,7,sIDN+2,RADIO_CA07);      // Active digit 3
  InitTable(Fields,8,sIDN+3,RADIO_CA08);      // Active digit 4
}
//------------------------------------------------------------------------
//  Read all parameters
//------------------------------------------------------------------------
int CTransponderRadio::Read (SStream *stream, Tag tag)
{
  int rc = TAG_IGNORED;

  switch (tag) {
  case 'mode':
    {
      int m = XPDR_OFF;
      ReadInt (&m, stream);
    }
    rc = TAG_READ;
    break;
  case 'smod':
    {
      int m = XPDR_SWITCH_OFF;
      ReadInt (&m, stream);
    }
    rc = TAG_READ;
    break;
  case 'airc':
    ReadString (airc, 16, stream);
    rc = TAG_READ;
    break;
  case 'lspd':
    // Lower airspeed limit
    ReadInt (&lspd, stream);
    rc = TAG_READ;
    break;
  case 'uspd':
    // Upper airspeed limit
    ReadInt (&uspd, stream);
    rc = TAG_READ;
    break;
  case 'mAlt':
    // Altitude message
    ReadMessage (&mAlt, stream);
    rc = TAG_READ;
    break;
  }

  if (rc != TAG_READ) {
    // See if the tag can be processed by the parent class type
    rc = CRadio::Read (stream, tag);
  }

  return rc;
}
//--------------------------------------------------------------------
//  Modify a digit 
//--------------------------------------------------------------------
void CTransponderRadio::ModActiveDigit(int xd)
{ char bvl = aIDN[xd] - '0';              // binary value 
  bvl += mDir;                            // Inc/dec
  if (-1 == bvl)  bvl = 7;
  if (+8 == bvl)  bvl = 0;
  aIDN[xd] = '0' | bvl;
  aCode = atoi(aIDN);
  return;
}
//--------------------------------------------------------------------
//  Modify a digit 
//--------------------------------------------------------------------
void CTransponderRadio::ModStandbyDigit(int xd)
{ char bvl = sIDN[xd] - '0';              // binary value 
  bvl += mDir;                            // Inc/dec
  if (-1 == bvl)  bvl = 7;
  if (+8 == bvl)  bvl = 0;
  sIDN[xd] = '0' | bvl;
  sCode = atoi(sIDN);
  return;
}
//------------------------------------------------------------------------
//  Process messages
//------------------------------------------------------------------------
EMessageResult CTransponderRadio::ReceiveMessage (SMessage *msg)
{ switch (msg->id) {
    case MSG_SETDATA:
      switch (msg->user.u.datatag) {
      case 'i11+':
        // Increment transponder code digit 1
        return MSG_PROCESSED;
      case 'i11-':
        // Decrement transponder code digit 1
        return MSG_PROCESSED;
      case 'i12+':
        // Increment transponder code digit 2
        return MSG_PROCESSED;
      case 'i12-':
        // Decrement transponder code digit 2
        return MSG_PROCESSED;
      case 'i13+':
        // Increment transponder code digit 3
        return MSG_PROCESSED;
      case 'i13-':
        // Decrement transponder code digit 3
        return MSG_PROCESSED;
      case 'i14+':
        // Increment transponder code digit 4
        return MSG_PROCESSED;
      case 'i14-':
        // Decrement transponder code digit 4
        return MSG_PROCESSED;
      case 'idt_':
        // Ident button
        return MSG_PROCESSED;
      case 'mods':
        // Mode switch
        return MSG_PROCESSED;
      case 'mode':
        // Operational mode
        return MSG_PROCESSED;
      case 'mod+':
        // Increment operational mode
        return MSG_PROCESSED;
      case 'mod-':
        // Decrement operational mode
        return MSG_PROCESSED;
      case 'tune':
        // Set active frequency
        return MSG_PROCESSED;
      case 'stby':
        // Set standby frequency
        return MSG_PROCESSED;
      }
    case MSG_GETDATA:
      switch (msg->user.u.datatag) {
      case 'gets':
        // Return a pointer to this radio 
        msg->voidData = this;
        return MSG_PROCESSED;
      case 'tune':
        // return active ident
        msg->realData = double(aCode);
        return MSG_PROCESSED;
      case 'smod':
        // Mode switch state
        return MSG_PROCESSED;
      case 'mode':
        // Get operational mode
        if (msg->dataType = TYPE_INT)   msg->intData  = State;
        if (msg->dataType = TYPE_REAL)  msg->realData = State;
        return MSG_PROCESSED;
      case 'alti':
        // Get altitude
        msg->intData  = 0;
        return MSG_PROCESSED;
      case 'on__':
        // Get On indicator status
        return MSG_PROCESSED;
      case 'sby_':
        // Get Standby indicator status
        return MSG_PROCESSED;
      case 'rply':
        // Get Reply indicator status
        return MSG_PROCESSED;
      case 'alt_':
        // Get Altitude 
        return MSG_PROCESSED;
      case 'fl__':
        // Get Flight Level indicator status
        return MSG_PROCESSED;
      }
    }

  return CRadio::ReceiveMessage (msg);
}
//------------------------------------------------------------------------
//  Event change mode
//------------------------------------------------------------------------
void CTransponderRadio::ChangeMode()
{ U_CHAR *tab = (mDir > 0)?(XpdFWD):(XpdBKW);
  U_CHAR  nst = tab[State];
  if (nst == XPDR_END)  return;
  State       = nst;
  return;
}
//------------------------------------------------------------------------
//  Dispatcher for events
//------------------------------------------------------------------------
void CTransponderRadio::Dispatcher(U_CHAR evn)
{ switch (evn)  {
  //----Change the mode -------------------------------
  case XPDR_MODE:
    return ChangeMode();
  //----Change active digit 1 --------
  case XPDR_ADG1:
    return ModActiveDigit(0);
  //----Change active digit 2 --------
  case XPDR_ADG2:
    return ModActiveDigit(1);
  //----Change active digit 3 --------
  case XPDR_ADG3:
    return ModActiveDigit(2);
  //----Change active digit 4 --------
  case XPDR_ADG4:
    return ModActiveDigit(3);
  //----Change standby digit 1 --------
  case XPDR_SDG1:
    return ModStandbyDigit(0);
  //----Change standby digit 2 --------
  case XPDR_SDG2:
    return ModStandbyDigit(1);
  //----Change standby digit 3 --------
  case XPDR_SDG3:
    return ModStandbyDigit(2);
  //----Change standby digit 4 --------
  case XPDR_SDG4:
    return ModStandbyDigit(3);

  }
              
  return;
}
//------------------------------------------------------------------------
//  Time slice: Proceed according to state
//------------------------------------------------------------------------
void CTransponderRadio::TimeSlice (float dT,U_INT FrNo)				// JSDEV*
{
  CRadio::TimeSlice (dT,FrNo);										// JSDEV*
}
//===================================================================================
// CBKKT76Radio
//===================================================================================
CBKKT76Radio::CBKKT76Radio (void)
{
  TypeIs (SUBSYSTEM_KT76_RADIO);

  entryMode = false;
  strcpy (tempCode, "    ");
}


int CBKKT76Radio::Read (SStream *stream, Tag tag)
{
  int rc = TAG_IGNORED;

  if (rc != TAG_READ) {
    // See if the tag can be processed by the parent class type
    rc = CTransponderRadio::Read (stream, tag);
  }

  return rc;
}

EMessageResult CBKKT76Radio::ReceiveMessage (SMessage *msg)
{ char digit = '0';
  bool digitPressed = false;

  switch (msg->id) {
    case MSG_SETDATA:
      switch (msg->user.u.datatag) {
      case '___0':
        // Zero button
        digit = '0';
        digitPressed  = true;
        return MSG_PROCESSED;
      case '___1':
        // One button
        digit = '1';
        digitPressed = true;
        return MSG_PROCESSED;
      case '___2':
        // Two button
        digit = '2';
        digitPressed  = true;
        return MSG_PROCESSED;
      case '___3':
        // Three button
        digit = '3';
        digitPressed  = true;
        return MSG_PROCESSED;
      case '___4':
        // Four button
        digit = '4';
        digitPressed  = true;
        return MSG_PROCESSED;
      case '___5':
        // Five button
        digit = '5';
        digitPressed  = true;
        return MSG_PROCESSED;
      case '___6':
        // Six button
        digit = '6';
        digitPressed  = true;
        return MSG_PROCESSED;
      case '___7':
        // Seven button
        digit = '7';
        digitPressed  = true;
        return MSG_PROCESSED;
      case 'clr_':
        // Clear button
        if (entryMode) {
          entryDigit--;
          tempCode[entryDigit] = ' ';
          if (entryDigit == 0) entryMode = false;
        }
        digitPressed  = false;
        return MSG_PROCESSED;
      case 'vfr_':
        // VFR button
        return MSG_PROCESSED;
      }

      if (digitPressed) {
        if (!entryMode) {
          // Transition to entry mode
          entryMode = true;
          entryDigit = 0;
          strcpy (tempCode, "    ");
        }

        // Check if entire code has been entered
        if (entryDigit == 3) {
          // Code entry complete
          int d0 = digit - '0';
          int d1 = tempCode[2] - '0';
          int d2 = tempCode[1] - '0';
          int d3 = tempCode[0] - '0';
          entryMode = false;
        } else {
          // Store digit in temporary code storage
          tempCode[entryDigit] = digit;
          entryDigit++;
        }
        
        // Reset entry mode timer on every keypress
        entryTimer = 0;
        return MSG_PROCESSED;
      }

    case MSG_GETDATA:
      // Override getdata message for 'tune' to send code or partial code
      switch (msg->user.u.datatag) {
      case 'tune':
        if (entryMode) {
          // Return partially entered code as CHAR* data
          msg->dataType     = TYPE_CHARPTR;
          msg->charPtrData  = tempCode;
          return MSG_PROCESSED;
        } else {
          // Return active transponder code as INT data
          msg->dataType = TYPE_INT;
          return MSG_PROCESSED;
        }
      }
    }

  return CTransponderRadio::ReceiveMessage (msg);
}

void CBKKT76Radio::TimeSlice (float dT,U_INT FrNo)			// JSDEV*
{
  // Call parent class timeslice
  CTransponderRadio::TimeSlice (dT,FrNo);					// JSDEV*

  // Transition back to idle state if four seconds have elapsed without keypress
  if (entryMode) {
    entryTimer += dT;
    if (entryTimer >= 4.0f) {
      entryMode = false;
    }
  }
}



//
// CAudioPanelRadio
//
CAudioPanelRadio::CAudioPanelRadio (void)
{
  TypeIs (SUBSYSTEM_AUDIO_PANEL_RADIO);
}


//
// CBKKAP140Radio
//
CBKKAP140Radio::CBKKAP140Radio (void)
{
  TypeIs (SUBSYSTEM_KAP140_RADIO);
}
//===================================END OF FILE ========================================================

