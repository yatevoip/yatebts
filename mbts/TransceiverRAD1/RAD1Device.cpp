/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


/*
	Compilation Flags

	SWLOOPBACK	compile for software loopback testing
*/ 


#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "Threads.h"
#include "RAD1Device.h"

#include <Logger.h>


using namespace std;

static int hexval (char ch)
{
	if ('0' <= ch && ch <= '9') {
		return ch - '0';
	}

	if ('a' <= ch && ch <= 'f') {
		return ch - 'a' + 10;
	}

	if ('A' <= ch && ch <= 'F') {
		return ch - 'A' + 10;
	}

	return -1;
}

static unsigned char * hex_string_to_binary(const char *string, int *lenptr)
{
	int sl = strlen (string);
	if (sl & 0x01){
//		fprintf (stderr, "%s: odd number of chars in <hex-string>\n", prog_name);
		return 0;
	}

	int len = sl / 2;
	*lenptr = len;
	unsigned char *buf = new unsigned char [len];

	for (int i = 0; i < len; i++){
		int hi = hexval (string[2 * i]);
		int lo = hexval (string[2 * i + 1]);
		if (hi < 0 || lo < 0){
//			fprintf (stderr, "%s: invalid char in <hex-string>\n", prog_name);
			delete [] buf;
			return 0;
		}
		buf[i] = (hi << 4) | lo;
	}
	return buf;
}

static bool i2c_write(rnrad1Core& core, int i2c_addr, const char *hex_string)
{
	int len = 0;
	unsigned char *buf = hex_string_to_binary (hex_string, &len);
	if (buf == 0) {
		return false;
	}
	return core.writeI2c(i2c_addr, buf, len);
}

static std::string i2c_read(rnrad1Core& core, int i2c_addr, int len)
{
	unsigned char *buf = new unsigned char [len];
	bool result = core.readI2c(i2c_addr, buf, len);
	if (!result) {
		return "";
	}

	char hex[64];
	for (int i = 0; i < len; i++){
		sprintf (hex+(2*i), "%02x", buf[i]);
	}

	return std::string(hex);
}

static unsigned int hex2dec(std::string hex)
{
	unsigned int dec;
	std::stringstream tempss;

	tempss << std::hex << hex;
	tempss >> dec;

	return dec;
}

static unsigned char* write_it(unsigned v, unsigned char *s) {
  s[0] = (v>>16) & 0x0ff;
  s[1] = (v>>8) & 0x0ff;
  s[2] = (v) & 0x0ff;
  return s;
}


const double RAD1Device::LO_OFFSET = 6.0e6;

const double RAD1Device::masterClockRate = (double) 52.0e6;

bool RAD1Device::compute_regs(double freq,
			      unsigned *R,
			      unsigned *control,
			      unsigned *N,
			      double *actual_freq) 
{
  if (freq < 1.2e9) {
	DIV2 = 1;
	freq_mult = 2;
	CP1 = 7;
	CP2 = 7;
  }
  else {
	DIV2 = 0;
	freq_mult = 1;
	// This default value matches the RAD1r3.
	CP1 = 7;
	// This default value matches the RAD1r3.
	CP2 = 7;
  }

  double phdet_freq = 13.0e6/(double) R_DIV;
  int desired_n = (int) round(freq*freq_mult/phdet_freq);
  *actual_freq = desired_n * phdet_freq/freq_mult;
  
  float B = floor(desired_n/16);
  float A = desired_n - 16*B;

  unsigned B_DIV = int(B);
  unsigned A_DIV = int(A);

  //printf("B: %f, A: %f, B_DIV: %u, A_DIV: %u\n",B,A,B_DIV,A_DIV);

  if (B < A) return false;
  *R = (R_RSV<<22) | 
    (BSC << 20) | 
    (TEST << 19) | 
    (LDP << 18) | 
    (ABP << 16) | 
    (R_DIV << 2);
  *control = (P<<22) | 
    (PD<<20) | 
    (CP2 << 17) | 
    (CP1 << 14) | 
    (PL << 12) | 
    (MTLD << 11) | 
    (CPG << 10) | 
    (CP3S << 9) | 
    (PDP << 8) | 
    (MUXOUT << 5) | 
    (CR << 4) | 
    (PC << 2);
  *N = (DIVSEL<<23) | 
    (DIV2<<22) | 
    (CPGAIN<<21) | 
    (B_DIV<<8) | 
    (N_RSV<<7) | 
    (A_DIV<<2);
  return true;
}


