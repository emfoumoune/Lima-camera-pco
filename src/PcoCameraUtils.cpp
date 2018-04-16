/**************************************************************************
###########################################################################
 This file is part of LImA, a Library for Image Acquisition

 Copyright (C) : 2009-2011
 European Synchrotron Radiation Facility
 BP 220, Grenoble 38043
 FRANCE

 This is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This software is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, see <http://www.gnu.org/licenses/>.
###########################################################################
**************************************************************************/

#ifndef __linux__
#include <windows.h>
#include <tchar.h>
#endif
#include <stdio.h>

#include <cstdlib>

#ifndef __linux__
#include <process.h>
#else
#include <unistd.h>
#endif

#include <sys/stat.h>

#include <sys/timeb.h>
#include <time.h>

#include "lima/HwSyncCtrlObj.h"

#include "lima/Exceptions.h"

#include "PcoSdkVersion.h"

#include "PcoCamera.h"
#include "PcoCameraUtils.h"
#include "PcoSyncCtrlObj.h"
#include "PcoBufferCtrlObj.h"

using namespace lima;
using namespace lima::Pco;

//const char *timebaseUnits[] = {"ns", "us", "ms"};

#define BUFF_INFO_SIZE 10000


void print_hex_dump_buff(void *ptr_buff, size_t len);
int __xlat_date(char *s1, char &ptrTo, int lenTo) ;
char *_xlat_date(char *s1, char *s2, char *s3) ;

//=========================================================================================================
const char* _timestamp_pcocamerautils() {return ID_FILE_TIMESTAMP ;}
//=========================================================================================================

//=========================================================================================================
// dummy comments for test 02ccc
//=========================================================================================================

//=========================================================================================================
//=========================================================================================================
int getNrBitsOn(WORD x) 
{
	int count = 0;
	WORD mask = 1;

	for(int i = 0; i <8; i++)
	{
		if(x & mask) count++;
		mask = mask << 1;
	}
	return count;
}
//=========================================================================================================
//=========================================================================================================
char *getTimestamp(timestampFmt fmtIdx, time_t xtime) {
   static char timeline[128];
   time_t ltime;
	struct tm today;
	const char *fmt;

  switch(fmtIdx) {
    case Iso: fmt = "%Y/%m/%d %H:%M:%S"; break;
    case IsoHMS: fmt = "%H:%M:%S"; break;
    case FnDate: fmt = "%Y-%m-%d"; break;

    default:
    case FnFull: fmt = "%Y-%m-%d-%H%M%S"; break;
  }

	if(xtime == 0) 
		time( &ltime );
	else
		ltime = xtime;
	localtime_s( &today, &ltime );
	strftime(timeline, 128, fmt, &today );
      
	return timeline;
}

time_t getTimestamp() { return time(NULL); }

//====================================================================
//====================================================================
//$Id: [Oct  8 2013 15:21:07] [Tue Oct  8 15:21:07 2013] (..\..\..\..\src\PcoCamera.cpp) $

#define LEN_BUFF_DATE 128
#define TOKNR_DT 5


int __xlat_date(char *s1, char &ptrTo, int lenTo) {
	char *tok[TOKNR_DT];
	char *tokNext = NULL;
	int tokNr, iM, iD, iY, i;
	char *ptr;
	const char *months = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec";
	const char *sM, *sT;
	char buff[LEN_BUFF_DATE+1];

	strcpy_s(buff, LEN_BUFF_DATE, s1);
	ptr = buff;

	for(tokNr = i = 0; i < TOKNR_DT; i++) {
		if( (tok[i] = strtok_s(ptr, " ", &tokNext)) == NULL) break;
		ptr = NULL;
		tokNr = i+1;
	}

	if(tokNr == 4) {
		// Oct  8 2013 15:21:07
		sM = tok[0]; iD = atoi(tok[1]); iY = atoi(tok[2]); sT = tok[3]; 
	} else if(tokNr == 5) {
		// Tue Oct  8 15:21:07 2013
		sM = tok[1]; iD = atoi(tok[2]); iY = atoi(tok[4]); sT = tok[3]; 
	} else {
		sM = "xxx"; iD = 99; iY = 9999; sT = "99:99:99"; 
	}
	
	ptr = strstr((char *) months,sM);
	iM = (ptr != NULL) ? ( int(ptr - months) / 4) + 1 : 99;


	return sprintf_s(&ptrTo, lenTo, "%04d/%02d/%02d %s", iY, iM, iD, sT);
}

char *_xlat_date(char *s1, char *s2, char *s3) {
	static char buff[LEN_BUFF_DATE+1];
	char *ptr = buff;
	char *ptrMax = buff + LEN_BUFF_DATE;

	ptr += sprintf_s(ptr, ptrMax - ptr, "$Id: comp[");
	ptr += __xlat_date(s1, *ptr, (int) (ptrMax - ptr));
	ptr += sprintf_s(ptr, ptrMax - ptr, "] file[");
	ptr += __xlat_date(s2, *ptr, (int) (ptrMax - ptr));
	ptr += sprintf_s(ptr, ptrMax - ptr, "] [%s] $", s3);
	return buff;
	
}

char *_split_date(const char *s) {
	static char s1[LEN_BUFF_DATE+1];
	static char s2[LEN_BUFF_DATE+1];
	static char s3[LEN_BUFF_DATE+1];
	const char *ptr1, *ptr2;

	ptr1 = strchr(s,'[');
	ptr2 = strchr(ptr1,']');
	strncpy_s(s1, LEN_BUFF_DATE, ptr1+1, ptr2-ptr1-1);
    s1[ptr2-ptr1-1] = 0;

	ptr1 = strchr(ptr2,'[');
	ptr2 = strchr(ptr1,']');
	strncpy_s(s2, LEN_BUFF_DATE, ptr1+1, ptr2-ptr1-1);
    s2[ptr2-ptr1-1] = 0;

	ptr1 = strchr(ptr2,'[');
	ptr2 = strchr(ptr1,']');
	strncpy_s(s3, LEN_BUFF_DATE, ptr1+1, ptr2-ptr1-1);
    s3[ptr2-ptr1-1] = 0;

	return _xlat_date(s1, s2, s3);
}


//====================================================================
//====================================================================

char *str_trim_left(char *s) {
	char c;
	if(s == NULL) return NULL;
	while((c = *s) != 0) {
		if(!isspace(c)) break;
		s++;		
	}
	return s;
}

char *str_trim_right(char *s) {
	char *ptr;
	if(s == NULL) return NULL;
	ptr = s + strlen(s) - 1;
	while(s <= ptr) {
		if(!isspace(*ptr)) break;
		*ptr-- = 0;
	}
	return s;
}
char *str_trim(char *s) {
	return str_trim_left(str_trim_right(s));
}

char *str_toupper(char *s) {
	char *ptr = s;
	while(*ptr) { 
		*ptr = toupper(*ptr);
		ptr++;
	}
	return s;
}


//=========================================================================================================
//=========================================================================================================

void stcPcoData::traceAcqClean(){
	void *ptr = &this->traceAcq;
	memset(ptr, 0, sizeof(struct stcTraceAcq));
}

void stcPcoData::traceMsg(char *s){
	char *ptr = traceAcq.msg;
	char *dt = getTimestamp(IsoHMS);
	size_t lg = strlen(ptr);
	ptr += lg;
	snprintf(ptr, LEN_TRACEACQ_MSG - lg,"%s> %s", dt, s);
}

static char buff[BUFF_INFO_SIZE +16];
const char *Camera::talk(const char *cmd){
	DEB_MEMBER_FUNCT();

	static char buff[BUFF_INFO_SIZE +16];
	sprintf_s(buff, BUFF_INFO_SIZE, "talk> %s", cmd);
	m_msgLog->add(buff);

	return _talk(cmd, buff, BUFF_INFO_SIZE);
}

#define NRTOK 10
#define NRCMDS 200
const char *Camera::_talk(const char *_cmd, char *output, int lg){
	DEB_MEMBER_FUNCT();
		char cmdBuff[BUFF_INFO_SIZE +1];
		char cmdBuffAux[BUFF_INFO_SIZE +1];
		const char  *keys[NRCMDS];
		const char *key, *keys_desc[NRCMDS];
		const char *cmd;
		int ikey = 0;
		const char *tok[NRTOK];
		int tokNr;
		char *ptr, *ptrMax;
		//int segmentPco = m_pcoData->wActiveRamSegment;
		//int segmentArr = segmentPco -1;
		
		ptr = output; *ptr = 0;
		ptrMax = ptr + lg;

		//int width = +20;

		strncpy_s(cmdBuff, BUFF_INFO_SIZE, _cmd, BUFF_INFO_SIZE-1);
		cmd = str_trim(cmdBuff);
		strncpy_s(cmdBuffAux, BUFF_INFO_SIZE, cmd, BUFF_INFO_SIZE-1);

		if(*cmd){
			char *tokContext;
			for(int i=0; i < NRTOK; i++) {
				if( (tok[i] = strtok_s((char *) cmd, " ", &tokContext)) == NULL) break;
				cmd = NULL;
				tokNr = i;
			}
			cmd = tok[0];
		} else {
			cmd = "camInfo";
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "camInfo";     
		keys_desc[ikey++] = "(R) detailed cam info (type, if, sn, hw & fw ver, ...)";     
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_ALL);

			return output;
		}

		//----------------------------------------------------------------------------------------------------------

#ifndef __linux__
#define BUFFER_LEN 256
		key = keys[ikey] = "getVersionDLL";     
		keys_desc[ikey++] = "(R) get verions of loaded DLLs";     
		if(_stricmp(cmd, key) == 0){
			char buff[BUFFER_LEN+1];

			ptr += sprintf_s(ptr, ptrMax - ptr, "%s", _getPcoSdkVersion(buff, BUFFER_LEN, (char *) "sc2_cam.dll"));
			ptr += sprintf_s(ptr, ptrMax - ptr, "%s", _getPcoSdkVersion(buff, BUFFER_LEN, (char *) "sc2_cl_me4.dll"));
			ptr += sprintf_s(ptr, ptrMax - ptr, "%s", _getPcoSdkVersion(buff, BUFFER_LEN, (char *) "sc2_clhs.dll"));
			return output;
		}
