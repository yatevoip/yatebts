/*
 * Copyright 2014 Free Software Foundation, Inc.
 *
 * This software is distributed under multiple licenses; see the COPYING file in
 * the main directory for licensing information for this specific distribuion.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 **/

#ifndef _BLADERF_DEVICE_H_
#define _BLADERF_DEVICE_H_

#include <libbladeRF.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "radioDevice.h"
#include <sys/time.h>
#include <math.h>
#include <string>
#include <iostream>

#include "bytesex.h"


// Timestamped data includes a header that overlaps the start of buffer
// The header takes 16 octets = 4 samples of (real part: 2 octets, imaginary: 2 octets)
// Timestamp is in half sample components; a full buffer increments it by 1016
#define PKT_SAMPLES_SUPER_RAW 512
#define PKT_SAMPLES_SUPER_USE (PKT_SAMPLES_SUPER_RAW - 4)
#define PKT_SAMPLES_HIGH_RAW 256
#define PKT_SAMPLES_HIGH_USE (PKT_SAMPLES_HIGH_RAW - 4)
#define PKT_BUFFERS 4

struct bladerf_timestamp {
    uint32_t rsvd;
    uint32_t time_lo;
    uint32_t time_hi;
    uint32_t flags;
};

struct bladerf_highspeed_timestamp {
    struct bladerf_timestamp header;
    int16_t samples[PKT_SAMPLES_HIGH_USE * 2];
};

struct bladerf_superspeed_timestamp {
    struct bladerf_timestamp header;
    int16_t samples[PKT_SAMPLES_SUPER_USE * 2];
};

/** A class to handle a bladeRF device */
class bladeRFDevice: public RadioDevice {

private:
  double desiredSampleRate; 	///< the desired sampling rate
  double actualSampleRate;	///< the actual bladeRF sampling rate
  unsigned int decimRate;	///< the bladeRF decimation rate
  int sps;

  unsigned long long samplesRead;	///< number of samples read from bladeRF
  unsigned long long samplesWritten;	///< number of samples sent to bladeRF

  bool skipRx;			///< set if bladeRF is transmit-only.

  static const unsigned int currDataSize_log2 = 21;
  static const unsigned long currDataSize = (1 << currDataSize_log2);
  short *data;
  struct bladerf *bdev;

  Mutex writeLock;

  double rxGain;
  int mRxGain1;

  bool isSuperSpeed;
  union {
    struct bladerf_highspeed_timestamp highSpeed[PKT_BUFFERS];
    struct bladerf_superspeed_timestamp superSpeed[PKT_BUFFERS];
  } rxBuffer;
  union {
    struct bladerf_highspeed_timestamp highSpeed;
    struct bladerf_superspeed_timestamp superSpeed;
  } txBuffer;
  int rxBufIndex;
  int rxBufCount;
  int rxConsumed;
  int txBuffered;
  TIMESTAMP rxTimestamp;
  TIMESTAMP txTimestamp;
  TIMESTAMP rxResyncCandidate;

  /** Rx DC offsets correction */
  bool mDcCorrect;
  int mRxMaxOffset;
  int mRxCorrectionI;
  int mRxCorrectionQ;
  int mRxAverageI;
  int mRxAverageQ;

  /** debug mode control */
  int pulseMode;
  int rxShowInfo;
  int txShowInfo;
  bool spammy;

  /** Number of raw samples in current mode */
  inline int rawSamples() const
    { return isSuperSpeed ? PKT_SAMPLES_SUPER_RAW : PKT_SAMPLES_HIGH_RAW; }

  /** Number of usable samples in current mode */
  inline int useSamples() const
    { return isSuperSpeed ? PKT_SAMPLES_SUPER_USE : PKT_SAMPLES_HIGH_USE; }

  /** Set the transmission frequency */
  bool tx_setFreq(double freq, double *actual_freq);

  /** Set the receiver frequency */
  bool rx_setFreq(double freq, double *actual_freq);