bool RAD1Device::tx_setFreq(double freq, double *actual_freq) 
{
  unsigned R, control, N;
  if (!compute_regs(freq, &R, &control, &N, actual_freq)) return false;
  if (R==0) return false;
 
  unsigned char result[3]; 
  writeLock.lock();
  m_uTx->writeSpi(0,SPI_ENABLE_TX_A,SPI_FMT_MSB | SPI_FMT_HDR_0,
		    write_it((R & ~0x3) | 1, result),3);
  m_uTx->writeSpi(0,SPI_ENABLE_TX_A,SPI_FMT_MSB | SPI_FMT_HDR_0,
		    write_it((control & ~0x3) | 0, result),3);
  usleep(10000);
  m_uTx->writeSpi(0,SPI_ENABLE_TX_A,SPI_FMT_MSB | SPI_FMT_HDR_0,
		    write_it((N & ~0x3) | 2,result),3);
  writeLock.unlock();
  
  if (m_uTx->readIO() & PLL_LOCK_DETECT)  return true;
  if (m_uTx->readIO() & PLL_LOCK_DETECT)  return true;
  return false;
}


bool RAD1Device::rx_setFreq(double freq, double *actual_freq) 
{
  unsigned R, control, N;
  if (!compute_regs(freq, &R, &control, &N, actual_freq)) return false;
  if (R==0) return false;

  unsigned char result[3]; 
  writeLock.lock(); 
  m_uRx->writeSpi(0,SPI_ENABLE_RX_A,SPI_FMT_MSB | SPI_FMT_HDR_0,
		    write_it((R & ~0x3) | 1, result), 3);
  m_uRx->writeSpi(0,SPI_ENABLE_RX_A,SPI_FMT_MSB | SPI_FMT_HDR_0,
		    write_it((control & ~0x3) | 0, result), 3);
  usleep(10000);
  m_uRx->writeSpi(0,SPI_ENABLE_RX_A,SPI_FMT_MSB | SPI_FMT_HDR_0,
		    write_it((N & ~0x3) | 2, result), 3);
  writeLock.unlock();
  
  usleep(100000); 
  if (m_uRx->readIO() & PLL_LOCK_DETECT)  return true;
  if (m_uRx->readIO() & PLL_LOCK_DETECT)  return true;

  return true; //return false;
}


RAD1Device::RAD1Device(int sps, bool wSkipRx)
{
  LOG(INFO) << "creating RAD1 device...";
  skipRx = wSkipRx;
  double desiredRate = sps*1625.0e3/6.0;
  decimRate = (unsigned int) round(masterClockRate/desiredRate);
  actualSampleRate = masterClockRate/decimRate;
  rxGain = 0;

  // This default value matches the RAD1r3.
  PC = 0;    // bits 3,2     Core power 15mA
#ifdef SWLOOPBACK 
  samplePeriod = 1.0e6/actualSampleRate;
  loopbackBufferSize = 0;
  gettimeofday(&lastReadTime,NULL);
  firstRead = false;
#endif
}