#endif

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "getVersionFile";     
		keys_desc[ikey++] = "(R) reads INSTALL_VERSION.txt";     
		if(_stricmp(cmd, key) == 0){
			_getDllPath(FILE_PCO_DLL, ptr, ptrMax -ptr);
			return output;
		}
		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "expDelayTime";     
		keys_desc[ikey++] = "(R) exposure & delay time (actual & valid ranges)";
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_EXP);
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "timingInfo";     
		keys_desc[ikey++] = "(R) timing information (exp, trig delay, ...)";
		if(_stricmp(cmd, key) == 0){
			double frameTime, expTime, sysDelay, sysJitter, trigDelay;

			_pco_GetImageTiming(frameTime, expTime, sysDelay, sysJitter, trigDelay );


			ptr += sprintf_s(ptr, ptrMax - ptr, "frameTime %g  ",  frameTime);
			ptr += sprintf_s(ptr, ptrMax - ptr, "expTime %g  ",  expTime);
			ptr += sprintf_s(ptr, ptrMax - ptr, "sysDelay %g  ",  sysDelay);
			ptr += sprintf_s(ptr, ptrMax - ptr, "sysJitter %g  ",  sysJitter);
			ptr += sprintf_s(ptr, ptrMax - ptr, "trigDelay %g  ",  trigDelay);
			ptr += sprintf_s(ptr, ptrMax - ptr, "\n");

			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "cocRunTime";     
		keys_desc[ikey++] = "(R) Camera Operation Code runtime covers the delay, exposure and readout time";
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr, "%g",  m_pcoData->cocRunTime);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "frameRate";     
		keys_desc[ikey++] = "(R) max frame rate (calculated as 1/cocRunTime)";   
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr, "%g", m_pcoData->frameRate);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "timestamp";     
		keys_desc[ikey++] = "(R) timestamp of compiled modules";     
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_VERSION);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "timestampAction";     
		keys_desc[ikey++] = "(R) action timestamps";     
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr,  "            now [%s]\n", getTimestamp(Iso));
			ptr += sprintf_s(ptr, ptrMax - ptr,  "    constructor [%s]\n", getTimestamp(Iso, _getActionTimestamp(tsConstructor)));
			ptr += sprintf_s(ptr, ptrMax - ptr,  "     last reset [%s]\n", getTimestamp(Iso, _getActionTimestamp(tsReset)));
			ptr += sprintf_s(ptr, ptrMax - ptr,  "last prepareAcq [%s]\n", getTimestamp(Iso, _getActionTimestamp(tsPrepareAcq)));
			ptr += sprintf_s(ptr, ptrMax - ptr,  "  last startAcq [%s]\n", getTimestamp(Iso, _getActionTimestamp(tsStartAcq)));
			ptr += sprintf_s(ptr, ptrMax - ptr,  "   last stopAcq [%s]\n", getTimestamp(Iso, _getActionTimestamp(tsStopAcq)));

			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "clTransferParam";     
		keys_desc[ikey++] = "(R) CameraLink transfer parameters";     
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_CAMERALINK);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "maxNbImages";     
		keys_desc[ikey++] = "(R) for DIMAX 2K 4K only / max number of images in the active ram segment";     
		if(_stricmp(cmd, key) == 0){
			if(!_isCameraType(Dimax | Pco2k | Pco4k )) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", -1);
				return output;
			}

			ptr += sprintf_s(ptr, ptrMax - ptr, "%ld", _pco_GetNumberOfImagesInSegment_MaxCalc(m_pcoData->wActiveRamSegment));
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "dumpRecordedImg";     
		keys_desc[ikey++] = "(R) for DIMAX only / TODO";     
		if(_stricmp(cmd, key) == 0){
			int res, nrImages, error;

			res = dumpRecordedImages(nrImages, error);


			ptr += sprintf_s(ptr, ptrMax - ptr, "res[%d] nrImages[%d] error[0x%x] ", res, nrImages, error);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "allocatedBuffer";     
		keys_desc[ikey++] = "(R) TODO";     
		if(_stricmp(cmd, key) == 0){
			int error;
			unsigned int bytesPerPix; getBytesPerPixel(bytesPerPix);
			WORD _wArmWidth, _wArmHeight, _wMaxWidth, _wMaxHeight;
			_pco_GetSizes(&_wArmWidth, &_wArmHeight, &_wMaxWidth, &_wMaxHeight, error);

			int sizeBytes = _wArmWidth * _wArmHeight * bytesPerPix;
			ptr += sprintf_s(ptr, ptrMax - ptr, "IMAGE info:\n"
			                                    "    X=[%d] Y=[%d] bytesPerPix=[%d] size=[%d B]\n",  
				_wArmWidth,  _wArmHeight, bytesPerPix, sizeBytes);
			
			ptr += sprintf_s(ptr, ptrMax - ptr, "PCO API allocated buffers:\n"
												"    allocated=[%s] nrBuff=[%d] size=[%d B][%g MB] imgPerBuff[%d]\n", 
				m_pcoData->bAllocatedBufferDone ? "TRUE" : "FALSE", 
				m_pcoData->iAllocatedBufferNumber, 
				m_pcoData->dwAllocatedBufferSize, m_pcoData->dwAllocatedBufferSize/1000000.,
				m_pcoData->dwAllocatedBufferSize/sizeBytes);

			ptr += sprintf_s(ptr, ptrMax - ptr, "LIMA allocated buffers: \n"
												"    nr of buffers=[%d] \n", 
				m_pcoData->iAllocatedBufferNumberLima);


			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "timeDimax";     
		keys_desc[ikey++] = "(R) for DIMAX only / acq time details (record and transfer time)";     
		if(_stricmp(cmd, key) == 0){

			if(!(_isCameraType(Dimax | Pco2k | Pco4k))) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "* ERROR - only for DIMAX / 2K");
				return output;
			}

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* Acq: rec[%ld] xfer[%ld] recLoopTime[%ld] recLoopTout[%ld] acqAll[%ld] (ms) [%s]\n",
				m_pcoData->msAcqRec, m_pcoData->msAcqXfer,  
				m_pcoData->msAcqTnow, m_pcoData->msAcqTout, 
				m_pcoData->msAcqAll, 
				getTimestamp(Iso, m_pcoData->msAcqRecTimestamp));


			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "lastImgRecorded";     
		keys_desc[ikey++] = "last image recorded";     
		if(_stricmp(cmd, key) == 0){
			DWORD lastImgRecorded = m_pcoData->traceAcq.nrImgRecorded;

			if(!(_isCameraType(Dimax | Pco2k | Pco4k))) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", -1);
			} else {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", lastImgRecorded);
			}
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "lastImgAcquired";     
		keys_desc[ikey++] = "last image acquired";     
		if(_stricmp(cmd, key) == 0){
			DWORD lastImgAcquired = m_pcoData->traceAcq.nrImgAcquired;


			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", lastImgAcquired);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "traceAcq";     
		keys_desc[ikey++] = "(R) trace details (not all records are filled!)";     
		if(_stricmp(cmd, key) == 0){
			time_t _timet;

			if(0 && !(_isCameraType(Dimax | Pco2k | Pco4k))) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "* ERROR - only for DIMAX / 2K");
				return output;
			}

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"\n"
				"* fnId[%s] nrEvents[%d]\n"
				"* ... fnIdXfer[%s]\n",
				m_pcoData->traceAcq.fnId,
				m_pco_buffer_nrevents,
				m_pcoData->traceAcq.fnIdXfer);

			ptr += sprintf_s(ptr, ptrMax - ptr, "* ... testCmdMode [0x%llx]\n",  m_pcoData->testCmdMode);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... msExposure[%g] msDelay[%g]\n",
				m_pcoData->traceAcq.sExposure * 1000.,
				m_pcoData->traceAcq.sDelay * 1000.);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... msLimaExposure[%g] Pco exposure[%d] base[%d]\n",
				m_pcoData->traceAcq.dLimaExposure * 1000.,
				m_pcoData->traceAcq.iPcoExposure, 
				m_pcoData->traceAcq.iPcoExposureBase);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... msLimaDelay[%g] Pco delay[%d] base[%d]\n",
				m_pcoData->traceAcq.dLimaDelay * 1000.,
				m_pcoData->traceAcq.iPcoDelay, 
				m_pcoData->traceAcq.iPcoDelayBase);

			ptr += sprintf_s(ptr, ptrMax - ptr, "* pcoBin horz[%d] vert[%d]\n",  
					m_pcoData->traceAcq.iPcoBinHorz, 
					m_pcoData->traceAcq.iPcoBinVert);

			Roi limaRoi;
			int error;
			_pco_GetROI(limaRoi, error);

			Point top_left = limaRoi.getTopLeft();
			Point bot_right = limaRoi.getBottomRight();
			Size size = limaRoi.getSize();			
			unsigned int bytesPerPix; getBytesPerPixel(bytesPerPix);

			ptr += sprintf_s(ptr, ptrMax - ptr, "* limaRoi xy0[%d,%d] xy1[%d,%d] size[%d,%d]\n",  
					top_left.x, top_left.y,
					bot_right.x, bot_right.y,
					size.getWidth(), size.getHeight());


			ptr += sprintf_s(ptr, ptrMax - ptr, "* ... pcoRoi x[%d,%d] y[%d,%d]\n",  
					m_pcoData->traceAcq.iPcoRoiX0, 
					m_pcoData->traceAcq.iPcoRoiX1, 
					m_pcoData->traceAcq.iPcoRoiY0, 
					m_pcoData->traceAcq.iPcoRoiY1);


			long long imgSize = size.getWidth()* size.getHeight() * bytesPerPix;
			long long totSize = imgSize * m_pcoData->traceAcq.nrImgRequested;
			double mbTotSize =  totSize/(1024.*1024.);
			double totTime = m_pcoData->traceAcq.msXfer / 1000.;
			double xferSpeed = mbTotSize / totTime;
			double framesPerSec = m_pcoData->traceAcq.nrImgRequested / totTime;
			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... imgSize[%lld B] totSize[%lld B][%g MB]\n",  
				imgSize, totSize, mbTotSize);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* nrImgRequested[%d] nrImgAcquired[%d]\n",
				m_pcoData->traceAcq.nrImgRequested,
				m_pcoData->traceAcq.nrImgAcquired);


			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... nrImgRequested0[%d] nrImgRecorded[%d] maxImgCount[%d]\n",
				m_pcoData->traceAcq.nrImgRequested0,
				m_pcoData->traceAcq.nrImgRecorded,
				m_pcoData->traceAcq.maxImgCount);

			ptr += sprintf_s(ptr, ptrMax - ptr,	
				"* limaTriggerMode[%s]\n",
				m_pcoData->traceAcq.sLimaTriggerMode);
			ptr += sprintf_s(ptr, ptrMax - ptr,	
				"* ... pcoTriggerMode[%s] [%d]\n",
				m_pcoData->traceAcq.sPcoTriggerMode,
				m_pcoData->traceAcq.iPcoTriggerMode);
			ptr += sprintf_s(ptr, ptrMax - ptr,	
				"* ... pcoAcqMode[%s] [%d]\n",
				m_pcoData->traceAcq.sPcoAcqMode,
				m_pcoData->traceAcq.iPcoAcqMode);


			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* msStartAcqStart[%ld]  msStartAcqEnd[%ld]\n",
				m_pcoData->traceAcq.msStartAcqStart, m_pcoData->traceAcq.msStartAcqEnd);
			

			for(int _i = 0; _i < LEN_TRACEACQ_TRHEAD; _i++){
				const char *desc = m_pcoData->traceAcq.usTicks[_i].desc;
				if(desc != NULL) {
					ptr += sprintf_s(ptr, ptrMax - ptr, 
						"* ... usTicks[%d][%5.3f] (ms)   (%s)\n", 
						_i, m_pcoData->traceAcq.usTicks[_i].value/1000.,
						desc);
			
				}
			}

			_timet = m_pcoData->traceAcq.endRecordTimestamp;

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* msImgCoc[%.3g] fps[%.3g] msTout[%ld] msTotal[%ld]\n",
				m_pcoData->traceAcq.msImgCoc, 
				1000. / m_pcoData->traceAcq.msImgCoc,
				m_pcoData->traceAcq.msTout,
				m_pcoData->traceAcq.msTotal);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... msRecordLoop[%ld] msRecord[%ld] endRecord[%s]\n",
				m_pcoData->traceAcq.msRecordLoop,
				m_pcoData->traceAcq.msRecord,
				_timet ? getTimestamp(Iso, _timet) : "");

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... msXfer[%ld] endXfer[%s]\n",
				m_pcoData->traceAcq.msXfer,
				getTimestamp(Iso, m_pcoData->traceAcq.endXferTimestamp));

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... xferTimeTot[%g s] xferSpeed[%g MB/s][%g fps]\n",  
				totTime, xferSpeed, framesPerSec);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... checkImgNr pco[%d] lima[%d] diff[%d]\n",  
				m_pcoData->traceAcq.checkImgNrPco,
				m_pcoData->traceAcq.checkImgNrLima,
				m_pcoData->traceAcq.checkImgNrPco -	m_pcoData->traceAcq.checkImgNrLima);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"%s\n", m_pcoData->traceAcq.msg);

			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "testCmd";     
		keys_desc[ikey++] = "DISABLED / debug tool";     
		if(_stricmp(cmd, key) == 0){

			if((tokNr >= 1) &&  (_stricmp(tok[1], "mode")==0)){
				ptr += sprintf_s(ptr, ptrMax - ptr, "testCmdMode [0x%llx]",  m_pcoData->testCmdMode);
				if(tokNr >= 2){
					int nr;
					unsigned long long _testCmdMode;

					nr = sscanf_s(tok[2], "0x%llx", &_testCmdMode);
					
					if(nr == 1) {
						m_pcoData->testCmdMode = _testCmdMode;
						ptr += sprintf_s(ptr, ptrMax - ptr, "   changed OK>  ");
					} else {
						ptr += sprintf_s(ptr, ptrMax - ptr, "   ERROR - NOT changed>  ");
					}
					ptr += sprintf_s(ptr, ptrMax - ptr, "testCmdMode [0x%llx]",  m_pcoData->testCmdMode);
				}
				return output;
			}
			
			
			//--- test of close
#ifndef __linux__
			if((tokNr >= 1) &&  (_stricmp(tok[1], "close")==0)){
				int error;
				//char *msg;

				m_cam_connected = false;

				//m_sync->_getBufferCtrlObj()->_pcoAllocBuffersFree();
				m_buffer->_pcoAllocBuffersFree();
				_pco_CloseCamera(error);

				ptr += sprintf_s(ptr, ptrMax - ptr, "%s> closed cam\n", tok[1]);
				return output;
			}
#endif
			
			//--- test of callback   "testCmd cb"
			if((tokNr >= 1) &&  (_stricmp(tok[1], "cb")==0)){
				Event *ev = new Event(Hardware,Event::Error,Event::Camera,Event::Default, "test cb");
				m_HwEventCtrlObj->reportEvent(ev);
				ptr += sprintf_s(ptr, ptrMax - ptr, "%s> done\n", tok[1]);
				return output;
			}

			//--- test of sleep
			if((tokNr == 2) &&  (_stricmp(tok[1], "time")==0)){
				long long us;

				long long usStart;

				ptr += sprintf_s(ptr, ptrMax - ptr, "sleeping ...\n"); 

				usElapsedTimeSet(usStart);

				::Sleep(atoi(tok[2])*1000);


				us = usElapsedTime(usStart);
				double ticksPerSec = usElapsedTimeTicsPerSec();
				ptr += sprintf_s(ptr, ptrMax - ptr, "us[%lld] ms[%5.3f] tics/us[%g]\n", us, us/1000., ticksPerSec/1.0e6);

				return output;
			}





			if(tokNr == 0) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "tokNr [%d] cmd [%s] No parameters", tokNr, cmd); 
			} else {
				ptr += sprintf_s(ptr, ptrMax - ptr, "tokNr [%d] cmd [%s]\n", tokNr, cmd); 
				for(int i = 1; i<= tokNr; i++) {
					ptr += sprintf_s(ptr, ptrMax - ptr, "tok [%d] [%s]\n", i, tok[i]); 
				}
			}
			return output;
		}



		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "rollingShutter";
		keys_desc[ikey++] = "(RW) for EDGE only / rolling shutter mode";
		if(_stricmp(cmd, key) == 0){
			int error;
			DWORD dwRolling, dwRollingNew;

			if(!_isCameraType(Edge)) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", -1);
				return output;
			}
			
			_get_shutter_rolling_edge(dwRolling, error);
			if(tokNr == 0) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", dwRolling);
				return output;
			}

			dwRollingNew = atoi(tok[1]);

			if( (tokNr != 1) || !((dwRollingNew == 1) || (dwRollingNew == 2) || (dwRollingNew == 4)) ){
				ptr += sprintf_s(ptr, ptrMax - ptr, "syntax ERROR - %s <1 (rolling), 2 (global), 4 (global reset)>", cmd);
				return output;
			}
			
			if(dwRollingNew != dwRolling){
				_set_shutter_rolling_edge(dwRollingNew, error);
			}
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", dwRollingNew);
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "bitAlignment";     
		keys_desc[ikey++] = "(RW) bit alignment (LSB = 1, MSB = 0)";
		if(_stricmp(cmd, key) == 0){
			int alignment;
			bool syntax = false;
			const char *res;
			
			if( (tokNr < 0) || (tokNr > 1)) {
				syntax = true;
			}
			
			if(tokNr == 1) {
				alignment = atoi(tok[1]);
				if((alignment == 0) || (alignment == 1)){
					_pco_SetBitAlignment(alignment);
				} else {
					syntax = true;
				}
			}

			if(syntax) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR: syntax: bitAlignment [<1 (LSB) || 0 (MSB)>]");
				return output;
			}

			_pco_GetBitAlignment(alignment);
			res = alignment == 0 ? "0 (MSB)" : "1 (LSB)";
			ptr += sprintf_s(ptr, ptrMax - ptr, "%s", res);
			return output;


		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "acqTimeoutRetry";     
		keys_desc[ikey++] = "(RW) max Timeout retries during acq (0 - infinite)";
		if(_stricmp(cmd, key) == 0){
			
			
			if(tokNr >= 1) {
				int ival = atoi(tok[1]);
				m_pcoData->acqTimeoutRetry = ival < 0 ? 0 : ival;
			}

			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", m_pcoData->acqTimeoutRetry);
			return output;


		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "getCheckImgNrResults";     
		keys_desc[ikey++] = "(R) get the last checkImgNr results";
		if(_stricmp(cmd, key) == 0){
			//int alignment;
			//bool syntax = false;
			//char *res;
			

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"pco: %d lima: %d diff: %d",  
				m_pcoData->traceAcq.checkImgNrPco,
				m_pcoData->traceAcq.checkImgNrLima,
				m_pcoData->traceAcq.checkImgNrPco -	m_pcoData->traceAcq.checkImgNrLima);

			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "testFrameFirst0"; 
		keys_desc[ikey++] = "(RW) TEST - force FrameFirst to 0 [<0 | 1>]";
		if(_stricmp(cmd, key) == 0){
			if(tokNr == 0) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", m_pcoData->testForceFrameFirst0);
				return output;
			}

			m_pcoData->testForceFrameFirst0 = !!atoi(tok[1]);

			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", m_pcoData->testForceFrameFirst0);
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "pcoLogsEnabled";
		keys_desc[ikey++] = "(R) PCO log files enalbled";
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", m_pcoData->pcoLogActive);
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "pixelRate";
		keys_desc[ikey++] = "(RW) pixelrate (Hz) for reading images from the image sensor";
		if(_stricmp(cmd, key) == 0){
			DWORD pixRate, pixRateNext; int error;

			if(_isCameraType(Dimax) ) tokNr = 0;
			
			if( (tokNr < 0) || (tokNr > 1)) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR: syntax: pixelRate [<new value (Hz)>]");
				return output;
			}
			
			if(tokNr == 0) {
				_pco_GetPixelRate(pixRate, pixRateNext, error);
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", pixRateNext);
				return output;
			}

			pixRate = atoi(tok[1]);
			_presetPixelRate(pixRate, error);
			
			if(error){
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR: unsupported cam or invalid value");
				return output;
			}

			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", m_pcoData->dwPixelRateRequested);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "pixelRateInfo";
		keys_desc[ikey++] = "(R) pixelrate (Hz) for reading images from the image sensor (actual & valid values)";
		if(_stricmp(cmd, key) == 0){

			_camInfo(ptr, ptrMax, CAMINFO_PIXELRATE);
			
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "pixelRateValidValues";
		keys_desc[ikey++] = "(R) pixelrate (Hz) valid values";
		if(_stricmp(cmd, key) == 0){
			DWORD dwPixRate, dwPixRateNext ; int error, i, nr;

		    _pco_GetPixelRate(dwPixRate, dwPixRateNext, error);

			for(nr = i=0; i<4; i++) {
				dwPixRate = m_pcoData->stcPcoDescription.dwPixelRateDESC[i];
				if(dwPixRate){
					nr++;
					ptr += sprintf_s(ptr, ptrMax - ptr, "%d  ",dwPixRate);
				}  
			}	

			if(nr == 0)			
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d  ",nr);
			return output;
		}



		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "roi";
		keys_desc[ikey++] = "get actual (fixed) last ROI requested (unfixed) ROIs";
		if(_stricmp(cmd, key) == 0){
			unsigned int x0, x1, y0, y1;
			Roi new_roi;
			int error;

			Roi limaRoi;
			_pco_GetROI(limaRoi, error);

			if((tokNr != 0) ){
					ptr += sprintf_s(ptr, ptrMax - ptr, "syntax ERROR - %s ", cmd);
					return output;
			}
				
			_xlatRoi_lima2pco(limaRoi,x0, x1, y0, y1); 
			ptr += sprintf_s(ptr, ptrMax - ptr, "* roi PCO X(%d,%d) Y(%d,%d) size(%d,%d)\n",  
					x0, x1, y0, y1, x1-x0+1, y1-y0+1);
			{
			Point top_left = limaRoi.getTopLeft();
			Point bot_right = limaRoi.getBottomRight();
			Size size = limaRoi.getSize();

			ptr += sprintf_s(ptr, ptrMax - ptr, "* roiLima PCO XY0(%d,%d) XY1(%d,%d) size(%d,%d)\n",  
					top_left.x, top_left.y,
					bot_right.x, bot_right.y,
					size.getWidth(), size.getHeight());


			}



			return output;

		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "lastFixedRoi";
		keys_desc[ikey++] = "get last ROI fixed";
		if(_stricmp(cmd, key) == 0){
			unsigned int x0, x1, y0, y1;
			Roi new_roi;
			int error;

			Roi limaRoiRequested, limaRoiFixed, limaRoi;
			time_t dt;
			
			_get_logLastFixedRoi(limaRoiRequested, limaRoiFixed,  dt);

			_pco_GetROI(limaRoi, error);

			x0 = limaRoi.getTopLeft().x - 1;
			x1 = limaRoi.getBottomRight().x - 1;
			y0 = limaRoi.getTopLeft().y - 1;
			y1 = limaRoi.getBottomRight().y - 1;

			ptr += sprintf_s(ptr, ptrMax - ptr, "* roi PCO ACTUAL X(%d,%d) Y(%d,%d) size(%d,%d)\n",  
					x0, x1, y0, y1, x1-x0+1, y1-y0+1);


			if(dt)
			{
				limaRoi = limaRoiRequested; 
				x0 = limaRoi.getTopLeft().x - 1;
				x1 = limaRoi.getBottomRight().x - 1;
				y0 = limaRoi.getTopLeft().y - 1;
				y1 = limaRoi.getBottomRight().y - 1;

				ptr += sprintf_s(ptr, ptrMax - ptr, "* roi PCO REQUESTED X(%d,%d) Y(%d,%d) size(%d,%d) [%s]\n",  
						x0, x1, y0, y1, x1-x0+1, y1-y0+1, getTimestamp(Iso, dt));

				limaRoi = limaRoiFixed; 
				x0 = limaRoi.getTopLeft().x - 1;
				x1 = limaRoi.getBottomRight().x - 1;
				y0 = limaRoi.getTopLeft().y - 1;
				y1 = limaRoi.getBottomRight().y - 1;

				ptr += sprintf_s(ptr, ptrMax - ptr, "* roi PCO FIXED X(%d,%d) Y(%d,%d) size(%d,%d)\n",  
						x0, x1, y0, y1, x1-x0+1, y1-y0+1);

			}
			else
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "* roi PCO FIXED - no roi fixed yet!\n");  
			
			}

			bool bSymX, bSymY;
			unsigned int xMax, yMax, xSteps, ySteps, xMinSize, yMinSize;
			getXYdescription(xSteps, ySteps, xMax, yMax, xMinSize, yMinSize); 
			getRoiSymetrie(bSymX, bSymY );

			ptr += sprintf_s(ptr, ptrMax - ptr, "* xySteps[%d,%d] xyMinSize[%d,%d] xyMaxSize[%d,%d] xySym[%d,%d]\n",  
				xSteps, ySteps, xMinSize, yMinSize, xMax, yMax, bSymX, bSymY);


			return output;

		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "timestampMode"; 
		keys_desc[ikey++] = "(RW) pco timestampMode [<new value (0, 1, 2, 3)>]"; 
		if(_stricmp(cmd, key) == 0){
			int error, val;
			WORD wTimeStampMode;

			int valMax;

			if(!_isCapsDesc(capsTimestamp))
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "timestampmode not allowed\n");
				return output;
			}
			valMax = _isCapsDesc(capsTimestamp3) ? 3 : 2;

			_pco_GetTimestampMode(wTimeStampMode, error);
			if(error) 
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "SDK ERROR _pco_GetTimestampMode\n");
				return output;
			}
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d   ", wTimeStampMode);

			if((tokNr == 1)){
				val = atoi(tok[1]);
				
				if((val < 0) ||(val>valMax)) {
					ptr += sprintf_s(ptr, ptrMax - ptr, "invalid value [%d] must be (0 - %d)",  val, valMax);
				}else {
					wTimeStampMode = val;
					_pco_SetTimestampMode(wTimeStampMode, error);
					if(error) 
					{
						ptr += sprintf_s(ptr, ptrMax - ptr, "SDK ERROR _pco_SetTimestampMode\n");
						return output;
					}
					_pco_GetTimestampMode(wTimeStampMode, error);
					if(error) 
					{
						ptr += sprintf_s(ptr, ptrMax - ptr, "SDK ERROR _pco_GetTimestampMode\n");
						return output;
					}
					ptr += sprintf_s(ptr, ptrMax - ptr, "%d   ", wTimeStampMode);
				}
			}
			ptr += sprintf_s(ptr, ptrMax - ptr, "\n");
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "debug";     
		keys_desc[ikey++] = "(RW) pco debug level [<new value in hex format (0x123)>]";     
		if(_stricmp(cmd, key) == 0){
			int nr;

			ptr += sprintf_s(ptr, ptrMax - ptr, "0x%llx",  m_pcoData->debugLevel);

			if((tokNr == 1)){
					nr = sscanf_s(tok[1], "0x%llx",  &m_pcoData->debugLevel);
					ptr += sprintf_s(ptr, ptrMax - ptr, "   %s>  ",  (nr == 1) ? "changed OK": "NOT changed");
					ptr += sprintf_s(ptr, ptrMax - ptr, "0x%llx",  m_pcoData->debugLevel);
			
					DEB_TRACE() << output ;
			}
			
#define _PRINT_DBG( x )	ptr += sprintf_s(ptr, ptrMax - ptr, "%15s  0x%08x\n", #x, x ) 	

			if((tokNr == 0)){
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n");
				_PRINT_DBG( DBG_BUFF ) ;
				_PRINT_DBG( DBG_XFER2LIMA ) ;
				_PRINT_DBG( DBG_LIMABUFF ) ;
				_PRINT_DBG( DBG_EXP ) ;
				_PRINT_DBG( DBG_XFERMULT ) ;
				_PRINT_DBG( DBG_XFERMULT1 ) ;
				_PRINT_DBG( DBG_ASSIGN_BUFF ) ;
				_PRINT_DBG( DBG_DUMMY_IMG ) ;
				_PRINT_DBG( DBG_WAITOBJ ) ;
				_PRINT_DBG( DBG_XFER_IMG ) ;
				_PRINT_DBG( DBG_ROI ) ;
			}

			return output;
		}



		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "ADC";     
		keys_desc[ikey++] = "(RW) ADC working ADC [<new value>]";     
		if(_stricmp(cmd, key) == 0){
			
			int adc_new, adc_working, adc_max;

			//error = _pco_GetADCOperation(adc_working, adc_max);
			_pco_GetADCOperation(adc_working, adc_max);
			if((tokNr <1)){
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", adc_working);
				return output;
			}

			adc_new = atoi(tok[1]);
			
			//error = _pco_SetADCOperation(adc_new, adc_working);
			_pco_SetADCOperation(adc_new, adc_working);
			
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", adc_working);

			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "ADCmax";     
		keys_desc[ikey++] = "(R) max nr of ADC";     
		if(_stricmp(cmd, key) == 0){
			int error;
			int adc_working, adc_max;

			error = _pco_GetADCOperation(adc_working, adc_max);
			if(error) adc_max = adc_working;
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", adc_max);
			
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "camType";     
		keys_desc[ikey++] = "(R) cam type, interface, serial number, hardware & firmware version";     
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_CAMERATYPE);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "camSN";     
		keys_desc[ikey++] = "(R) cam serial number";     
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", _getCameraSerialNumber());
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "lastError";     
		keys_desc[ikey++] = "(R) last PCO SDK error";     
		if(_stricmp(cmd, key) == 0){
			m_pcoData->pcoErrorMsg[ERR_SIZE] = 0;
			ptr += sprintf_s(ptr, ptrMax - ptr, "[x%08x] [%s]\n", 
				m_pcoData->pcoError, m_pcoData->pcoErrorMsg
				);
			
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "msgLog";     
		keys_desc[ikey++] = "(R) log of last cmds executed ";     
		if(_stricmp(cmd, key) == 0){

			ptr += m_msgLog->dump(ptr, (int)(ptrMax - ptr), 0);
			m_msgLog->dumpPrint(true);			
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "msgLogFlush";     
		keys_desc[ikey++] = "flush log of last cmds executed ";     
		if(_stricmp(cmd, key) == 0){
			m_msgLog->flush(-1);			
			ptr += sprintf_s(ptr, ptrMax - ptr, "flushed ...");
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "dumpData";     
		keys_desc[ikey++] = "(R) hex dump of the stcPcoData";     
		if(_stricmp(cmd, key) == 0){

			print_hex_dump_buff(m_pcoData, sizeof(stcPcoData));
			ptr += sprintf_s(ptr, ptrMax - ptr, "dumped\n");
			
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "acqEnable";     
		keys_desc[ikey++] = "(R) acq enable signal status (BNC acq enbl in)";     
		if(_stricmp(cmd, key) == 0){
			int error;
			WORD wAcqEnableState;

//			error = PcoCheckError(__LINE__, __FILE__, PCO_GetAcqEnblSignalStatus(m_handle, &wAcquEnableState));
			_pco_GetAcqEnblSignalStatus(wAcqEnableState, error);
			
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", wAcqEnableState);
			
			return output;
		}


		
		
		//----------------------------------------------------------------
//		key = keys[ikey] = "hwioSignalsLin";
//		keys_desc[ikey++] = "(R) for DIMAX/EDGE only / get hw io signals";     
		key = keys[ikey] = "gethwioSignals";     
		keys_desc[ikey++] = "(R) get HWIO signals (only for some cameras (dimax, edge)";     
		if(_stricmp(cmd, key) == 0){
			int error, i, nrDesc, mask;

			if(!_isCapsDesc(capsHWIO)) 
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR - not allowed");
				return output;
			}

			_pco_GetHWIOSignalAll(error);
			if(error) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "SDK ERROR [%d]\n", error);
				return output;
			}
			
			nrDesc = m_pcoData->wNrPcoHWIOSignal;

			ptr += sprintf_s(ptr, ptrMax - ptr, "* number of HWIO signal descriptors[%d]\n", 
				 nrDesc);
			
			for(i=0; i< nrDesc; i++) 
			{
				WORD val;


/***************************************************************
wSignalDefinitions: Flags showing signal options:
- 0x01: Signal can be enabled/disabled
- 0x02: Signal is a status output
- 0x10: Signal function 1 has got parameter value
- 0x20: Signal function 2 has got parameter value
- 0x40: Signal function 3 has got parameter value
- 0x80: Signal function 4 has got parameter value
****************************************************************/
			    ptr += sprintf_s(ptr, ptrMax - ptr, "\n#=============== HWIO Signal Descriptor [%d] (max[%d])\n",i, nrDesc-1);
				
				ptr += sprintf_s(ptr, ptrMax - ptr, "* Signal Names:\n");
					for(int iSel = 0; iSel <4; iSel++)
					{
#ifdef __linux__
						char * ptrName = m_pcoData->stcPcoHWIOSignalDesc[i].szSignalName[iSel];
#else
						char * ptrName = m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[iSel];
#endif
						if(*ptrName) ptr += sprintf_s(ptr, ptrMax - ptr, "   [%s]  selected[%d]\n", ptrName, iSel);
					}
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n");

				ptr += sprintf_s(ptr, ptrMax - ptr,	"* OPTIONS of the HWIO signal for descriptor[%d]\n", i);

				val = m_pcoData->stcPcoHWIOSignalDesc[i].wSignalDefinitions;
				ptr += sprintf_s(ptr, ptrMax - ptr, "   def[0x%x]: ", val); 
                if((mask = (val & 0x01))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal can be enabled/disabled (0x%x)] ", mask);				
                if((mask = (val & 0x02))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal is a status output (0x%x)] ", mask);				
                if((mask = (val & 0x10))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal function 1 has got parameter value (0x%x)] ", mask);				
                if((mask = (val & 0x20))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal function 2 has got parameter value (0x%x)] ", mask);				
                if((mask = (val & 0x40))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal function 3 has got parameter value (0x%x)] ", mask);				
                if((mask = (val & 0x80))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal function 4 has got parameter value (0x%x)] ", mask);				
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 
					

/***************************************************************
wSignalTypes: Flags showing which signal type is available:
- 0x01: TTL
- 0x02: High Level TTL
- 0x04: Contact Mode
- 0x08: RS485 differential
***************************************************************/
				val = m_pcoData->stcPcoHWIOSignalDesc[i].wSignalTypes;
				ptr += sprintf_s(ptr, ptrMax - ptr, "   type[0x%x]: ", val); 
                if((mask = (val & 0x01))) ptr += sprintf_s(ptr, ptrMax - ptr, "[TTL (0x%x)] ", mask);				
                if((mask = (val & 0x02))) ptr += sprintf_s(ptr, ptrMax - ptr, "[High Level TTL (0x%x)] ", mask);				
                if((mask = (val & 0x04))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Contact Mode (0x%x)] ", mask);				
                if((mask = (val & 0x08))) ptr += sprintf_s(ptr, ptrMax - ptr, "[RS485 differential (0x%x)] ", mask);				
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 


/***************************************************************
wSignalPolarity: Flags showing which signal polarity can be selected:
- 0x01: Low level active
- 0x02: High Level active
- 0x04: Rising edge active
- 0x08: Falling edge active
***************************************************************/
				val = m_pcoData->stcPcoHWIOSignalDesc[i].wSignalPolarity;
				ptr += sprintf_s(ptr, ptrMax - ptr, "   pol[0x%x]: ", val); 
                if((mask = (val & 0x01))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Low level active (0x%x)] ", mask);				
                if((mask = (val & 0x02))) ptr += sprintf_s(ptr, ptrMax - ptr, "[High Level active (0x%x)] ", mask);				
                if((mask = (val & 0x04))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Rising edge active (0x%x)] ", mask);				
                if((mask = (val & 0x08))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Falling edge active (0x%x)] ", mask);				
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 

/***************************************************************
wSignalFilter: Flags showing the filter option:
- 0x01: Filter can be switched off (t > ~65ns)
- 0x02: Filter can be switched to medium (t > ~1us)
- 0x04: Filter can be switched to high (t > ~100ms)
Notes: the command will be rejected, if Recording State is [run]
***************************************************************/

				val = m_pcoData->stcPcoHWIOSignalDesc[i].wSignalFilter;
				ptr += sprintf_s(ptr, ptrMax - ptr, "   filter[0x%x]: ", val); 
                if((mask = (val & 0x01))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched off (t > ~65ns) (0x%x)] ", mask);				
                if((mask = (val & 0x02))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched to medium (t > ~1us) (0x%x)] ", mask);				
                if((mask = (val & 0x04))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched to high (t > ~100ms) (0x%x)] ", mask);				
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 


				WORD wSelected = m_pcoData->stcPcoHWIOSignal[i].wSelected;
				{ 
                    {
    			    ptr += sprintf_s(ptr, ptrMax - ptr,
						"\n"
						"* STATUS of the HWIO signal for descriptor[%d] selected[%d] [%s]\n",
						i, wSelected, 
#ifndef __linux__
						m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[wSelected]
#else
						m_pcoData->stcPcoHWIOSignalDesc[i].szSignalName[wSelected]
#endif

						);

/***************************************************************
Enabled Flags showing enable state of the signal
 0x00: Signal is off
 0x01: Signal is active
***************************************************************/
				        val = m_pcoData->stcPcoHWIOSignal[i].wEnabled;
				        ptr += sprintf_s(ptr, ptrMax - ptr, "   enabled[0x%x]: ", val); 
						ptr += sprintf_s(ptr, ptrMax - ptr, 
							val ? "[Signal is active]" : "[Signal is off]");				
						ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 

/***************************************************************
Type Flags showing which signal type is selected
 0x01: TTL
 0x02: High Level TTL
 0x04: Contact Mode
 0x08: RS485 differential
***************************************************************/
				        val = m_pcoData->stcPcoHWIOSignal[i].wType;
				        ptr += sprintf_s(ptr, ptrMax - ptr, "   type[0x%x]: ", val); 
                        if((mask = (val & 0x01))) ptr += sprintf_s(ptr, ptrMax - ptr, "[TTL (0x%x)] ", mask);				
                        if((mask = (val & 0x02))) ptr += sprintf_s(ptr, ptrMax - ptr, "[High Level TTL (0x%x)] ", mask);				
                        if((mask = (val & 0x04))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Contact Mode (0x%x)] ", mask);				
                        if((mask = (val & 0x08))) ptr += sprintf_s(ptr, ptrMax - ptr, "[RS485 differential (0x%x)] ", mask);				
				        ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 

/***************************************************************
Polarity Flags showing which signal polarity is selected
 0x01: High level active
 0x02: Low level active
 0x04: Rising edge active
 0x08: Falling edge active
***************************************************************/
				        val = m_pcoData->stcPcoHWIOSignal[i].wPolarity;
				        ptr += sprintf_s(ptr, ptrMax - ptr, "   pol[0x%x]: ", val); 
                        if((mask = (val & 0x01))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Low level active (0x%x)] ", mask);				
                        if((mask = (val & 0x02))) ptr += sprintf_s(ptr, ptrMax - ptr, "[High Level active (0x%x)] ", mask);				
                        if((mask = (val & 0x04))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Rising edge active (0x%x)] ", mask);				
                        if((mask = (val & 0x08))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Falling edge active (0x%x)] ", mask);				
				        ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 

/***************************************************************
FilterSetting Flags showing the filter option which is selected
 0x01: Filter can be switched off (t > ~65ns)
 0x02: Filter can be switched to medium (t > ~1 u)
 0x04: Filter can be switched to high (t > ~100ms)
Selected In case the HWIOSignaldescription shows more than one SignalNames, this parameter can be
used to select a different signal, e.g. Status Busy or Status Exposure.
***************************************************************/
				        val = m_pcoData->stcPcoHWIOSignal[i].wFilterSetting;
				        ptr += sprintf_s(ptr, ptrMax - ptr, "   filter[0x%x]: ", val); 
                        if((mask = (val & 0x01))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched off (t > ~65ns) (0x%x)] ", mask);				
                        if((mask = (val & 0x02))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched to medium (t > ~1us) (0x%x)] ", mask);				
                        if((mask = (val & 0x04))) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched to high (t > ~100ms) (0x%x)] ", mask);				
				        ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 


                    }  // if signame
				}   // for wselec
					//print_hex_dump_buff(&m_pcoData->stcPcoHWIOSignalDesc[i].szSignalName[0][0], 24*4);
			} // for descr

			
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "hwioSignals";     
		keys_desc[ikey++] = "(R) for DIMAX/EDGE only / get hw io signals";     
		if(_stricmp(cmd, key) == 0){
			int error, i;

			_pco_GetHWIOSignalAll(error);
			if(error) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR [%d]", error);
				return output;
			}
			//ptr += sprintf_s(ptr, ptrMax - ptr, "signals [%d] [%d]\n", m_pcoData->wNrPcoHWIOSignal0, m_pcoData->wNrPcoHWIOSignal);
			
			for(i=0; i< m_pcoData->wNrPcoHWIOSignal; i++) {
				ptr += sprintf_s(ptr, ptrMax - ptr, 
					"name[%s] [%s] [%s] [%s] idx[%d] num[%d] \n"
					"-def:     def[0x%x] type[0x%x] pol[0x%x] filt[0x%x]\n"
					"-sig:    enab[0x%x] type[0x%x] pol[0x%x] filt[0x%x] sel[0x%x]\n\n", 
					
#ifndef __linux__					
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[0],
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[1],
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[2],
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[3],
#else
					m_pcoData->stcPcoHWIOSignalDesc[i].szSignalName[0],
					m_pcoData->stcPcoHWIOSignalDesc[i].szSignalName[1],
					m_pcoData->stcPcoHWIOSignalDesc[i].szSignalName[2],
					m_pcoData->stcPcoHWIOSignalDesc[i].szSignalName[3],
#endif



					i, 
					m_pcoData->stcPcoHWIOSignal[i].wSignalNum,

					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalDefinitions,
					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalTypes,
					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalPolarity,
					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalFilter,

					m_pcoData->stcPcoHWIOSignal[i].wEnabled,
					m_pcoData->stcPcoHWIOSignal[i].wType,
					m_pcoData->stcPcoHWIOSignal[i].wPolarity,
					m_pcoData->stcPcoHWIOSignal[i].wFilterSetting,
					m_pcoData->stcPcoHWIOSignal[i].wSelected
					);
			}

			
			return output;
		}



/************************************************************************************************************************
  WORD  wSignalNum;                    // Index for strSignal (0,1,2,3,)
  WORD  wEnabled;                      // Flag shows enable state of the signal (0: off, 1: on)
  WORD  wType;                         // Selected signal type (1: TTL, 2: HL TTL, 4: contact, 8: RS485, 80: TTL-A/GND-B)
  WORD  wPolarity;                     // Selected signal polarity (1: H, 2: L, 4: rising, 8: falling)
  WORD  wFilterSetting;                // Selected signal filter (1: off, 2: med, 4: high) // 12
  WORD  wSelected;                     // Select signal (0: standard signal, >1 other signal)
************************************************************************************************************************/
  

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "sethwioSignals";     
		keys_desc[ikey++] = "(W) set HWIO signals (only for some cameras (dimax, edge)";     
		if(_stricmp(cmd, key) == 0){
			int error;

			if(!_isCapsDesc(capsHWIO)) 
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR - not allowed");
				return output;
			}

			if(tokNr != 6)
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, 
					"ERROR - invalid nr of parameters [%d]\n"
					"Parameters: <SignalNum> <Enabled> <Type> <Polarity> <FilterSetting> <Selected> (values in decimal, -1 ignored)"
					, tokNr);
				return output;
			}

			_pco_GetHWIOSignalAll(error);
			if(error) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR [%d]", error);
				return output;
			}

			int iSignalNum = atoi(tok[1]);
			int iEnabled = atoi(tok[2]);
			int iType = atoi(tok[3]);
			int iPolarity = atoi(tok[4]);
			int iFilterSetting = atoi(tok[5]);
			int iSelected = atoi(tok[6]);
			WORD wVal, wMask;
			int nrBits;
			
			int nrDesc = m_pcoData->wNrPcoHWIOSignal;

			if((iSignalNum< 0) ||(iSignalNum > nrDesc))
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR invalid SignalNum[%d] (0 - %d)", iSignalNum, nrDesc -1);
				return output;
			}
			m_pcoData->stcPcoHWIOSignal[iSignalNum].wSignalNum = (WORD) iSignalNum;
	
			if(iEnabled >= 0)
			{
				m_pcoData->stcPcoHWIOSignal[iSignalNum].wEnabled = iEnabled ? 1 : 0;
			}

			if(iType >= 0)
			{
				wVal = (WORD) iType;
				wMask = m_pcoData->stcPcoHWIOSignalDesc[iSignalNum].wSignalTypes;
				if( ((nrBits = getNrBitsOn(wVal)) != 1) || ((wVal & wMask) != wVal))
				{
					ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR invalid Type[%d] nrBits[%d] mask[%d]", wVal, nrBits, wMask);
					_pco_GetHWIOSignalAll(error);
					return output;
				}
				m_pcoData->stcPcoHWIOSignal[iSignalNum].wPolarity = wVal;
			}

			if(iPolarity >= 0)
			{
				wVal = (WORD) iPolarity;
				wMask = m_pcoData->stcPcoHWIOSignalDesc[iSignalNum].wSignalPolarity;
				if( ((nrBits = getNrBitsOn(wVal)) != 1) || ((wVal & wMask) != wVal))
				{
					ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR invalid Polarity[%d] nrBits[%d] mask[%d]", wVal, nrBits, wMask);
					_pco_GetHWIOSignalAll(error);
					return output;
				}
				m_pcoData->stcPcoHWIOSignal[iSignalNum].wPolarity = wVal;
			}

			if(iFilterSetting >= 0)
			{
				wVal = (WORD) iFilterSetting;
				wMask = m_pcoData->stcPcoHWIOSignalDesc[iSignalNum].wSignalFilter;
				if( ((nrBits = getNrBitsOn(wVal)) != 1) || ((wVal & wMask) != wVal))
				{
					ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR invalid FilterSetting[%d] nrBits[%d] mask[%d]", wVal, nrBits, wMask);
					_pco_GetHWIOSignalAll(error);
					return output;
				}
				m_pcoData->stcPcoHWIOSignal[iSignalNum].wFilterSetting = wVal;

			}

			if(iSelected >= 0)
			{
                char cName;
                
#ifndef __linux__
                cName = *m_pcoData->stcPcoHWIOSignalDesc[iSignalNum].strSignalName[iSelected];
#else
                cName = *m_pcoData->stcPcoHWIOSignalDesc[iSignalNum].szSignalName[iSelected];
#endif                
                
				if( (iSelected > 3) || (cName==0))
				{
					ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR invalid Selected[%d]", iSelected);
					_pco_GetHWIOSignalAll(error);
					return output;
				}
				m_pcoData->stcPcoHWIOSignal[iSignalNum].wSelected = (WORD) iSelected;
			}
	
			_pco_SetHWIOSignal(iSignalNum,error);
			
			if(error)
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "SDK error [%d]", error);
			} 
			else
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "OK");
			}
			
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "dumpData";     
		keys_desc[ikey++] = "(R) hex dump of the stcPcoData";     
		if(_stricmp(cmd, key) == 0){

			print_hex_dump_buff(m_pcoData, sizeof(stcPcoData));
			ptr += sprintf_s(ptr, ptrMax - ptr, "dumped\n");
			
			return output;
		}



		//----------------------------------------------------------------------------------------------------------