  /** Set the RAD1 style VCTCXO*/
  bool setVCTCXOrad1(unsigned int adj);

  /** Set the RX DAC correction offsets */
  bool setRxOffsets(int corrI, int corrQ);

  /** Set the TX DAC correction offsets */
  bool setTxOffsets(int corrI, int corrQ);

  /** Set loopback mode */
  bool setLoopback(bladerf_loopback loopback);

public:

  /** Object constructor */
  bladeRFDevice (int oversampling, bool skipRx = false);

  /** Instantiate the bladeRF */
  bool open(const std::string &args = "", bool extref = false);

  /** Start the bladeRF */
  bool start();

  /** Stop the bladeRF */
  bool stop();

  /**
	Read samples from the bladeRF.
	@param buf preallocated buf to contain read result
	@param len number of samples desired
	@param overrun Set if read buffer has been overrun, e.g. data not being read fast enough
	@param timestamp The timestamp of the first samples to be read
	@param underrun Set if bladeRF does not have data to transmit, e.g. data not being sent fast enough
	@param RSSI The received signal strength of the read result
	@return The number of samples actually read
  */
  int  readSamples(short *buf, int len, bool *overrun,
		   TIMESTAMP timestamp = 0xffffffff,
		   bool *underrun = NULL,
		   unsigned *RSSI = NULL);
  /**
        Write samples to the bladeRF.
        @param buf Contains the data to be written.
        @param len number of samples to write.
        @param underrun Set if bladeRF does not have data to transmit, e.g. data not being sent fast enough
        @param timestamp The timestamp of the first sample of the data buffer.
        @param isControl Set if data is a control packet, e.g. a ping command
        @return The number of samples actually written
  */
  int  writeSamples(short *buf, int len, bool *underrun,
		    TIMESTAMP timestamp = 0xffffffff,
		    bool isControl = false);

  /** The bladeRF never goes out of alignment */
  bool updateAlignment(TIMESTAMP timestamp) { return true; }

  /** Set the VCTCXO offset*/
  bool setVCTCXO(unsigned int wAdjFreq = 0);
 
  /** Set the transmitter frequency */
  bool setTxFreq(double wFreq, double wAdjFreq = 0);

  /** Set the receiver frequency */
  bool setRxFreq(double wFreq, double wAdjFreq = 0);

  /** Returns the starting write Timestamp*/
  TIMESTAMP initialWriteTimestamp(void) { return 1; }

  /** Returns the starting read Timestamp*/
  TIMESTAMP initialReadTimestamp(void) { return 1; }

  /** returns the full-scale transmit amplitude **/
  double fullScaleInputValue() {return 2040.0;}

  /** returns the full-scale receive amplitude **/
  double fullScaleOutputValue() {return 2040.0;}

  /** sets the receive chan gain, returns the gain setting **/
  double setRxGain(double dB);

  /** get the current receive gain */
  double getRxGain(void) {return rxGain;}

  /** return maximum Rx Gain **/
  double maxRxGain(void) {return BLADERF_RXVGA2_GAIN_MAX;}

  /** return minimum Rx Gain **/
  double minRxGain(void) {return BLADERF_RXVGA2_GAIN_MIN;}

  /** sets the transmit chan gain, returns the gain setting **/
  double setTxGain(double dB);

  /** return maximum Tx Gain **/
  double maxTxGain(void) {return BLADERF_TXVGA2_GAIN_MAX;}

  /** return minimum Rx Gain **/
  double minTxGain(void) {return BLADERF_TXVGA2_GAIN_MIN;}


  /** Return internal status values */
  inline double getTxFreq() { return 0;}
  inline double getRxFreq() { return 0;}
  inline double getSampleRate() {return actualSampleRate;}
  inline double numberRead() { return samplesRead; }
  inline double numberWritten() { return samplesWritten;}

  /** return factory settings */
  unsigned int getFactoryValue(const std::string &name);

  bool runCustom(const std::string &command);
};

#endif // _BLADERF_DEVICE_H_