bool RAD1Device::open(const std::string &args, bool)
{
  int devID = atoi(args.c_str());
  readEEPROM(devID);

  writeLock.unlock();

  LOG(INFO) << "making RAD1 device..";
#ifndef SWLOOPBACK 
  string rbf = "fpga.rbf";
  //string rbf = "inband_1rxhb_1tx.rbf"; 
  if (!skipRx) {
  try {
    m_uRx = rnrad1Rx::make(devID,decimRate,
                           rbf,"ezusb.ihx");
    m_uRx->setFpgaMasterClockFreq(masterClockRate);
  }
  
  catch(...) {
    LOG(ERR) << "make failed on Rx";
    return false;
  }

  if (m_uRx->fpgaMasterClockFreq() != masterClockRate)
  {
    LOG(ERR) << "WRONG FPGA clock freq = " << m_uRx->fpgaMasterClockFreq()
               << ", desired clock freq = " << masterClockRate;
    return false;
  }
  m_uRx->writeOE(0,0xffff);
  m_uRx->writeOE((POWER_UP|RX_TXN|ENABLE), 0xffff);
  m_uRx->writeIO((POWER_UP|RX_TXN),(POWER_UP|RX_TXN|ENABLE));


  // FIXME -- This should be configurable.
  setRxGain(47); //maxRxGain());


  }

  try {
    m_uTx = rnrad1Tx::make(devID,decimRate*2,
                           rbf,"ezusb.ihx");
    m_uTx->setFpgaMasterClockFreq(masterClockRate);
  }
  
  catch(...) {
    LOG(ERR) << "make failed on Tx";
    return false;
  }

  m_uTx->writeOE(0,0xffff);
  m_uTx->writeOE((POWER_UP|RX_TXN|ENABLE), 0xffff);
  m_uTx->writeIO((POWER_UP|RX_TXN),(POWER_UP|RX_TXN|ENABLE));

  if (m_uTx->fpgaMasterClockFreq() != masterClockRate)
  {
    LOG(ERR) << "WRONG FPGA clock freq = " << m_uTx->fpgaMasterClockFreq()
               << ", desired clock freq = " << masterClockRate;
    return false;
  }

#endif

  samplesRead = 0;
  samplesWritten = 0;
  return true;
}



bool RAD1Device::start() 
{
  LOG(INFO) << "starting RAD1...";
#ifndef SWLOOPBACK 
  if (!m_uRx && !skipRx) return false;
  if (!m_uTx) return false;
  
  writeLock.lock();
  // power up and configure daughterboards
  m_uTx->writeOE(0,0xffff);
  m_uTx->writeOE((POWER_UP|RX_TXN|ENABLE), 0xffff);
  m_uTx->writeIO((POWER_UP|RX_TXN),(POWER_UP|RX_TXN|ENABLE));
  //m_uTx->writeIO(0,ENABLE,(RX_TXN | ENABLE));
  m_uTx->writeIO((RX_TXN | ENABLE), (RX_TXN | ENABLE));//only for litie
  m_uTx->writeFpgaReg(FR_ATR_MASK_0 ,0);//RX_TXN|ENABLE);
  m_uTx->writeFpgaReg(FR_ATR_TXVAL_0,0);//,0 |ENABLE);
  m_uTx->writeFpgaReg(FR_ATR_RXVAL_0,0);//,RX_TXN|0);
  m_uTx->writeFpgaReg(40,0);
  m_uTx->writeFpgaReg(42,0);
  m_uTx->setPga(0,m_uTx->pgaMax()); // should be 20dB
  m_uTx->setPga(1,m_uTx->pgaMax());
  m_uTx->setMux(0x00000098);
  LOG(INFO) << "TX pgas: " << m_uTx->pga(0) << ", " << m_uTx->pga(1);
  writeLock.unlock();

  if (!skipRx) {
    writeLock.lock();
    m_uRx->writeFpgaReg(FR_ATR_MASK_0  + 1*3,0);
    m_uRx->writeFpgaReg(FR_ATR_TXVAL_0 + 1*3,0);
    m_uRx->writeFpgaReg(FR_ATR_RXVAL_0 + 1*3,0);
    m_uRx->writeFpgaReg(41,0);
    m_uRx->writeOE((POWER_UP|RX_TXN|ENABLE), 0xffff);
    m_uRx->writeIO((POWER_UP|RX_TXN|ENABLE),(POWER_UP|RX_TXN|ENABLE));
    //m_uRx->writeIO(1,0,RX2_RX1N); // using Tx/Rx/
    m_uRx->writeIO(RX2_RX1N,RX2_RX1N); // using Rx2
    m_uRx->setAdcBufferBypass(0,true);
    m_uRx->setAdcBufferBypass(1,true);
    m_uRx->setPga(0,m_uRx->pgaMax()); // should be 20dB
    m_uRx->setPga(1,m_uRx->pgaMax());
    m_uRx->setMux(0x00000010);
    writeLock.unlock();
  }

  data = new short[currDataSize];
  dataStart = 0;
  dataEnd = 0;
  timeStart = 0;
  timeEnd = 0;
  timestampOffset = 0;
  latestWriteTimestamp = 0;
  lastPktTimestamp = 0;
  hi32Timestamp = 0;
  isAligned = false;

 
  if (!skipRx) 
    return (m_uRx->start() && m_uTx->start());
  else
    return m_uTx->start();
#else
  gettimeofday(&lastReadTime,NULL);
  return true;
#endif
}