#ifndef __linux__
		key = keys[ikey] = "winMem";     
		keys_desc[ikey++] = "(R) read win memory";     
		if(_stricmp(cmd, key) == 0){
			const float gB = 1024. * 1024. * 1024.;
			MEMORYSTATUSEX statex;
			statex.dwLength = sizeof(statex);
			
			GlobalMemoryStatusEx(&statex);

			ptr += sprintf_s(ptr, ptrMax - ptr, "               dwLength [%u]\n", statex.dwLength);
			ptr += sprintf_s(ptr, ptrMax - ptr, "           dwMemoryLoad [%u]\n", statex.dwMemoryLoad);
			ptr += sprintf_s(ptr, ptrMax - ptr, "           ullTotalPhys [%g GB][%llu]\n", statex.ullTotalPhys/gB, statex.ullTotalPhys);
			ptr += sprintf_s(ptr, ptrMax - ptr, "           ullAvailPhys [%g GB][%llu]\n", statex.ullAvailPhys/gB, statex.ullAvailPhys);

			ptr += sprintf_s(ptr, ptrMax - ptr, "       ullTotalPageFile [%g GB][%llu]\n", statex.ullTotalPageFile/gB, statex.ullTotalPageFile);
			ptr += sprintf_s(ptr, ptrMax - ptr, "       ullAvailPageFile [%g GB][%llu]\n", statex.ullAvailPageFile/gB, statex.ullAvailPageFile);

			ptr += sprintf_s(ptr, ptrMax - ptr, "        ullTotalVirtual [%g GB][%llu]\n", statex.ullTotalVirtual/gB, statex.ullTotalVirtual);
			ptr += sprintf_s(ptr, ptrMax - ptr, "        ullAvailVirtual [%g GB][%llu]\n", statex.ullAvailVirtual/gB, statex.ullAvailVirtual);
			ptr += sprintf_s(ptr, ptrMax - ptr, "ullAvailExtendedVirtual [%g GB][%llu]\n", statex.ullAvailExtendedVirtual/gB, statex.ullAvailExtendedVirtual);


			return output;
		}