bool RAD1Device::stop() 
{
#ifndef SWLOOPBACK 
  if (!m_uRx) return false;
  if (!m_uTx) return false;
  
  // power down
  m_uTx->writeIO((~POWER_UP|RX_TXN),(POWER_UP|RX_TXN|ENABLE));
  m_uRx->writeIO(~POWER_UP,(POWER_UP|ENABLE));
  
  delete[] data;
#endif
  return true;
}

double RAD1Device::setTxGain(double dB) {
 
   writeLock.lock();
   if (dB > maxTxGain()) dB = maxTxGain();
   if (dB < minTxGain()) dB = minTxGain();

   m_uTx->setPga(0,dB);
   m_uTx->setPga(1,dB);

   //LOG(NOTICE) << "Setting TX PGA to " << dB << " dB.";

   writeLock.unlock();
  
   return dB;
}


double RAD1Device::setRxGain(double dB) {

   writeLock.lock();
   if (dB > maxRxGain()) dB = maxRxGain();
   if (dB < minRxGain()) dB = minRxGain();
   
   double dBret = dB;

   dB = dB - minRxGain();

   double rfMax = 70.0;
   if (dB > rfMax) {
        m_uRx->setPga(0,dB-rfMax);
        m_uRx->setPga(1,dB-rfMax);
        dB = rfMax;
   }
   else {
        m_uRx->setPga(0,0);
        m_uRx->setPga(1,0);
   }
   m_uRx->writeAuxDac(0,
        (int) ceil((1.2 + 0.02 - (dB/rfMax))*4096.0/3.3));

   LOG(DEBUG) << "Setting DAC voltage to " << (1.2+0.02 - (dB/rfMax)) << " " << (int) ceil((1.2 + 0.02 - (dB/rfMax))*4096.0/3.3);

   rxGain = dBret;
   
   writeLock.unlock();
   
   return dBret;
}