#endif

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "comment";     
		keys_desc[ikey++] = "(W) print timestamp & comment in the screen";     
		if(_stricmp(cmd, key) == 0){
			char *comment = str_trim(cmdBuffAux + strlen(cmd));

			ptr += sprintf_s(ptr, ptrMax - ptr, _sprintComment(false, comment) );

			DEB_TRACE() << output ;
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "nameInfo";     
		keys_desc[ikey++] = "(R) get name info";     
		if(_stricmp(cmd, key) == 0){
			int error;

			ptr += sprintf_s(ptr, ptrMax - ptr, "[" );
			_pco_GetInfoString(0, ptr, (int) (ptrMax - ptr), error);
			ptr += strlen(ptr);

			ptr += sprintf_s(ptr, ptrMax - ptr, "] [" );
			_pco_GetInfoString(1, ptr, (int) (ptrMax - ptr), error);
			ptr += strlen(ptr);

			ptr += sprintf_s(ptr, ptrMax - ptr, "] [" );
			_pco_GetInfoString(2, ptr, (int) (ptrMax - ptr), error);
			ptr += strlen(ptr);

			ptr += sprintf_s(ptr, ptrMax - ptr, "]" );

			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "sdkRelease";     
		keys_desc[ikey++] = "(R) sdk release";     
		if(_stricmp(cmd, key) == 0){

        const char* release ;

#ifdef __linux__
        release =  PCO_SDK_LIN_RELEASE;
#else
        release =  PCO_SDK_WIN_RELEASE;
#endif    

			ptr += sprintf_s(ptr, ptrMax - ptr, release );

			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "binInfo";     
		keys_desc[ikey++] = "(R) binning info";     
		if(_stricmp(cmd, key) == 0){
			int err;
			_pco_GetBinningInfo(ptr, (int) (ptrMax - ptr), err);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "roiInfo";     
		keys_desc[ikey++] = "(R) roi info";     
		if(_stricmp(cmd, key) == 0){
			int err;
			_pco_GetRoiInfo(ptr, (int) (ptrMax - ptr), err);
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "RecorderForcedFifo";     
		keys_desc[ikey++] = "(RW) force recorder mode to FIFo (0/1)";
		if(_stricmp(cmd, key) == 0){
			int forced;

			if(tokNr == 1) {
				forced = atoi(tok[1]);
				setRecorderForcedFifo(forced);
			}

			getRecorderForcedFifo(forced);
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", forced);

			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "NrEvents";     
		keys_desc[ikey++] = "(RW) set NrEvents (1-4)";
		if(_stricmp(cmd, key) == 0){
			int nrEvents;

			if(tokNr == 1) {
				nrEvents = atoi(tok[1]);
				setNrEvents(nrEvents);
			}

			getNrEvents(nrEvents);
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", nrEvents);

			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		// this must be the last cmd TALK / END
		//----------------------------------------------------------------------------------------------------------

		key = keys[ikey] = "?";     
		keys_desc[ikey++] = "(R) this help / list of the talk cmds";     
		if(_stricmp(cmd, key) == 0){
			int i, j, ikeyMax;
			const char *ptri, *ptrj;
			size_t len = 0;

			ikeyMax = ikey;

			for(i = 0; i < ikeyMax; i++) 
			{
				for(j = i; j < ikeyMax; j++) 
				{
					ptri = keys[i]; ptrj = keys[j];
					if(_stricmp(ptri,ptrj) > 0)
					{
						keys[j] = ptri;
						keys[i] = ptrj;
						ptri = keys_desc[i];
						keys_desc[i] = keys_desc[j];
						keys_desc[j] = ptri;
					}
				}
				if(len < strlen(keys[i])) len = strlen(keys[i]);
			}

			for(i = 0; i < ikeyMax; i++) 
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "%*s - %s\n", -(int) len, keys[i], keys_desc[i]);
			}
			ptr += sprintf_s(ptr, ptrMax - ptr, "--- nrCmds[%d][%d]\n", ikeyMax, NRCMDS);
			return output;
		}

		sprintf_s(ptr, ptrMax - ptr, "ERROR unknown cmd [%s]", cmd);
		return output;
}





//====================================================================
// utils
//====================================================================

#define LEN_LINE_BUFF	128
#define BYTES_PER_LINE		16
#define OFFSET_COL_HEX1	(4 + 1 + 3)
#define OFFSET_COL_HEX2	(OFFSET_COL_HEX1 + (8 * 3) + 2)
#define OFFSET_COL_ASC1	(OFFSET_COL_HEX2 + (8 * 3) + 3)
#define OFFSET_COL_ASC2	(OFFSET_COL_ASC1 + 8 + 1)

//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *nibble_to_hex(char *s, BYTE nibble)
{
  nibble &= 0x0f;
  *s = (nibble <= 9) ?  '0' + nibble : 'a' + nibble - 10;
  return s+1;
}



//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *byte_to_hex(char *s, BYTE byte)
{
  s = nibble_to_hex(s, byte >> 4);
  return nibble_to_hex(s, byte);;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *word_to_hex(char *s, WORD word)
{
  s = byte_to_hex(s, word >> 8);
  return byte_to_hex(s, (BYTE) word);
  
}


//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *dword_to_hex(char *s, DWORD dword)
{
  s = word_to_hex(s, dword >> 16);
  return word_to_hex(s, (WORD) dword);
  
}


//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *_hex_dump_bytes(void *obj, size_t lenObj, char *buff, size_t lenBuff) {
	char *ptr = buff;
	char *ptrMax = buff + lenBuff;
	char *ptrObj = (char *) obj;

	memset(buff, ' ', lenBuff);

	for(unsigned int i=0; i< lenObj; i++) {

		if(ptr+3 > ptrMax) break;
		ptr = byte_to_hex(ptr, *ptrObj ++);
		if(ptr+1 == ptrMax) break;
		ptr++;

	}

	*ptr = 0; 
	return buff;

}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
const char *hex_dump_line(void *buff, size_t len, size_t *nr, WORD *offset) {

	static char line_buff[LEN_LINE_BUFF];

	char *s;
	BYTE *ptr_buff;
	BYTE *ptr_buff0;
	int i, nr1, nr2;

	ptr_buff = (BYTE *) buff;
	ptr_buff0 = ptr_buff;

	s = line_buff;


	if(len <= 0) {
		strcpy_s(s,LEN_LINE_BUFF,"<empty>");
		return line_buff;
	}

	memset(line_buff, ' ', LEN_LINE_BUFF);

	*nr = (len < BYTES_PER_LINE) ? len : BYTES_PER_LINE;

	s = word_to_hex(s, *offset);
	*offset += (WORD) *nr;
	*s = ':';
	s = line_buff + OFFSET_COL_HEX1;

	if(*nr <= BYTES_PER_LINE / 2) {
		nr1 = (int) *nr;
		nr2 = -1;
	} else {
		nr1 = BYTES_PER_LINE / 2;
		nr2 = ((int) *nr) - nr1;
	}

	for(i = 0; i < nr1; i++) {
		s = byte_to_hex(s, *ptr_buff++);
		s++;
	}

	if(nr2 >0 ){
		*s = '-';
		s = line_buff + OFFSET_COL_HEX2;
		for(i = 0; i < nr2; i++) {
			s = byte_to_hex(s, *ptr_buff++);
			s++;
		}
	}	

	ptr_buff = ptr_buff0;
	s = line_buff + OFFSET_COL_ASC1;
	for(i = 0; i < nr1; i++) {
		*s++ = (isprint(*ptr_buff)) ? *ptr_buff : '.';
		ptr_buff++;
	}

	if(nr2 >0 ){
		s = line_buff + OFFSET_COL_ASC2;
		for(i = 0; i < nr2; i++) {
			*s++ = (isprint(*ptr_buff)) ? *ptr_buff : '.';
			ptr_buff++;
		}
	}	
	
	*s = 0;

	return line_buff;

}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void print_hex_dump_buff(void *ptr_buff, size_t len) {
	WORD offset = 0;
	size_t nr = 0;
	BYTE * ptr = (BYTE *) ptr_buff;
	;
	
	printf("dump buff / len: %ld\n", len);
	
	while(len > 0) {
		printf("%s\n", hex_dump_line(ptr, len, &nr, &offset));
		len -= nr;
		ptr += nr;
	}

}


//--------------------------------------------------------------------
//--------------------------------------------------------------------


ringLog::ringLog(int size) {
        buffer = new struct data [size];
        m_capacity = m_capacity_max = size;
        m_size = 0;
        m_head = 0;
}
void ringLog::flush(int capacity) {
		if((capacity > 0) &&(capacity <= m_capacity_max))
			m_capacity = capacity;
        m_size = 0;
        m_head = 0;
}

ringLog::~ringLog() {
        delete buffer;
}

int ringLog::add(const char *s) {

    struct data *ptr;
    int offset;
    
    if (m_size < m_capacity){
            offset = (m_head + m_size) % m_capacity;
            ptr = buffer + offset;
            m_size++;
    } else {
            ptr = buffer + m_head;
            m_head = (m_head + 1) % m_capacity;
            m_size = m_capacity;
    }
    
    ptr->timestamp = getTimestamp();

	long long l = strlen(s);

	strncpy_s(ptr->str, RING_LOG_BUFFER_SIZE, s, RING_LOG_BUFFER_SIZE-1);
    return m_size;

}


void ringLog::dumpPrint(bool direction) {

        static char timeline[128];
        struct data *ptr;
        int offset;
        time_t ltime;
        struct tm today;
        const char *fmt = "%Y/%m/%d %H:%M:%S";
        int i;
        
        for(i=0; i< m_size; i++) {
        
                offset = direction ? i : m_size -1 -i;
                
                ptr = buffer + (m_head + offset) % m_capacity;
                ltime = ptr->timestamp;

				localtime_s( &today, &ltime );

                strftime(timeline, 128, fmt, &today);
                
                printf("%s> %s\n", timeline, ptr->str);
        }
        
}

int ringLog::dump(char *s, int lgMax, bool direction) {

        //static char timeline[128];
        struct data *ptr;
        int offset;
        time_t ltime;
        struct tm today;
        const char *fmt = "%Y/%m/%d %H:%M:%S";
		int linMax = 25 + RING_LOG_BUFFER_SIZE;
        int i;
		
        int lg = 0;
		//char *ptrOut = s;

        for(i=0; (i< m_size) && ((lgMax - lg) > linMax); i++) {
        
                offset = direction ? i : m_size -1 -i;
                
                ptr = buffer + (m_head + offset) % m_capacity;
                ltime = ptr->timestamp;

				localtime_s( &today, &ltime );

                lg += (int) strftime(s + lg, lgMax - lg, fmt, &today);
                lg += sprintf_s(s + lg, lgMax - lg, "> %s\n", ptr->str);
        }
        
		return lg;
}

//=========================================================================================================
//=========================================================================================================

unsigned long long Camera::_getDebug(unsigned long long mask = ULLONG_MAX){

		return m_pcoData->debugLevel & mask;

}
//=========================================================================================================
//=========================================================================================================

const char *Camera::_checkLogFiles(bool firstCall) {
	const char *logFiles[] = {
		"C:\\ProgramData\\pco\\SC2_Cam.log", 
		"C:\\ProgramData\\pco\\PCO_CDlg.log", 
		"C:\\ProgramData\\pco\\PCO_Conv.log",
        "C:\\ProgramData\\pco\\me4_memlog_end.log",
		NULL};
	const char **ptr = logFiles;
	const char *logOn = "\n\n"		
		"###############################################################################\n"
		"###############################################################################\n"
		"###############################################################################\n"
		"###                                                                         ###\n"
		"###                           !!!  ATTENTION !!!                            ###\n"
		"###                                                                         ###\n"
		"###                     THE PCO LOG FILES ARE ENABLED                       ###\n"
		"###                                                                         ###\n"
		"###                 this option is ONLY for DEBUG & TESTS                   ###\n"
		"###                                                                         ###\n"
		"###                   it downgrades the acquisition time                    ###\n"
		"###                                                                         ###\n"
		"###                                                                         ###\n"
		"###     to DISABLE it:                                                      ###\n"
		"###          * stop the device server                                       ###\n"
		"###          * open the directory C:\\ProgramData\\pco\\                       ###\n"
		"###          * rename all the .log files as .txt                            ###\n"
		"###          * start again the device server                                ###\n"
		"###                                                                         ###\n"
		"###############################################################################\n"
		"###############################################################################\n"
		"###############################################################################\n\n\n";

	const char *logOff = "";
	struct stat fileStat;
	int error;
	bool found = false;


	if(firstCall) {
		while(*ptr != NULL) {
			error = stat(*ptr, &fileStat);
			//printf("----------- [%d][%s]\n", error, *ptr);
 			found |= !error;
			ptr++;
		}
        m_pcoData->pcoLogActive = found;
	}

    return m_pcoData->pcoLogActive ? logOn : logOff;	
};

//====================================================================
//====================================================================

#define INFO_BUFFER_SIZE 1024
//void printError( TCHAR* msg );

char * _getComputerName(char *infoBuff, DWORD  bufCharCount  )
{

#ifndef __linux__
  // Get and display the name of the computer. 
  if( !GetComputerName( infoBuff, &bufCharCount ) )
	  sprintf_s(infoBuff, bufCharCount, "ERROR: GetComputerName" ); 
#else
    char hostname[1024];
    gethostname(hostname, 1024);

    sprintf_s(infoBuff, bufCharCount, hostname ); 
#endif

  return infoBuff ;
}

char * _getUserName(char *infoBuff, DWORD  bufCharCount  )
{
 
#ifndef __linux__
  // Get and display the user name. 
  if( !GetUserName( infoBuff, &bufCharCount ) )
	  sprintf_s(infoBuff, bufCharCount, "ERROR: GetUserName" ); 
#else
    int err = getlogin_r(infoBuff, bufCharCount);
    if(err)
    {
	  sprintf_s(infoBuff, bufCharCount, "ERROR getlogin_r" ); 
    }
#endif

  return infoBuff ;
}

#ifdef __linux__
char * _getDllPath(const char* pzFileName, char *path, size_t strLen)
{
    *path = 0;
    return path;
}


#else
//====================================================================
//====================================================================
#define PCOSDK_FILENAME "sc2_cam.dll"

#include <winver.h>
#pragma comment(lib, "version.lib")

#define LEN_DRIVE	7
#define LEN_DIR		MAX_PATH



//====================================================================
//====================================================================
char * _getDllPath(const char* pzFileName, char *path, size_t strLen)
{
	errno_t err;

	char drive[LEN_DRIVE+1];
	char dir[LEN_DIR+1];
	char _pathFn[MAX_PATH+1];
	char _pathFnInstall[MAX_PATH+1];
	char *ptr;
	size_t nr;
	FILE *stream;

	*path = 0;
	ptr = path;

#ifndef ENABLE_GETDLLPATH
	nr = sprintf_s(ptr, strLen-1,"_getDllPath DISABLED");
	return path;
#endif

	GetModuleFileName(GetModuleHandle(pzFileName), _pathFn, MAX_PATH);

	err = _splitpath_s(_pathFn, drive, LEN_DRIVE, dir, LEN_DIR, NULL, 0, NULL, 0);

	size_t l = strlen(dir);
	ptr = dir + l -2;
	while(*ptr != '\\') ptr--;
	*ptr = 0;

	ptr = path;
	err = _makepath_s(_pathFnInstall, MAX_PATH, drive, dir, FILENAME_INSTALL_VERSION, FILEEXT_INSTALL_VERSION);
	printf("----- path[%s] path1[%s] drive[%s] dir[%s]\n", _pathFn, _pathFnInstall, drive, dir);
	nr = sprintf_s(ptr, strLen-1,"%s\n", _pathFnInstall);
    ptr += nr;
	strLen -= nr;

	err  = fopen_s( &stream, _pathFnInstall, "r" );
#if 0
	if(err == 0) {
		if( fgets( path, strLen, stream ) == NULL){
			*ptr = 0;
		}
	}
#endif
	nr = fread(ptr, 1, strLen-1, stream); 
	ptr[nr] = 0;
    fclose( stream );
	return path;
}

//=======================================================================================================
//=======================================================================================================
HRESULT LastError()
{
		return -1;
}

HRESULT GetFileVersion(TCHAR *filename, VS_FIXEDFILEINFO *pvsf)
{
  DWORD dwHandle;
  DWORD cchver = GetFileVersionInfoSize(filename,&dwHandle);
  if(cchver == 0)
   return LastError();

  char* pver = new char[cchver];
  BOOL bret = GetFileVersionInfo(filename,0,cchver,pver);
  if(!bret)
  {
   delete[] pver;
   return LastError();
  }
  UINT uLen;
  void *pbuf;
  bret = VerQueryValue(pver,_T("\\"),&pbuf,&uLen);
  if(!bret)
  {
   delete[] pver;
   return LastError();
  }
  memcpy(pvsf,pbuf,sizeof(VS_FIXEDFILEINFO));
  delete[] pver;
  return S_OK;
}

int GetFileVerStructA(TCHAR* pzFileName, int* ima, int* imi, int* imb)
{
  HRESULT ret;

  VS_FIXEDFILEINFO vsf1;
  if(SUCCEEDED(ret=GetFileVersion(pzFileName,&vsf1)))
  {
   *ima=vsf1.dwFileVersionMS>>16;
   *imi=vsf1.dwFileVersionMS&0xFFFF;
   *imb=vsf1.dwFileVersionLS&0xFFFF;
   return 0;
  }
  return -1;
}

#endif
//=======================================================================================================
//=======================================================================================================

char * _getPcoSdkVersion(char *infoBuff, int strLen, char *lib)
{
	char *ptr = infoBuff;

#ifndef __linux__

	int ima, imi, imb;
	int nr;

	*ptr = 0;

	if(!GetFileVerStructA(lib, &ima, &imi, &imb))
	{
		nr = sprintf_s(ptr, strLen, "file[%s] ver[%d.%d.%d]\n", lib, ima, imi, imb);
	}
#else
		sprintf_s(ptr, strLen, PCO_SDK_LIN_RELEASE);

#endif
	return infoBuff ;
}



//====================================================================
//====================================================================
char * Camera::_camInfo(char *ptr, char *ptrMax, long long int flag)
{
	DEB_MEMBER_FUNCT();

	long long int lgbuff, lgbuffmax;
	lgbuffmax = ptrMax - ptr;

	if(flag & CAMINFO_BLOCK) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "\n* camInfo [begin]\n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... timestamp[%s]\n", getTimestamp(Iso));

	}


	//--------------- CAMERA TYPE
	if(flag & CAMINFO_CAMERATYPE) {

		ptr += sprintf_s(ptr, ptrMax - ptr, "* camera type \n");

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... cam_name[%s]\n", 
			_getCameraIdn());

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... dwSerialNumber[%d]\n", 
			_getCameraSerialNumber());

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wCamType[0x%x] [%s]\n", 
			_getCameraType(), _getCameraTypeStr()); 

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wCamSubType[0x%x] [%s]\n", 
			_getCameraSubType(), _getCameraSubTypeStr()); 

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wInterfaceType[0x%x] [%s]\n", 
			_getInterfaceType(), _getInterfaceTypeStr()); 
	
	}
	
	//--------------- general info
	if(flag & CAMINFO_GENERAL) {

		ptr += sprintf_s(ptr, ptrMax - ptr, "* general info \n");

		
		// OBSOLETE after sdk 120
		//ptr += sprintf_s(ptr, ptrMax - ptr, "* ... GigE IP [%d.%d.%d.%d]\n", 
		//	m_pcoData->ipField[0], m_pcoData->ipField[1], m_pcoData->ipField[2], m_pcoData->ipField[3]);
		
		double pixSizeX, pixSizeY;
		_get_PixelSize(pixSizeX, pixSizeY);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... pixelSize (um) [%g,%g] \n",  pixSizeX, pixSizeY);
	}
			
	//--------------- version
	if(flag & CAMINFO_VERSION) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "* lima version \n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "%s", m_pcoData->version);
	}
	//--------------- firmware

	if(flag & CAMINFO_FIRMWARE) {
		int err;
		_pco_GetFirmwareInfo(ptr, (int) (ptrMax - ptr), err);
		ptr += strlen(ptr);
	}

	//--------------- adc, pixelrate, 
	if(flag & CAMINFO_ADC) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "*** adc\n");
		int adc_working, adc_max;

		//int err = _pco_GetADCOperation(adc_working, adc_max);
		_pco_GetADCOperation(adc_working, adc_max);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ADC working[%d] max[%d]\n", 
				adc_working, adc_max);
	}

	if(flag & CAMINFO_PIXELRATE) {
		DWORD dwPixRate, dwPixRateNext ; int error, i;
		_pco_GetPixelRate(dwPixRate, dwPixRateNext, error);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* pixelRate\n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... pixelRate[%d](%g MHz)\n", dwPixRate, dwPixRate/1000000.);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    pixelRateRequested[%d](%g MHz) \n", 	dwPixRateNext, dwPixRateNext/1000000.);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    valid pixelRates "); 
		for(i=0; i<4; i++) 
		{
			dwPixRate = m_pcoData->stcPcoDescription.dwPixelRateDESC[i];
			if(dwPixRate){ptr += sprintf_s(ptr, ptrMax - ptr, " [%d]",dwPixRate);}  
		}	
		ptr += sprintf_s(ptr, ptrMax - ptr, "\n");
	}


	//--------------------- size, roi, ...
	if(flag & CAMINFO_ROI)
	{
		int err;	
		ptr += sprintf_s(ptr, ptrMax - ptr, "* size, roi, ... \n");
		unsigned int maxWidth, maxHeight,Xstep, Ystep, xMinSize, yMinSize; 
		getXYdescription(Xstep, Ystep, maxWidth, maxHeight, xMinSize, yMinSize); 
		WORD _wArmWidth, _wArmHeight, _wMaxWidth, _wMaxHeight;
		_pco_GetSizes(&_wArmWidth, &_wArmHeight, &_wMaxWidth, &_wMaxHeight, err);


		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... maxWidth=[%d] maxHeight=[%d] \n",  maxWidth,  maxHeight);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    Xstep=[%d] Ystep=[%d] (PCO ROI steps)\n",  Xstep,  Ystep);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... xMinSize=[%d] yMinSize=[%d] \n",  xMinSize,  yMinSize);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wXResActual=[%d] wYResActual=[%d] \n",  _wArmWidth,  _wArmHeight);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wXResMax=[%d] wYResMax=[%d] \n",  _wMaxWidth,  _wMaxHeight);


		unsigned int x0,x1,y0,y1;
		Roi limaRoi;
		int error;

		_pco_GetROI(limaRoi, error);
		_xlatRoi_lima2pco(limaRoi, x0,x1,y0,y1);
		

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... roi X[%d,%d] Y[%d,%d] size[%d,%d]\n",  
				x0, x1, y0, y1, x1-x0+1, y1-y0+1);

		Point top_left = limaRoi.getTopLeft();
		Point bot_right = limaRoi.getBottomRight();
		Size size = limaRoi.getSize();			

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... roiLima XY0[%d,%d] XY1[%d,%d] size[%d,%d]\n",  
						top_left.x, top_left.y,
						bot_right.x, bot_right.y,
						size.getWidth(), size.getHeight());

		unsigned int bytesPerPix; getBytesPerPixel(bytesPerPix);
		WORD bitsPerPix; getBitsPerPixel(bitsPerPix);


		long long imgSizePix = size.getWidth()* size.getHeight();
		long long imgSize = imgSizePix * bytesPerPix;
		double imgSizeMb =  imgSize/(1024.*1024.);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... imgSize[%lld px][%lld B][%g MB] \n",  
				imgSizePix, imgSize, imgSizeMb);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* bitsPerPix[%d] bytesPerPix[%d] \n",  bitsPerPix,  bytesPerPix);

		int iFrames;
		m_sync->getNbFrames(iFrames);
		double totalImgSizeMb =  imgSizeMb * iFrames;
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... m_sync->getNbFrames=[%d frames] totalSize[%g MB]\n", iFrames, totalImgSizeMb);

	}
	//--------------------- exp, coc, ...
	if(flag & CAMINFO_EXP){
		ptr += sprintf_s(ptr, ptrMax - ptr, "* exp, coc, ... \n");
		double _exposure, _delay;
		struct lima::HwSyncCtrlObj::ValidRangesType valid_ranges;
		m_sync->getValidRanges(valid_ranges);
		m_sync->getExpTime(_exposure);
		m_sync->getLatTime(_delay);

		DWORD _dwMinExposureStepDESC = m_pcoData->stcPcoDescription.dwMinExposureStepDESC;
		DWORD _dwMinDelayStepDESC =	m_pcoData->stcPcoDescription.dwMinDelayStepDESC;

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... exp[%g ms][%g s] delay[%g ms][%g s]\n", 
			_exposure*1.0e3,_exposure, 
			_delay*1.0e3, _delay); 

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    exp valid min[%g us] max[%g ms] step[%g us]\n", 
			valid_ranges.min_exp_time * 1.0e6, 
			valid_ranges.max_exp_time * 1.0e3,
			_dwMinExposureStepDESC * 1.0e-3  );

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    delay valid min[%g us] max[%g ms] step[%g us]\n", 
			valid_ranges.min_lat_time * 1.0e6, 
			valid_ranges.max_lat_time * 1.0e3,
			_dwMinDelayStepDESC * 1.0e-3 );
			

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    cocRunTime[%g] (s) frameRate[%g] (fps)\n",  
			m_pcoData->cocRunTime, m_pcoData->frameRate);
			
	}


    //------ DIMAX
	if( (flag & CAMINFO_DIMAX) && (_isCameraType(Dimax | Pco2k | Pco4k)) ){
		unsigned int bytesPerPix; getBytesPerPixel(bytesPerPix);
		WORD bitsPerPix; getBitsPerPixel(bitsPerPix);
		int segmentPco = m_pcoData->wActiveRamSegment;
		int segmentArr = segmentPco -1;

		ptr += sprintf_s(ptr, ptrMax - ptr, "*** DIMAX - 2k - 4k info \n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "* pagesInRam[%d] pixPerPage[%d] bitsPerPix[%d] ramGB[%.3g] bytesPerPix[%d] imgGB[%.3g]\n",  
			m_pcoData->dwRamSize, 
			m_pcoData->wPixPerPage, 
			bitsPerPix,
			(1.0e-9 * m_pcoData->dwRamSize) * m_pcoData->wPixPerPage *  bitsPerPix / 8.0,
			bytesPerPix,
			(1.0e-9 * m_pcoData->dwRamSize) * m_pcoData->wPixPerPage *  bytesPerPix);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* PcoActiveSegment=[%d]\n", segmentArr+1);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* m_pcoData->dwMaxFramesInSegment[%d]=[%d frames]\n", segmentArr, m_pcoData->dwMaxFramesInSegment[segmentArr]);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* m_pcoData->dwSegmentSize[%d]=[%d pages]\n", segmentArr, m_pcoData->dwSegmentSize[segmentArr]);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* m_pcoData->dwValidImageCnt[%d]=[%d]\n", segmentArr, m_pcoData->dwValidImageCnt[segmentArr]);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* m_pcoData->dwMaxImageCnt[%d]=[%d]\n", segmentArr, m_pcoData->dwMaxImageCnt[segmentArr]);


		int err;
		_pco_GetSegmentInfo(err);
		
		struct stcSegmentInfo *_stc;
		for(int iseg = 0; iseg <  PCO_MAXSEGMENTS; iseg++)
		{
			_stc = &m_pcoData->m_stcSegmentInfo[iseg];
			ptr += sprintf_s(ptr, ptrMax - ptr, "* segmentInformation seg[%d][%d] err[%d]\n", iseg + 1, _stc->iSegId, _stc->iErr);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*    bin[%d,%d] res[%d,%d] roiX[%d,%d] roiY[%d,%d]\n",
				_stc->wBinHorz, _stc->wBinVert,
				_stc->wXRes, _stc->wYRes,
				_stc->wRoiX0, _stc->wRoiX1, 
				_stc->wRoiY0, _stc->wRoiY1);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*    validImgCount[%d] maxImgCount[%d]\n",
				_stc->dwValidImageCnt,_stc->dwMaxImageCnt);
		}

		_pco_GetStorageMode_GetRecorderSubmode();
				
		ptr += sprintf_s(ptr, ptrMax - ptr, "* mode[%s] storage_mode[%d] recorder_submode[%d]\n", 
			m_pcoData->storage_str, m_pcoData->storage_mode, m_pcoData->recorder_submode);
		ptr += sprintf_s(ptr, ptrMax - ptr, 
			"* Acq: rec[%ld] xfer[%ld] recNow[%ld] recTout[%ld] (ms) [%s]\n",
			m_pcoData->msAcqRec, m_pcoData->msAcqXfer,  
			m_pcoData->msAcqTnow, m_pcoData->msAcqTout, 
			getTimestamp(Iso, m_pcoData->msAcqRecTimestamp));

	}


    //------ unsorted


	if(flag & CAMINFO_CAMERALINK) {
		if (_isInterfaceType(ifCameralinkAll)){ 
			ptr += sprintf_s(ptr, ptrMax - ptr, "*** CameraLink transfer parameters\n");
			ptr += sprintf_s(ptr, ptrMax - ptr, "*  settings[%s]\n", m_pcoData->sClTransferParameterSettings);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*  baudrate[%u] %g Kbps\n", m_pcoData->clTransferParam.baudrate, m_pcoData->clTransferParam.baudrate/1000.);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*clock freq[%u] %g MHz\n", m_pcoData->clTransferParam.ClockFrequency, m_pcoData->clTransferParam.ClockFrequency/1000000.);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*    CCline[%u]\n", m_pcoData->clTransferParam.CCline);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*DataFormat[0x%x]\n", m_pcoData->clTransferParam.DataFormat);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*  Transmit[%u]\n", m_pcoData->clTransferParam.Transmit);
		} else {
			ptr += sprintf_s(ptr, ptrMax - ptr, "*** CameraLink transfer parameters - NO CAMERALINK interface\n");
		}
	}

	if(flag & CAMINFO_UNSORTED) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "*** unsorted parameters\n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "* bMetaDataAllowed[%d] wMetaDataMode[%d] wMetaDataSize[%d] wMetaDataVersion[%d] \n",  
			m_pcoData->bMetaDataAllowed, m_pcoData->wMetaDataMode, m_pcoData->wMetaDataSize,  m_pcoData->wMetaDataVersion);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* wLUT_Identifier[x%04x] wLUT_Parameter [x%04x]\n",
			m_pcoData->wLUT_Identifier, m_pcoData->wLUT_Parameter);

		int alignment;
		const char *res;
		_pco_GetBitAlignment(alignment);
		res = alignment == 0 ? "[0][MSB]" : "[1][LSB]";
		ptr += sprintf_s(ptr, ptrMax - ptr, "* data bit alignment%s\n", res);

		WORD wTimeStampMode;
		const char *mode;
		int error;
		_pco_GetTimestampMode(wTimeStampMode, error);
		if(error) wTimeStampMode = 100;
		switch(wTimeStampMode) {
			case 0: mode = "no stamp in image"; break;
			case 1: mode = "BCD coded stamp in the first 14 pixel"; break;
			case 2: mode = "BCD coded stamp in the first 14 pixel + ASCII text"; break;
			case 3: mode = "ASCII text only"; break;
			default: mode = "unknown"; break;
		}

		ptr += sprintf_s(ptr, ptrMax - ptr, "* imageTimestampMode[%d][%s]\n", wTimeStampMode, mode);


		_pco_GetTemperatureInfo(error);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* temperature: CCD[%.1f]  CAM[%d]  PS[%d]\n", 
			m_pcoData->temperature.sCcd/10., 
			m_pcoData->temperature.sCam, 
			m_pcoData->temperature.sPower);

		ptr += sprintf_s(ptr, ptrMax - ptr, "*    cooling: min[%d]  max[%d]  default[%d]  Setpoint[%d]\n",  
			m_pcoData->temperature.sMinCoolSet, 
			m_pcoData->temperature.sMaxCoolSet,
			m_pcoData->temperature.sDefaultCoolSet,
			m_pcoData->temperature.sSetpoint);

	}


#if 0
//--------------------------- TODO / NEED to use _snprintf_s and control of error!
	int _nr;

	for(int ii=1; ii <= 1000; ii++) {
		_nr = _snprintf_s(ptr, ptrMax - ptr, _TRUNCATE, "************************************************************** %d\n",ii);
		if(_nr < 0) {return ptr; } else { ptr += _nr; }
	}

	if(flag & CAMINFO_LOG) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "*** log \n");
//------------------------------------ the function  m_log.c_str() FAILS!
		_nr = _snprintf_s(ptr, ptrMax - ptr, _TRUNCATE, "%s\n", m_log.c_str());
		if(_nr < 0) {return ptr; } else { ptr += _nr; }
	}
#endif





	if(flag & CAMINFO_BLOCK) {
		lgbuff = ptrMax - ptr;
		int used = int((100. * (lgbuffmax - lgbuff))/lgbuffmax);
		ptr += sprintf_s(ptr, ptrMax - ptr, "\n*** camInfo (buff free[%lld B] used[%d %%] size[%lld B]) [end]\n", lgbuff, used, lgbuffmax);
	}

	return ptr;
}



//===============================================================
//===============================================================
// include these definitions in the PreprocessorDefinitions
//    liblimapco -> properties -> c/c++ -> preprocessor -> preprocessor definitions
//
//    VS_PLATFORM=$(PlatformName)
//    VS_CONFIGURATION=$(ConfigurationName)
//===============================================================
//===============================================================