// NOTE: Assumes sequential reads
int RAD1Device::readSamples(short *buf, int len, bool *overrun, 
			    TIMESTAMP timestamp,
			    bool *underrun,
			    unsigned *RSSI) 
{
#ifndef SWLOOPBACK 
  if (!m_uRx) return 0;
  
  timestamp += timestampOffset;
  
  if (timestamp + len < timeStart) {
    memset(buf,0,len*2*sizeof(short));
    return len;
  }

  if (underrun) *underrun = false;
 
  uint32_t readBuf[20000];
 
  while (1) {
    //guestimate USB read size
    int readLen=0;
    {
      int numSamplesNeeded = timestamp + len - timeEnd;
      if (numSamplesNeeded <=0) break;
      readLen = 512 * ((int) ceil((float) numSamplesNeeded/126.0));
      if (readLen > 8000) readLen= (8000/512)*512;
    }
    
    // read USRP packets, parse and save A/D data as needed
    readLen = m_uRx->read((void *)readBuf,readLen,overrun);
    for(int pktNum = 0; pktNum < (readLen/512); pktNum++) {
      // tmpBuf points to start of a USB packet
      uint32_t* tmpBuf = (uint32_t *) (readBuf+pktNum*512/4);
      TIMESTAMP pktTimestamp = usrp_to_host_u32(tmpBuf[1]);
      uint32_t word0 = usrp_to_host_u32(tmpBuf[0]);
      uint32_t chan = (word0 >> 16) & 0x1f;
      unsigned payloadSz = word0 & 0x1ff;
      LOG(DEBUG) << "first two bytes: " << hex << word0 << " " << dec << pktTimestamp;

      bool incrementHi32 = ((lastPktTimestamp & 0x0ffffffffll) > pktTimestamp);
      if (incrementHi32 && (timeStart!=0)) {
           LOG(DEBUG) << "high 32 increment!!!";
           hi32Timestamp++;
      }
      pktTimestamp = (((TIMESTAMP) hi32Timestamp) << 32) | pktTimestamp;
      lastPktTimestamp = pktTimestamp;

      if (chan == 0x01f) {
	// control reply, check to see if its ping reply
        uint32_t word2 = usrp_to_host_u32(tmpBuf[2]);
	if ((word2 >> 16) == ((0x01 << 8) | 0x02)) {
          timestamp -= timestampOffset;
	  timestampOffset = pktTimestamp - pingTimestamp + PINGOFFSET;
	  LOG(DEBUG) << "updating timestamp offset to: " << timestampOffset;
          timestamp += timestampOffset;
	  isAligned = true;
	}
	continue;
      }
      if (chan != 0) {
	LOG(DEBUG) << "chan: " << chan << ", timestamp: " << pktTimestamp << ", sz:" << payloadSz;
	continue;
      }
      if ((word0 >> 28) & 0x04) {
	if (underrun) *underrun = true; 
	LOG(DEBUG) << "UNDERRUN in TRX->USRP interface";
      }
      if (RSSI) *RSSI = (word0 >> 21) & 0x3f;
      
      if (!isAligned) continue;
      
      unsigned cursorStart = pktTimestamp - timeStart + dataStart;
      while (cursorStart*2 > currDataSize) {
	cursorStart -= currDataSize/2;
      }
      if (cursorStart*2 + payloadSz/2 > currDataSize) {
	// need to circle around buffer
	memcpy(data+cursorStart*2,tmpBuf+2,(currDataSize-cursorStart*2)*sizeof(short));
	memcpy(data,tmpBuf+2+(currDataSize/2-cursorStart),payloadSz-(currDataSize-cursorStart*2)*sizeof(short));
      }
      else {
	memcpy(data+cursorStart*2,tmpBuf+2,payloadSz);
      }
      if (pktTimestamp + payloadSz/2/sizeof(short) > timeEnd) 
	timeEnd = pktTimestamp+payloadSz/2/sizeof(short);

      //LOG(DEBUG) << "timeStart: " << timeStart << ", timeEnd: " << timeEnd << ", pktTimestamp: " << pktTimestamp;

    }	
  }     
 
  // copy desired data to buf
  unsigned bufStart = dataStart+(timestamp-timeStart);
  if (bufStart + len < currDataSize/2) { 
    //LOG(DEBUG) << "bufStart: " << bufStart;
    memcpy(buf,data+bufStart*2,len*2*sizeof(short));
    memset(data+bufStart*2,0,len*2*sizeof(short));
  }
  else {
    //LOG(DEBUG) << "len: " << len << ", currDataSize/2: " << currDataSize/2 << ", bufStart: " << bufStart;
    unsigned firstLength = (currDataSize/2-bufStart);
    //LOG(DEBUG) << "firstLength: " << firstLength;
    memcpy(buf,data+bufStart*2,firstLength*2*sizeof(short));
    memset(data+bufStart*2,0,firstLength*2*sizeof(short));
    memcpy(buf+firstLength*2,data,(len-firstLength)*2*sizeof(short));
    memset(data,0,(len-firstLength)*2*sizeof(short));
  }
  dataStart = (bufStart + len) % (currDataSize/2);
  timeStart = timestamp + len;

  // do IQ swap here
  for (int i = 0; i < len; i++) {
    short tmp = usrp_to_host_short(buf[2*i]);
    buf[2*i] = usrp_to_host_short(buf[2*i+1]);
    buf[2*i+1] = tmp;
  } 
 
  return len;
  
#else
  if (loopbackBufferSize < 2) return 0;
  int numSamples = 0;
  struct timeval currTime;
  gettimeofday(&currTime,NULL);
  double timeElapsed = (currTime.tv_sec - lastReadTime.tv_sec)*1.0e6 + 
    (currTime.tv_usec - lastReadTime.tv_usec);
  if (timeElapsed < samplePeriod) {return 0;}
  int numSamplesToRead = (int) floor(timeElapsed/samplePeriod);
  if (numSamplesToRead < len) return 0;
  
  if (numSamplesToRead > len) numSamplesToRead = len;
  if (numSamplesToRead > loopbackBufferSize/2) {
    firstRead =false; 
    numSamplesToRead = loopbackBufferSize/2;
  }
  memcpy(buf,loopbackBuffer,sizeof(short)*2*numSamplesToRead);
  loopbackBufferSize -= 2*numSamplesToRead;
  memcpy(loopbackBuffer,loopbackBuffer+2*numSamplesToRead,
	 sizeof(short)*loopbackBufferSize);
  numSamples = numSamplesToRead;
  if (firstRead) {
    int new_usec = lastReadTime.tv_usec + (int) round((double) numSamplesToRead * samplePeriod);
    lastReadTime.tv_sec = lastReadTime.tv_sec + new_usec/1000000;
    lastReadTime.tv_usec = new_usec % 1000000;
  }
  else {
    gettimeofday(&lastReadTime,NULL);
    firstRead = true;
  }
  samplesRead += numSamples;
  
  return numSamples;
#endif
}

int RAD1Device::writeSamples(short *buf, int len, bool *underrun, 
			     unsigned long long timestamp,
			     bool isControl) 
{
  writeLock.lock();

#ifndef SWLOOPBACK 
  if (!m_uTx) return 0;
 
  static uint32_t outData[128*200];
 
  for (int i = 0; i < len*2; i++) {
	buf[i] = host_to_usrp_short(buf[i]);
  }

  int numWritten = 0;
  unsigned isStart = 1;
  unsigned RSSI = 0;
  unsigned CHAN = (isControl) ? 0x01f : 0x00;
  len = len*2*sizeof(short);
  int numPkts = (int) ceil((float)len/(float)504);
  unsigned isEnd = (numPkts < 2);
  uint32_t *outPkt = outData;
  int pktNum = 0;
  while (numWritten < len) {
    // pkt is pointer to start of a USB packet
    uint32_t *pkt = outPkt + pktNum*128;
    isEnd = (len - numWritten <= 504);
    unsigned payloadLen = ((len - numWritten) < 504) ? (len-numWritten) : 504;
    pkt[0] = (isStart << 12 | isEnd << 11 | (RSSI & 0x3f) << 5 | CHAN) << 16 | payloadLen;
    pkt[1] = timestamp & 0x0ffffffffll;
    memcpy(pkt+2,buf+(numWritten/sizeof(short)),payloadLen);
    numWritten += payloadLen;
    timestamp += payloadLen/2/sizeof(short);
    isStart = 0;
    pkt[0] = host_to_usrp_u32(pkt[0]);
    pkt[1] = host_to_usrp_u32(pkt[1]);
    pktNum++;
  }
  m_uTx->write((const void*) outPkt,sizeof(uint32_t)*128*numPkts,NULL);

  samplesWritten += len/2/sizeof(short);
  writeLock.unlock();

  return len/2/sizeof(short);
#else
  int retVal = len;
  memcpy(loopbackBuffer+loopbackBufferSize,buf,sizeof(short)*2*len);
  samplesWritten += retVal;
  loopbackBufferSize += retVal*2;
   
  return retVal;
#endif
}

bool RAD1Device::updateAlignment(TIMESTAMP timestamp) 
{
#ifndef SWLOOPBACK 
  short data[] = {0x00,0x02,0x00,0x00};
  bool tmpUnderrun;
  if (writeSamples((short *) data,1,&tmpUnderrun,timestamp & 0x0ffffffffll,true)) {
    pingTimestamp = timestamp;
    return true;
  }
  return false;
#else
  return true;
#endif
}