#define Win32				"Win32"
#define x64					"x64"
#define Release_Win7_Sync	"Release_Win7_Sync"
#define Release				"Release"

#ifndef VS_PLATFORM
#pragma message ( "--- VS_PLATFORM - UNDEFINED" )
#define VS_PLATFORM "undef"
#endif

#ifndef VS_CONFIGURATION
#pragma message ( "--- VS_CONFIGURATION - UNDEFINED" )
#define VS_CONFIGURATION "undef"
#endif 


#ifndef VS_PLATFORM
#pragma message ( "--- VS_PLATFORM - UNDEFINED" )
#define VS_PLATFORM "undef"
#endif

#ifndef VS_CONFIGURATION
#pragma message ( "--- VS_CONFIGURATION - UNDEFINED" )
#define VS_CONFIGURATION "undef"
#endif

char * _getVSconfiguration(char *infoBuff, DWORD  bufCharCount  )
{
	sprintf_s(infoBuff, bufCharCount, "platform[%s] configuration[%s]",  
		VS_PLATFORM,
		VS_CONFIGURATION); 
	return infoBuff ;
}
//====================================================================
//====================================================================

void Camera::_traceMsg(char *msg)
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "\n>>>  " << msg ;		

}

//====================================================================
//====================================================================
#define LEN_COMMENT 511
const char * Camera::_sprintComment(bool bAlways, const char *comment, const char *comment1, const char *comment2)
{
	DEB_MEMBER_FUNCT();

	static char buff[LEN_COMMENT+1];
//			"\n=================================================\n--- %s %s %s %s\n",

	
	sprintf_s(buff, LEN_COMMENT, "%s %s %s", comment, comment1, comment2);
	msgLog(buff);

	sprintf_s(buff, LEN_COMMENT, 
			"\n--- %s %s %s %s\n",
			getTimestamp(Iso), comment, comment1, comment2);

	if(bAlways)
	{
		DEB_ALWAYS() << buff;
	}

	return buff ;
}

//====================================================================
//====================================================================

/*************************************************************************
#define TIMESTAMP_MODE_BINARY           1
#define TIMESTAMP_MODE_BINARYANDASCII   2

SYSTEMTIME st;
int act_timestamp;
int shift;

if(camval.strImage.wBitAlignment==0)
  shift = (16-camval.strSensor.strDescription.wDynResDESC);
else
  shift = 0;

err=PCO_SetTimestampMode(hdriver,TIMESTAMP_MODE_BINARY); //Binary+ASCII

grab image to adr

time_from_timestamp(adr,shift,&st);
act_timestamp=image_nr_from_timestamp(adr,shift);
*************************************************************************/

int _get_imageNr_from_imageTimestamp(void *buf,int shift)
{
	unsigned short *b;
	unsigned short c;
	int y;
	int image_nr=0;

	b=(unsigned short *)(buf);
	y=100*100*100;

	for(;y>0;y/=100)
	{
		c=*b>>shift;
		image_nr+= (((c&0x00F0)>>4)*10 + (c&0x000F))*y;
		b++;
	}

	return image_nr;
}

int _get_time_from_imageTimestamp(void *buf,int shift,SYSTEMTIME *st)
{
	unsigned short *b;
	unsigned short c;
	int x,us;

	memset(st,0,sizeof(SYSTEMTIME));
	b=(unsigned short *)buf;

	//counter
	for(x=0;x<4;x++)
	{
		c=*(b+x)>>shift;
	}

	x=4;
	//year
	c=*(b+x)>>shift;
	st->wYear+=(c>>4)*1000;
	st->wYear+=(c&0x0F)*100;
	x++;

	c=*(b+x)>>shift;
	st->wYear+=(c>>4)*10;
	st->wYear+=(c&0x0F);
	x++;

	//month
	c=*(b+x)>>shift;
	st->wMonth+=(c>>4)*10;
	st->wMonth+=(c&0x0F);
	x++;


	//day
	c=*(b+x)>>shift;
	st->wDay+=(c>>4)*10;
	st->wDay+=(c&0x0F);
	x++;

	//hour
	c=*(b+x)>>shift;
	st->wHour+=(c>>4)*10;
	st->wHour+=(c&0x0F);
	x++;

	//min   
	c=*(b+x)>>shift;
	st->wMinute+=(c>>4)*10;
	st->wMinute+=(c&0x0F);
	x++;

	//sec   
	c=*(b+x)>>shift;
	st->wSecond+=(c>>4)*10;
	st->wSecond+=(c&0x0F);
	x++;

	//us   
	us=0;
	c=*(b+x)>>shift;
	us+=(c>>4)*100000;
	us+=(c&0x0F)*10000;
	x++;

	c=*(b+x)>>shift;
	us+=(c>>4)*1000;
	us+=(c&0x0F)*100;
	x++;

	c=*(b+x)>>shift;
	us+=(c>>4)*10;
	us+=(c&0x0F);
	x++;

	st->wMilliseconds=us/100;

	return 0;
}