bool RAD1Device::setVCTCXO(unsigned int freq_cal) {
#ifndef SWLOOPBACK 
  m_uRx->writeAuxDac(2,freq_cal << 4);
#endif
  return true;
}

#ifndef SWLOOPBACK 
bool RAD1Device::setTxFreq(double wFreq, double wAdjFreq) {
  // Tune to wFreq+LO_OFFSET, to prevent LO bleedthrough from interfering with transmitted signal.
  double actFreq;
  unsigned int freq_cal;
  freq_cal = (unsigned int) round(wAdjFreq); //128+round(wAdjFreq/wFreq*5.4e6);
  m_uRx->writeAuxDac(2,freq_cal << 4);
  if (!tx_setFreq(wFreq+1*LO_OFFSET,&actFreq)) return false;
  bool retVal = m_uTx->setTxFreq(wFreq-actFreq);
  LOG(DEBUG) << "set TX: " << wFreq-actFreq << " actual TX: " << m_uTx->txFreq();
  //printf("set TX: %f, set RX: %f\n", wFreq-actFreq, m_uTx->txFreq());
  //only for litie,set pow to maxim,switch on 1W amp
  //m_uRx->writeAuxDac(1,0);
  //m_uRx->writeIO(RX_TXN,RX_TXN);

  return retVal;
};

bool RAD1Device::setRxFreq(double wFreq, double wAdjFreq) {
  // Tune to wFreq-2*LO_OFFSET, to
  //   1) prevent LO bleedthrough (as with the setTxFreq method above)
  //   2) The extra LO_OFFSET pushes potential transmitter energy (GSM BS->MS transmissions 
  //        are 45Mhz above MS->BS transmissions) into a notch of the baseband lowpass filter 
  //        in front of the ADC.  This possibly gives us an extra 10-20dB Tx/Rx isolation.
  double actFreq;
  unsigned int freq_cal;
  freq_cal = (unsigned int) round(wAdjFreq); //128+round(wAdjFreq/wFreq*5.4e6);
  m_uRx->writeAuxDac(2,freq_cal << 4);
  if (!rx_setFreq(wFreq-2*LO_OFFSET,&actFreq)) return false;
  bool retVal = m_uRx->setRxFreq(wFreq-actFreq);
  //printf("set RX: %f, actual RX: %f\n",wFreq-actFreq,m_uRx->rxFreq());
  LOG(DEBUG) << "set RX: " << wFreq-actFreq << " actual RX: " << m_uRx->rxFreq();

  return retVal;
};

#else
bool RAD1Device::setTxFreq(double, double) { return true;};
bool RAD1Device::setRxFreq(double, double) { return true;};
#endif


// Factory calibration handling

unsigned int RAD1Device::getFactoryValue(const std::string &name) {
	if (name.compare("sdrsn")==0) {
		return sdrsn;

	} else if (name.compare("rfsn")==0) {
		return rfsn;

	// TODO : (mike) I thought these should be DEC comparisons but only HEX vals are matching,
	//		too rushed for 3.1 to figure out why I'm dumb
	} else if (name.compare("band")==0) {
		// BAND_85="85" # dec 133
		if (band == 85) {
			return 850;

		// BAND_90="90" # dec 144
		} else if (band == 90) {
			return 900;

		// BAND_18="18" # dec 24
		} else if (band == 18) {
			return 1800;

		// BAND_19="19" # dec 25
		} else if (band == 19) {
			return 1900;

		// BAND_21="21" # dec 33
		} else if (band == 21) {
			return 2100;

		// BAND_MB="ab" # dec 171
		} else if (band == 0xab) {
			return 0;

		// TODO : anything to handle here? for now pretend they have a multi-band
		} else {
			return 0;
		}

	} else if (name.compare("freq")==0) {
		return freq;

	} else if (name.compare("rxgain")==0) {
		return rxgain;

	} else if (name.compare("txgain")==0) {
		return txgain;

	// TODO : need a better error condition here
	} else {
		return 0;
	}
}