//====================================================================
//====================================================================

void Camera::getRollingShutter(int &val)
{
	int error;
	DWORD dwRolling;

	if(!_isCameraType(Edge)) 
	{
		val = -1;
		return;
	} 
	_get_shutter_rolling_edge(dwRolling, error);
	val = dwRolling;

}


void Camera::setRollingShutter(int val)
{
	DEB_MEMBER_FUNCT();

	int error;
	DWORD dwRolling, dwRollingNew;

	dwRollingNew = (DWORD) val;

	if(!_isValid_rollingShutter(dwRollingNew))
	{
		DEB_ALWAYS() << "ERROR requested Rolling Shutter not allowed " << DEB_VAR1(dwRollingNew);
		error = -1;
		return;
	}

	_get_shutter_rolling_edge(dwRolling, error);
			

	if(dwRollingNew != dwRolling){
		_set_shutter_rolling_edge(dwRollingNew, error);
	}
}


void Camera::getRollingShutterInfo(std::string &o_sn) 
{
	char *ptr = buff;
	char *ptrMax = buff + sizeof(buff);
	int val;

	bool bRS = _isCapsDesc(capsRollingShutter);
    bool bGL = _isCapsDesc(capsGlobalShutter);
    bool bGR = _isCapsDesc(capsGlobalResetShutter);

	if( !(bRS || bGL || bGR)) 
	{
		ptr += sprintf_s(ptr, ptrMax - ptr, "Rolling Shutter is not allowed");
		o_sn = buff;
		return;
	}

	getRollingShutter(val);
	ptr += sprintf_s(ptr, ptrMax - ptr, "actual[%d] valid: ", val);

	if(bRS) ptr += sprintf_s(ptr, ptrMax - ptr, "rolling[%d] ", PCO_EDGE_SETUP_ROLLING_SHUTTER);
	if(bGL) ptr += sprintf_s(ptr, ptrMax - ptr, "global[%d] ", PCO_EDGE_SETUP_GLOBAL_SHUTTER);
	if(bGR) ptr += sprintf_s(ptr, ptrMax - ptr, "globalReset[%d] ",  PCO_EDGE_SETUP_GLOBAL_RESET);
	
	o_sn = buff;
	return;
}


//=================================================================================================
//=================================================================================================

void usElapsedTimeSet(long long &us0) {

#ifdef __linux__
    TIME_UTICKS tickNow;
    clock_gettime(CLOCK_REALTIME, &tickNow); 

    us0 = (long long)   ((tickNow.tv_sec) * 1000000. +
            (tickNow.tv_nsec) / 1000. );

#else
	TIME_UTICKS tick;   // A point in time
	TIME_UTICKS ticksPerSecond;
	QueryPerformanceFrequency(&ticksPerSecond); 
	QueryPerformanceCounter(&tick);

	double ticsPerUSecond = ticksPerSecond.QuadPart/1.0e6;
	us0 = (long long) (tick.QuadPart/ticsPerUSecond);
#endif
}

//------------------------------------------------
long long usElapsedTime(long long &us0) {

long long usDiff;

#ifdef __linux__
    TIME_UTICKS tickNow;
    clock_gettime(CLOCK_REALTIME, &tickNow); 

    usDiff = ((long long int)  ((tickNow.tv_sec) * 1000000. +
            (tickNow.tv_nsec) / 1000. )) - us0;

#else
	TIME_UTICKS tick;   // A point in time
	TIME_UTICKS ticksPerSecond;
	QueryPerformanceFrequency(&ticksPerSecond); 
	QueryPerformanceCounter(&tick);

	double ticsPerUSecond = ticksPerSecond.QuadPart/1.0e6;
	long long us = (long long) (tick.QuadPart/ticsPerUSecond);
    usDiff = us - us0;
#endif

	return usDiff;

}
//------------------------------------------------


long msElapsedTime(TIME_USEC &t0) {
    long msDiff;
	TIME_USEC tNow;


#ifdef __linux__
    long seconds, useconds;
    gettimeofday(&tNow, NULL);

    seconds  = tNow.tv_sec  - t0.tv_sec;
    useconds = tNow.tv_usec - t0.tv_usec;

    msDiff = long ( ((seconds) * 1000 + useconds/1000.0) + 0.5 );
#else
	_ftime64_s(&tNow);

	msDiff = (long)((tNow.time - t0.time)*1000) + (tNow.millitm - t0.millitm);
#endif

	return msDiff;
}

//------------------------------------------------


void msElapsedTimeSet(TIME_USEC &t0) {

#ifdef __linux__
    gettimeofday(&t0, NULL);
#else
	_ftime64_s(&t0);
#endif

}

//------------------------------------------------

double usElapsedTimeTicsPerSec() {
    double ticks = 0;
#ifdef __linux__
	TIME_UTICKS tick;   // A point in time
    clock_gettime(CLOCK_REALTIME, &tick); 

#else
	LARGE_INTEGER ticksPerSecond;
	QueryPerformanceFrequency(&ticksPerSecond); 
    ticks = (double) ticksPerSecond.QuadPart;
#endif
	return ticks;

}

//==========================================================================================================
//==========================================================================================================