void RAD1Device::readEEPROM(int deviceID) {

	rnrad1Core core(
		deviceID,
		RAD1_CMD_INTERFACE,
		RAD1_CMD_ALTINTERFACE,
		"fpga.rbf",
		"ezusb.ihx",
		false
	);

	std::string temp;
	std::string temp1;

	/*
	SDR_ADDR="0x50"
	BASEST="df"
	MKR_ST="ff"
	./RAD1Cmd i2c_write $SDR_ADDR $BASEST$MKR_ST
	sleep 1
	*/
	i2c_write(core, 0x50, "dfff");
	sleep(1);
//	std::cout << "i2c_write = " << ret << std::endl;

	/*
	TEMP=$( ./RAD1Cmd i2c_read $SDR_ADDR 16 )
	sleep 1
	*/
	temp = i2c_read(core, 0x50, 16);
	sleep(1);
//	std::cout << "i2c_read 16 = " << temp << std::endl;

	/*
	TEMP=$( ./RAD1Cmd i2c_read $SDR_ADDR 32 )
	*/
	temp = i2c_read(core, 0x50, 32);
//	std::cout << "i2c_read 32 = " << temp << std::endl;

	/*
	# parse SDR serial number
	TEMP1=$( echo "$TEMP" | cut -c 1-4  )
	SDRSN=$( printf '%d' 0x$TEMP1 )
	echo "SDR Serial Number [$SDRSN] hex was [$TEMP1]"
	*/
	temp1 = temp.substr(0, 4);
	sdrsn = hex2dec(temp1);
//	std::cout << "SDR Serial Number [" << sdrsn << "] hex was [" << temp1 << "]" << std::endl;

	/*
	# parse RF serial number
	TEMP1=$( echo "$TEMP" | cut -c 5-8  )
	RFSN=$( printf '%d' 0x$TEMP1 )
	echo "RF Serial Number [$RFSN] hex was [$TEMP1]"
	*/
	temp1 = temp.substr(4, 4);
	rfsn = hex2dec(temp1);
//	std::cout << "RF Serial Number [" << rfsn << "] hex was [" << temp1 << "]" << std::endl;

	/*
	# BAND
	BAND=$( echo "$TEMP" | cut -c 9-10  )
	echo "RF BAND [$BAND]"
	*/
	temp1 = temp.substr(8, 2);
	band = atoi(temp1.c_str());
//	std::cout << "RF BAND [" << band << "]" << std::endl;

	/*
	# Frequency Setting
	TEMP1=$( echo "$TEMP" | cut -c 11-12  )
	FREQ=$( printf '%d' 0x$TEMP1 )
	echo "FREQ [$FREQ]"
	*/
	temp1 = temp.substr(10, 2);
	freq = hex2dec(temp1);
//	std::cout << "FREQ [" << freq << "] hex was [" << temp1 << "]" << std::endl;

	/*
	# RxGAIN
	TEMP1=$( echo "$TEMP" | cut -c 13-14  )
	RXGN=$( printf '%d' 0x$TEMP1 )
	echo "RxGAIN [$RXGN]"
	*/
	temp1 = temp.substr(12, 2);
	rxgain = hex2dec(temp1);
//	std::cout << "RxGAIN [" << rxgain << "] hex was [" << temp1 << "]" << std::endl;

	/*
	# TxGAIN
	TEMP1=$( echo "$TEMP" | cut -c 15-16  )
	TXGN=$( printf '%d' 0x$TEMP1 )
	echo "ATTEN [$TXGN]"
	*/
	temp1 = temp.substr(14, 2);
	txgain = hex2dec(temp1);
//	std::cout << "TxGAIN/ATTEN [" << txgain << "] hex was [" << temp1 << "]" << std::endl;

}


// Device creation factory

RadioDevice *RadioDevice::make(int &sps, bool skipRx)
{
  return new RAD1Device(sps, skipRx);
}

void RadioDevice::staticInit()
{
}
