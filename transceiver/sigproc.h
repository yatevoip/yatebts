/**
 * sigproc.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Transceiver Signal Process Library related classes
 *
 * Yet Another Telephony Engine - Base Transceiver Station
 * Copyright (C) 2014 Null Team Impex SRL
 *
 * This software is distributed under multiple licenses;
 * see the COPYING file in the main directory for licensing
 * information for this specific distribution.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef SIGPROC_H
#define SIGPROC_H

#include <yateclass.h>
#include <math.h>
#include <stdio.h>

namespace TelEngine {

// Enable ObjStore statistics
//#define SIGPROC_OBJ_STORE_DEBUG
// Disable ObjStore: can be used when running memcheck to detect invalid access
//#define SIGPROC_OBJ_STORE_DISABLE

class SigProcUtils;                      // Utility functions
class Complex;                           // A Complex (float) number
class SignalProcessing;                  // Signal processing

#define GSM_SYMBOL_RATE (13e6 / 48) // 13 * 10^6 / 48
#define BITS_PER_TIMESLOT 156.25
#define PI M_PI
#define PI2 (2.0 * PI)

/**
 * Processing library utility functions, not related to signal processing
 * @short Processing library utilities
 */
class SigProcUtils
{
public:
    /**
     * Calculate minimum value
     * @param v1 First value
     * @param v2 Second value
     * @return v1 or v2
     */
    static inline unsigned int min(unsigned int v1, unsigned int v2)
	{ return (v1 <= v2) ? v1 : v2; }

    /**
     * Calculate minimum value
     * @param v1 First value
     * @param v2 Second value
     * @param v3 Third value
     * @return v1 or v2 or v3
     */
    static inline unsigned int min(unsigned int v1, unsigned int v2, unsigned int v3)
	{ return min(v1,min(v2,v3)); }

    /**
     * Copy memory
     * @param dest Destination buffer
     * @param len Destination buffer length
     * @param src Pointer to data to copy
     * @param n The number of bytes to copy
     * @param objSize Object (element) size in bytes
     * @param offs Optional offset in destination
     * @return The number of elements copied
     */
    static unsigned int copy(void* dest, unsigned int len,
	const void* src, unsigned int n,
	unsigned int objSize = 1, unsigned int offs = 0);

    /**
     * Reset memory (set to 0)
     * @param buf The buffer
     * @param len Buffer length
     * @param objSize Object (element) size in bytes
     * @param offs Optional offset in buffer
     * @return The number of processed elements
     */
    static unsigned int bzero(void* buf, unsigned int len,
	unsigned int objSize = 1, unsigned int offs = 0);
	
    /**
     * Split a string and append lines to another one
     * @param buf Destination string
     * @param str Input string
     * @param lineLen Line length, characters to copy
     * @param offset Offset in first line (if incomplete). No data will be
     *  added on first line if offset is greater then line length
     * @param linePrefix Prefix for new lines.
     *  Set it to empty string or 0 to use the suffix
     * @param suffix End of line for the last line
     * @return Destination string address
     */
    static String& appendSplit(String& buf, const String& str, unsigned int lineLen,
	unsigned int offset = 0, const char* linePrefix = 0,
	const char* suffix = "\r\n");

    /**
     * Callback method to parse a complex number
     * @param obj The complex number resulted from parse.
     * @param data The string to be parsed
     */
    static void callbackParse(Complex& obj, const String& data);
	
    /**
     * Append float value to a String (using "%g" format)
     * @param dest Destination String
     * @param val Value to append
     * @param sep Separator to use
     * @return Destination String address
     */
    static String& appendFloat(String& dest, const float& val, const char* sep);

    /**
     * Append a Complex number to a String (using "%g%+gi" format)
     * @param dest Destination String
     * @param val Value to append
     * @param sep Separator to use
     * @return Destination String address
     */
    static String& appendComplex(String& dest, const Complex& val, const char* sep);

    /**
     * Energize a number. Refer the input value to the requested energy.
     * @param value The number to energize.
     * @param energy The energy to be applied
     * @param scale Scale factor for the given value.
     * @param clamp This value is incremented if the value is outside -1 1 range.
     */
    inline static float energize(float value, unsigned int energy, float scale, unsigned int& clamp)
    {
	if (scale != 1)
	value *= scale;
	if (value > 1) {
	    clamp++;
	    return energy;
	}
	if (value < -1) {
	    clamp++;
	    return - energy;
	}
	return value * energy;
    }
};


/**
 * This class implements a complex number
 * @short A Complex (float) number
 */
class Complex
{
public:
    /**
     * Constructor
     * Default (0,0i)
     */
    inline Complex()
	: m_real(0), m_imag(0)
	{}

    /**
     * Constructor
     * @param real The real part of the complex number
     * @param imag The imaginary part of a complex number
     */
    inline Complex(float real, float imag = 0)
	: m_real(real), m_imag(imag)
	{}

    /**
     * Copy Constructor
     * @param c The source complex number
     */
    inline Complex(const Complex& c)
	: m_real(c.real()), m_imag(c.imag())
	{}

    /**
     * Obtain the real part of the complex number
     * @return The real part
     */
    inline float real() const
	{ return m_real; }

    /**
     * Set the real part of the complex number
     * @param r The new real part value
     */
    inline void real(float r)
	{ m_real = r; }

    /**
     * Obtain the imaginary part of a complex number
     * @return The imaginary part
     */
    inline float imag() const
	{ return m_imag; }

    /**
     * Set the imaginary part of the complex number
     * @param i The new imaginary part value
     */
    inline void imag(float i)
	{ m_imag = i; }

    /**
     * Set data
     * @param r The real part of the complex number
     * @param i The imaginary part of a complex number
     */
    inline void set(float r = 0, float i = 0) {
	    real(r);
	    imag(i);
	}

    inline bool operator==(const Complex& c) const
	{ return real() == c.real() && imag() == c.imag(); }

    inline Complex& operator=(float real) {
	    set(real);
	    return *this; 
	}

    inline Complex& operator=(const Complex& c) {
	    set(c.real(),c.imag());
	    return *this; 
	}

    inline Complex& operator+=(float real) { 
	    m_real += real;
	    return *this;
	}

    inline Complex& operator+=(const Complex& c)
	{ return sum(*this,*this,c); }

    inline Complex& operator-=(float real) {
	    m_real -= real;
	    return *this; 
	}

    inline Complex& operator-=(const Complex& c)
	{ return diff(*this,*this,c); }

    inline Complex& operator*=(float f)
	{ return multiplyF(*this,*this,f); }

    inline Complex operator*(const Complex& c) const {
	    Complex tmp;
	    return multiply(tmp,*this,c);
	}

    inline Complex operator*(float f) const {
	    Complex tmp(*this);
	    return (tmp *= f);
	}

    inline Complex operator+(const Complex& c) const {
	    Complex tmp;
	    return sum(tmp,*this,c);
	}

    /**
     * Obtain tha absolute value of a complex number
     * @return The absolute value.
     */
    inline float abs() const
	{ return ::sqrt(m_real*m_real + m_imag*m_imag);}

    /**
     * Multiply this number with its conjugate
     * @return The result
     */
    inline float mulConj() const
	{ return m_real * m_real + m_imag * m_imag; }

    void dump(String& dest) const;

    static inline Complex& exp(Complex& dest, float r, float i) {
	    dest.set(::expf(r) * ::cosf(i),::expf(r) * ::sinf(i));
	    return dest;
	}

    /**
     * Multiply two complex numbers
     * Multiply c1 and c2 and put the result in dest.
     * @param dest Destination
     * @param c1 First number
     * @param c2 Second number
     * @return Destination number address
     */
    static inline Complex& multiply(Complex& dest, const Complex& c1,
	const Complex& c2) {
	    dest.set(c1.real() * c2.real() - c1.imag() * c2.imag(),
		c1.real() * c2.imag() + c1.imag() * c2.real());
	    return dest;
	}

    /**
     * Multiply a complex numbers with a float
     * Multiply c1 and real and put the result in dest
     * @param dest Destination
     * @param c Complex number
     * @param real Float number
     * @return Destination number address
     */
    static inline Complex& multiplyF(Complex& dest, const Complex& c, float real) {
	    dest.set(c.real() * real,c.imag() * real);
	    return dest;
	}

    /**
     * Multiply two complex arrays
     * Multiply c1 and c2 and put the result in dest.
     * The number of elements to process is min(len,len1,len2)
     * @param dest Destination array
     * @param len Destination array length
     * @param c1 Pointer to first array
     * @param len1 First array length
     * @param c2 Second array
     * @param len2 Second array length
     */
    static inline void multiply(Complex* dest, unsigned int len,
	const Complex* c1, unsigned int len1, const Complex* c2, unsigned int len2) {
	    unsigned int n = SigProcUtils::min(len,len1,len2);
	    for (; n; --n, ++dest, ++c1, ++c2)
		multiply(*dest,*c1,*c2);
	}

    /**
     * Multiply two complex arrays.
     * Multiply c1 and c2 and put the result in c1.
     * The number of elements to process is min(len1,len2)
     * @param c1 Pointer to first array
     * @param len1 First array length
     * @param c2 Second array
     * @param len2 Second array length
     */
    static inline void multiply(Complex* c1, unsigned int len1,
	const Complex* c2, unsigned int len2)
	{ multiply(c1,len1,c1,len1,c2,len2); }

    /**
     * Compute the sum of two complex numbers.
     * Sum c1 and c2 and put the result in dest
     * @param dest Destination number
     * @param c1 First number
     * @param c2 Second number
     * @return Destination number address
     */
    static inline Complex& sum(Complex& dest, const Complex& c1, const Complex& c2) {
	    dest.set(c1.real() + c2.real(),c1.imag() + c2.imag());
	    return dest;
	}

    /**
     * Compute the sum of two complex arrays.
     * Sum c1 and c2 and put the result in dest.
     * The number of elements to process is min(len,len1,len2)
     * @param dest Destination array
     * @param len Destination array length
     * @param c1 Pointer to first array
     * @param len1 First array length
     * @param c2 Second array
     * @param len2 Second array length
     */
    static inline void sum(Complex* dest, unsigned int len,
	const Complex* c1, unsigned int len1, const Complex* c2, unsigned int len2) {
	    unsigned int n = SigProcUtils::min(len,len1,len2);
	    for (; n; --n, ++dest, ++c1, ++c2)
		sum(*dest,*c1,*c2);
	}

    /**
     * Compute the sum of two complex arrays.
     * Sum c1 and c2 and put the result in c1.
     * The number of elements to process is min(len1,len2)
     * @param c1 Pointer to first array
     * @param len1 First array length
     * @param c2 Second array
     * @param len2 Second array length
     */
    static inline void sum(Complex* c1, unsigned int len1,
	const Complex* c2, unsigned int len2)
	{ sum(c1,len1,c1,len1,c2,len2); }

    /**
     * Compute the sum of product between complex number and its conjugate
     *  of all numbers in an array
     * @param data Array to process
     * @param len Array length
     * @return The result
     */
    static float sumMulConj(const Complex* data, unsigned int len);

    /**
     * Compute the sum of product between 2 complex numbers
     * E.g. dest += c1 * c2
     * @param dest Destination number
     * @param c1 First number
     * @param c2 Second number
     * @return Destination number address
     */
    static inline Complex& sumMul(Complex& dest, const Complex& c1, const Complex& c2) {
	    dest.set(dest.real() + (c1.real() * c2.real() - c1.imag() * c2.imag()),
		dest.imag() + (c1.real() * c2.imag() + c1.imag() * c2.real()));
	    return dest;
	}

    /**
     * Compute the sum of product between 2 complex arrays
     * E.g. dest = SUM(c1[i] * c2[i])
     * @param dest Destination number
     * @param c1 Pointer to first array
     * @param len1 First array length
     * @param c2 Second array
     * @param len2 Second array length
     * @return Destination number address
     */
    static inline Complex& sumMul(Complex& dest, const Complex* c1, unsigned int len1,
	const Complex* c2, unsigned int len2){
	    for (unsigned int n = SigProcUtils::min(len1,len2); n; --n, ++c1, ++c2)
		sumMul(dest,*c1,*c2);
	    return dest;
	}

    /**
     * Compute the sum of dest and product of c1,c2
     * E.g. dest[i] = dest[i] + (c1[i] * c2[i])
     * @param dest Destination array
     * @param len Destination array length
     * @param c1 Pointer to first array
     * @param len1 First array length
     * @param c2 Second array
     * @param len2 Second array length
     */
    static inline void sumMul(Complex* dest, unsigned int len,
	const Complex* c1, unsigned int len1, const Complex* c2, unsigned int len2) {
	    unsigned int n = SigProcUtils::min(len,len1,len2);
	    for (; n; --n, ++dest, ++c1, ++c2)
		sumMul(*dest,*c1,*c2);
	}

    /**
     * Multiply c1 by f. Add the result to c
     * @param c Destination number
     * @param c1 Complex number to multiply
     * @param f Float number to multiply
     */
    static inline void sumMulF(Complex& c, const Complex& c1, float f)
	{ c.set(c.real() + c1.real() * f,c.imag() + c1.imag() * f); }

    /**
     * Calculate : c = c + (c1 + c2) * f
     * @param c Destination number
     * @param c1 First complex number
     * @param c2 Second complex number
     * @param f Float number to multiply
     */
    static inline void sumMulFSum(Complex& c, const Complex& c1,const Complex& c2, float f)
	{ c.set(c.real() + (c1.real() + c2.real()) * f,c.imag() + (c1.imag() + c2.imag()) * f);}

    /**
     * Compute the difference of two complex numbers
     * @param dest Destination number
     * @param c1 First number
     * @param c2 Second number
     * @return Destination number address
     */
    static inline Complex& diff(Complex& dest, const Complex& c1, const Complex& c2) {
	    dest.set(c1.real() - c2.real(),c1.imag() - c2.imag());
	    return dest;
	}

    /**
     * Compute the difference of two complex arrays.
     * The number of elements to process is min(len,len1,len2)
     * @param dest Destination array
     * @param len Destination array length
     * @param c1 Pointer to first array
     * @param len1 First array length
     * @param c2 Second array
     * @param len2 Second array length
     */
    static inline void diff(Complex* dest, unsigned int len,
	const Complex* c1, unsigned int len1, const Complex* c2, unsigned int len2) {
	    unsigned int n = SigProcUtils::min(len,len1,len2);
	    for (; n; --n, ++dest, ++c1, ++c2)
		diff(*dest,*c1,*c2);
	}

    /**
     * Compute the difference of two complex arrays.
     * The number of elements to process is min(len1,len2)
     * @param c1 Pointer to first array
     * @param len1 First array length
     * @param c2 Second array
     * @param len2 Second array length
     */
    static inline void diff(Complex* c1, unsigned int len1,
	const Complex* c2, unsigned int len2)
	{ diff(c1,len1,c1,len1,c2,len2); }

    /**
     * Replace each element in an array with its conjugate
     * @param c Pointer to data
     * @param len Data length
     */
    static inline void conj(Complex* c, unsigned int len) {
	    if (c)
		for (; len; --len, ++c)
		    c->imag(-c->imag());
	}

    /**
     * Set complex elements from 16 bit integers array (pairs of real/imaginary parts)
     * @param dest Destination buffer
     * @param len Destination buffer length
     * @param src The source buffer
     * @param n Source buffer length in 16 bit integer elements
     * @param offs Optional offset in destination to start copy
     */
    static void setInt16(Complex* dest, unsigned int len,
	const int16_t* src, unsigned int n, unsigned int offs = 0);

    /**
     * Dump a Complex vector
     * @param dest The destination string
     * @param c The vector to dump
     * @param len The number of elements to dump
     * @param sep Optional separator between values
     */
    static void dump(String& dest, const Complex* c, unsigned int len,
	const char* sep = " ");

private:
    float m_real;                        // The Real part of the complex number
    float m_imag;                        // The imaginary part of the complex numbers
};


/**
 * Basic vector template.
 * NOTE: This template won't work for objects holding pointers,
 *  it copies data using raw memory copy
 * @short Basic vector template
 * @param basicType True for basic types (i.e. float, int ...),
 *  false for struct/class objects. Basic type vectors will be reset when allocated
 * @param hardArrayOps Use hard copy/reset if true. Use loops to assign data if false
 */
template <class Obj, bool basicType = true, bool hardArrayOps = true> class SigProcVector
{
public:
    /**
     * Constructor
     */
    inline SigProcVector()
	: m_data(0), m_length(0)
	{}

    /**
     * Constructor
     * @param len Array length
     */
    inline SigProcVector(unsigned int len)
	: m_data(0), m_length(0)
	{ assign(len); }

    /**
     * Constructor from data
     * @param ptr Source buffer
     * @param n Elements to copy from source buffer
     * @param len Optional vector length, 0 to use source buffer length
     */
    inline SigProcVector(const Obj* ptr, unsigned int n, unsigned int len = 0)
	: m_data(0), m_length(0)
	{ assign(len ? len : n,ptr,n); }

    /**
     * Constructor from another vector
     * @param other Vector to assign
     */
    inline SigProcVector(const SigProcVector& other)
	: m_data(0), m_length(0)
	{ *this = other; }

    /**
     * Destructor
     */
    virtual ~SigProcVector()
	{ clear(); }

    /**
     * Retrieve vector data pointer
     * @return Vector data pointer, 0 if empty (not allocated)
     */
    inline Obj* data()
	{ return m_data; }

    /**
     * Retrieve vector data pointer
     * @return A const pointer to vector data, 0 if empty (not allocated)
     */
    inline const Obj* data() const
	{ return m_data; }

    /**
     * Retrieve vector length
     * @return Vector length
     */
    inline unsigned int length() const
	{ return m_length; }

    /**
     * Retrieve vector size in bytes (length() * sizeof(Obj))
     * @return Vector size in bytes
     */
    inline unsigned int size() const
	{ return length() * sizeof(Obj); }

    /**
     * Retrieve available elements from offset
     * @param offs Offset
     * @param nItems Items to check (-1 for all available)
     * @return Return available elements from given offset
     */
    inline unsigned int available(unsigned int offs = 0, int nItems = -1) const {
	    if (offs < length()) {
		unsigned int room = length() - offs;
		return nItems >= 0 ? SigProcUtils::min(room,nItems) : room;
	    }
	    return 0;
	}

    /**
     * Clear the vector
     * @param del Release data
     */
    inline void clear(bool del = true) {
	    if (!m_data)
		return;
	    if (del)
		delete[] m_data;
	    m_data = 0;
	    m_length = 0;
	}

    /**
     * Resize (re-alloc or free) this vector if required size is not the same
     *  as the current one
     * @param len Required vector length
     * @param rst Set it to true to reset vector contents
     */
    inline void resize(unsigned int len, bool rst = false) {
	    if (!rst) {
		if (len != length())
		    assign(len);
	    }
	    else if (len != length())
		assign(len,0,0,true);
	    else
		reset();
	}

    /**
     * Allocate space for vector. Copy data into it
     * @param ptr Buffer to copy
     * @param len The number of elements to copy
     */
    inline void assign(const Obj* ptr, unsigned int len)
	{ assign(len,ptr,len); }

    /**
     * Assign a slice of this vector
     * @param offs The offset to start
     * @param nCopy The number of elements to copy, 0 to assign the remaining elements
     */
    inline void assignSlice(unsigned int offs, unsigned int nCopy = 0) {
	    if (!data())
		return;
	    if (offs >= length()) {
		clear();
		return;
	    }
	    nCopy = SigProcUtils::min(length() - offs,nCopy ? nCopy : length());
	    if (nCopy != length())
		assign(nCopy,data() + offs,nCopy);
	}

    /**
     * Allocate space for vector
     * Reset memory if 'basicType' template parameter is true
     * @param len Vector length
     * @param ptr Optional pointer to buffer to copy
     * @param nCopy The number of elements to copy from 'ptr'
     * @param rst True to reset vector contents
     *  (ignored if not basic type: non basic type must have a default constructor anyway)
     */
    void assign(unsigned int len, const Obj* ptr = 0, unsigned int nCopy = 0,
	bool rst = true) {
	    Obj* tmp = len ? new Obj[len] : 0;
	    if (!tmp) {
		if (len)
		    Debug("SigProcVector",DebugFail,
			"Failed to allocate len=%u obj_size=%u [%p]",
			len,(unsigned int)(sizeof(Obj)),this);
		clear();
		return;
	    }
	    if (!(ptr && nCopy)) {
		if (basicType && rst)
		    reset(tmp,len);
	    }
	    else {
		unsigned int n = copy(tmp,len,ptr,nCopy);
		if (n < len && basicType && rst)
		    reset(tmp + n,len - n);
	    }
	    clear();
	    m_data = tmp;
	    m_length = len;
	}

    /**
     * Copy elements from another vector
     * @param ptr Pointer to data to copy
     * @param n The number of bytes to copy
     * @param offs Optional offset in our vector
     * @return The number of elements copied
     */
    inline unsigned int copy(const Obj* ptr, unsigned int n, unsigned int offs = 0)
	{ return copy(data(),length(),ptr,n,offs); }

    /**
     * Copy elements from another vector
     * @param other Vector to copy
     * @param offs Optional offset in our vector
     * @return The number of elements copied
     */
    inline unsigned int copy(const SigProcVector& other, unsigned int offs = 0)
	{ return copy(other.data(),other.length(),offs); }

    /**
     * Copy a slice inside the vector.
     * Source and destination buffer may overlap
     * @param fromOffs Source offset
     * @param toOffs Destinatin offset
     * @param len The number of elements to move
     * @return The number of elements moved
     */
    virtual unsigned int copySlice(unsigned int fromOffs, unsigned int toOffs,
	unsigned int len) {
	    len = SigProcUtils::min(len,available(fromOffs),available(toOffs));
	    if (!len)
		return 0;
	    // Source and destination don't overlap: just copy data
	    Obj* src = data() + fromOffs;
	    Obj* dest = data() + toOffs;
	    if (fromOffs < toOffs) {
		if (fromOffs + len < toOffs)
		    return copy(dest,len,src,len);
	    }
	    else if (toOffs + len < fromOffs)
		return copy(dest,len,src,len);
	    // Data overlap: use element copy
	    if (toOffs < fromOffs) {
		// Moving backward: start copy from 'src' start
		for (Obj* last = src + len; src != last; src++, dest++)
		    *dest = *src;
	    }
	    else {
		// Moving forward: start copy from 'src' end
		Obj* last = src - 1;
		dest += len - 1;
		src += len - 1;
		for (; src != last; src--, dest--)
		    *dest = *src;
	    }
	    return len;
	}

    /**
     * Reset the vector
     * @param offs Optional offset in vector
     * @param n Optional number of elements to reset (negative for all remaining)
     * @return The number of elements reset
     */
    inline unsigned int reset(unsigned int offs = 0, int n = -1)
	{ return reset(data() + offs,available(offs,n)); }

    /**
     * Fill this vector
     * @param value Value to fill with
     * @param offs Optional offset in vector
     * @return The number of elements set
     */
    inline unsigned int fill(const Obj& value, unsigned int offs = 0)
	{ return fill(data() + offs,available(offs),value); }

    /**
     * Take another vector's data
     * @param other Vector to steal data from
     */
    inline void steal(SigProcVector& other) {
	    clear();
	    m_data = other.data();
	    m_length = other.length();
	    other.clear(false);
	}

    /**
     * Exchange data with another vector
     * @param other Vector to exchange data with
     */
    inline void exchange(SigProcVector& other) {
	    Obj* tmpData = m_data;
	    unsigned int tmpLen = m_length;
	    m_data = other.m_data;
	    m_length = other.m_length;
	    other.m_data = tmpData;
	    other.m_length = tmpLen;
	}

    /**
     * Vector indexing operator
     * @param index Index to retrieve
     * @return The address of the object at the given index
     */
    inline Obj& operator[](unsigned int index)
	{ return m_data[index]; }

    /**
     * Vector indexing operator
     * @param index Index yo retrieve
     * @return Const address of the object at the given index
     */
    inline const Obj& operator[](unsigned int index) const
	{ return m_data[index]; }

    /**
     * Assignment operator
     * @param other Vector to set from
     * @return This vector's address
     */
    inline SigProcVector& operator=(const SigProcVector& other) {
	    assign(other.data(),other.length());
	    return *this;
	}

    /**
     * Assignment operator
     * @param other Vector to set from
     * @return This vector's address
     */
    inline SigProcVector& operator=(const SigProcVector* other) {
	    if (other)
		return operator=(*other);
	    clear();
	    return *this;
	}

    /**
     * Dump this vector to a string
     * @param buf Destination string
     * @param func Pointer to function who appends the object to a String
     * @param sep Vector elements separator
     * @return Destination string address
     */
    inline String& dump(String& buf,
	String& (*func)(String& dest, const Obj& item, const char* sep),
	const char* sep = ",", unsigned int offs = 0, int len = -1) const
	{ return dump(buf,data(),length(),func,0,-1,sep); }

    /**
     * Dump this vector to a string
     * @param buf Destination string
     * @param offs Offset in data to start
     * @param nDump The number of elements to dump (negative: all, 0: none)
     * @param func Pointer to function who appends the object to a String
     * @param sep Vector elements separator
     * @return Destination string address
     */
    inline String& dump(String& buf, unsigned int offs, int nDump,
	String& (*func)(String& dest, const Obj& item, const char* sep),
	const char* sep = ",")
	{ return dump(buf,data(),length(),func,offs,nDump,sep); }

    /**
     * Dump this vector to string, split it and append lines to a buffer
     * @param buf Destination string
     * @param func Pointer to function who append the object to a String
     *  This parameter is required
     * @param offset Offset in first line (if incomplete). No data will be
     *  added on first line if offset is greater then line length
     * @param linePrefix Prefix for new lines.
     *  Set it to empty string or 0 to use the suffix
     * @param suffix String to always add to final result
     * @param sep Vector elements separator
     * @return Destination string address
     */
    String& appendSplit(String& buf, unsigned int lineLen,
	String& (*func)(String& dest, const Obj& item, const char* sep),
	unsigned int offset = 0, const char* linePrefix = 0,
	const char* suffix = "\r\n", const char* sep = ",") const {
	    if (!func)
		return buf;
	    if (TelEngine::null(linePrefix))
		linePrefix = suffix;
	    if (!lineLen || TelEngine::null(linePrefix)) {
		dump(buf,func,sep);
		return buf.append(suffix);
	    }
	    String localBuf;
	    const Obj* v = data();
	    for (unsigned int n = length(); n; n--, v++) {
		String tmp;
		(*func)(tmp,*v,0);
		if (n > 1)
		    tmp << sep;
		offset += tmp.length();
		if (offset > lineLen) {
		    localBuf << linePrefix;
		    offset = tmp.length();
		}
		localBuf << tmp;
	    }
	    return buf.append(localBuf.append(suffix));
	}

    /**
     * Hexify data, split it and append lines to a string
     * @param buf Destination string
     * @param lineLen Line length, characters to copy
     * @param offset Offset in first line (if incomplete). No data will be
     *  added on first line if offset is greater then line length
     * @param linePrefix Prefix for new lines.
     *  Set it to empty string or 0 to use the suffix
     * @param suffix End of line for the last line
     * @return Destination string address
     */
    inline String& appendSplitHex(String& buf, unsigned int lineLen,
	unsigned int offset = 0, const char* linePrefix = 0,
	const char* suffix = "\r\n") const {
	    String hex;
	    return SigProcUtils::appendSplit(buf,hex.hexify((void*)data(),size()),
		lineLen,offset,linePrefix,suffix);
	}
	
    /**
     * Build this vector from a hexadecimal string representation.
     * Clears the vector at start, i.e. the vector will be empty on failure.
     * The vector may be empty on success also
     * Each octet must be represented in the input string with 2 hexadecimal characters.
     * If a separator is specified, the octets in input string must be separated using
     *  exactly 1 separator. Only 1 leading or 1 trailing separators are allowed.
     * @param str Input character string
     * @param len Length of input string
     * @param sep Separator character used between octets.
     *  [-128..127]: expected separator (0: no separator is expected).
     *  Detect the separator if other value is given
     * @return 0 on success, negative if unhexify fails,
     *  positive if the result is not a multiple of Obj size
     */
    int unHexify(const char* str, unsigned int len, int sep = 255) {
	    clear();
	    DataBlock d;
	    bool ok = (sep < -128 || sep > 127) ? d.unHexify(str,len) :
		d.unHexify(str,len,(char)sep);
	    if (!(ok && (d.length() % sizeof(Obj)) == 0))
		return ok ? 1 : -1;
	    assign((Obj*)d.data(),d.length() / sizeof(Obj));
	    return 0;
	}

    /**
     * Compare with another vector. The vector item must implement the '==' operator
     * @param other Vector to compare with
     * @return This vector's length on success,
     *  this vector's length plus 1 if the 2 vectors don't have the same length,
     *  the index of the first different element otherwise
     */
    inline unsigned int compare(const SigProcVector& other)
	{ return compare(*this,other); }

    /**
     * Outputs data
     * @param len Data length
     * @param lineLen The number of elements to dump on each output
     * @param func Pointer to function who appends the object to a String
     * @param blockSep Optional text to be displayed before and after output
     * @param sep Vector elements separator
     * @param info Optional info
     */
    inline void output(unsigned int lineLen,
	String& (*func)(String& dest, const Obj& item, const char* sep),
	const char* blockSep, const char* sep = ",", const char* info = 0) const
	{ output(data(),length(),lineLen,func,blockSep,sep,info); }

    /**
     * Dump data to a string
     * @param buf Destination string
     * @param data Data to dump
     * @param len Data length
     * @param func Pointer to function who appends the object to a String
     * @param offs Offset in data to start
     * @param nDump The number of elements to dump (negative: all, 0: none)
     * @param sep Vector elements separator
     * @return Destination string address
     */
    static String& dump(String& buf, const Obj* data, unsigned int len,
	String& (*func)(String& dest, const Obj& item, const char* sep),
	unsigned int offs = 0, int nDump = -1, const char* sep = ",") {
	    if (!(func && data && len) || offs >= len)
		return buf;
	    if (nDump < 0)
		len -= offs;
	    else
		len = SigProcUtils::min(len - offs,(unsigned int)nDump);
	    String localBuf;
	    for (data += offs; len; len--, data++)
		(*func)(localBuf,*data,sep);
	    return buf.append(localBuf);
	}

    /**
     * Outputs data
     * @param data Data to dump
     * @param len Data length
     * @param lineLen The number of elements to dump on each output
     * @param func Pointer to function who appends the object to a String
     * @param blockSep Optional text to be displayed before and after output
     * @param sep Vector elements separator
     * @param info Optional info
     */
    static void output(const Obj* data, unsigned int len, unsigned int lineLen,
	String& (*func)(String& dest, const Obj& item, const char* sep),
	const char* blockSep, const char* sep = ",", const char* info = 0) {
	    if (!(func && data && len && lineLen))
		return;
	    if (lineLen > len)
		lineLen = len;
	    if (info)
		Output("%s%s",info,TelEngine::c_safe(blockSep));
	    else if (blockSep)
		Output("%s",blockSep);
	    while (len) {
		if (len < lineLen)
		    lineLen = len;
		String tmp;
		dump(tmp,data,lineLen,func,0,-1,sep);
		data += lineLen;
		len -= lineLen;
		Output("%s%s",tmp.c_str(),(len ? sep : ""));
	    }
	    if (blockSep)
		Output("%s",blockSep);
	}

    /**
     * Compare 2 vectors. The vector item must implement the '==' operator
     * @param v1 First vector
     * @param v2 Second vector
     * @return v1 length on success, v1 length plus 1 if the 2 vectors don't have the same length,
     *  the index of the first different element otherwise
     */
    static unsigned int compare(const SigProcVector& v1, const SigProcVector& v2) {
	    if (v1.length() != v2.length())
		return v1.length() + 1;
	    unsigned int i = 0;
	    const Obj* data1 = v1.data();
	    const Obj* data2 = v2.data();
	    while (i < v1.length() && *data1++ == *data2++)
		i++;
	    return i;
	}

    /**
     * Create a vector from a string
     * @param input The input string
     * @param parseCallback Callback method to parse the data.
     * @return True if the data can be parsed.
     */
    bool parse(const String& input,void (*callbackParse)(Obj& obj, const String& data)) {
	ObjList* split = input.split(',',false);
	if (!split)
	    return false;
	resize(split->count());
	Obj* data = m_data;
	for (ObjList* o = split->skipNull();o;o = o->skipNext(),data++) {
	    String* srep = static_cast<String*>(o->get());
	    callbackParse(*data,*srep);
	}
	TelEngine::destruct(split);
	return true;
    }

    /**
     * Fill a vector
     * @param dest Destination buffer
     * @param destLen Destination buffer length
     * @param value Value to fill with
     * @param offs Optional offset in destination vector
     * @return The number of elements set
     */
    static unsigned int fill(Obj* dest, unsigned int destLen, const Obj& value,
	unsigned int offs = 0) {
	    if (!dest || offs >= destLen)
		return 0;
	    unsigned int room = destLen - offs;
	    dest += offs;
	    for (unsigned int n = room; n; --n, ++dest)
		*dest = value;
	    return room;
	}

protected:
    /**
     * Copy elements from another vector
     * @param dest Destination buffer
     * @param destLen Destination buffer length
     * @param ptr Pointer to data to copy
     * @param n The number of bytes to copy
     * @param offs Optional offset in our vector
     * @return The number of elements copied
     */
    virtual unsigned int copy(Obj* dest, unsigned int destLen,
	const Obj* ptr, unsigned int n, unsigned int offs = 0) {
	    if (hardArrayOps)
		return SigProcUtils::copy(dest,destLen,ptr,n,sizeof(Obj),offs);
	    if (!(dest && destLen && ptr && n) || offs >= destLen)
		return 0;
	    n = SigProcUtils::min(destLen - offs,n);
	    dest += offs;
	    for (unsigned int i = n; i; --i, ++dest, ++ptr)
		*dest = *ptr;
	    return n;
	}

    /**
     * Reset a vector
     * @param dest Destination buffer
     * @param destLen Destination buffer length
     * @param offs Optional offset in vector
     * @return The number of elements reset
     */
    virtual unsigned int reset(Obj* dest, unsigned int destLen, unsigned int offs = 0) {
	    if (hardArrayOps)
		return SigProcUtils::bzero(dest,destLen,sizeof(Obj),offs);
	    Obj value;
	    return fill(dest,destLen,value,offs);
	}

private:
     Obj* m_data;
     unsigned int m_length;
};


typedef SigProcVector<Complex,false> ComplexVector;
typedef SigProcVector<float> FloatVector;
typedef SigProcVector<int> IntVector;
typedef SigProcVector<ComplexVector,false,false> ComplexVectorVector;


/**
 * Store objects to avoid re-alloc/re-init for large objects.
 * Template object must inherit GenObject
 * @short Object store/factory
 */
template <class Obj> class ObjStore : protected String
{
public:
    /**
     * Constructor
     * @param maxLen Maximum number of objects to store
     * @param name Store name
     */
    inline ObjStore(unsigned int maxLen, const char* name)
	: m_name(name), m_mutex(false,m_name.c_str()),
#ifdef SIGPROC_OBJ_STORE_DISABLE
	m_max(0),
#else
	m_max(maxLen),
#endif
	m_count(0),
	m_requested(0), m_created(0), m_returned(0), m_deleted(0)
	{}

    /**
     * Destructor
     */
    ~ObjStore() {
	    if (m_requested)
		printStatistics();
	}
	
    /**
     * Print statistics
     */
    inline void printStatistics() {
#ifdef SIGPROC_OBJ_STORE_DEBUG
	    Output("ObjStore(%s) created=" FMT64U "/" FMT64U " deleted=" FMT64U "/" FMT64U,
		m_name.c_str(),m_created,m_requested,m_deleted,m_returned);
#endif
	}

    /**
     * Retrieve an object from store
     * @return Valid pointer (may be a newly created one)
     */
    inline Obj* get() {
#ifdef SIGPROC_OBJ_STORE_DEBUG
	    m_requested++;
#endif
	    Lock lck(m_mutex);
	    Obj* o = static_cast<Obj*>(m_list.remove(false));
	    if (o) {
		m_count--;
		return o;
	    }
#ifdef SIGPROC_OBJ_STORE_DEBUG
	    m_created++;
#endif
	    return new Obj;
	}

    /**
     * Store an object
     * @param o Object to store
     */
    inline void store(Obj*& o) {
	    if (!o)
		return;
#ifdef SIGPROC_OBJ_STORE_DEBUG
	    m_returned++;
#endif
	    Lock lck(m_mutex);
	    if (m_count < m_max) {
		m_list.insert(o);
		m_count++;
		o = 0;
		return;
	    }
#ifdef SIGPROC_OBJ_STORE_DEBUG
	    m_deleted++;
#endif
	    TelEngine::destruct(o);
	}

private:
    String m_name;
    Mutex m_mutex;
    ObjList m_list;
    unsigned int m_max;
    unsigned int m_count;
    uint64_t m_requested;
    uint64_t m_created;
    uint64_t m_returned;
    uint64_t m_deleted;
};


/**
 * @short Signal processing library
 * NOTE: Class methods are not thread safe
 */
class SignalProcessing
{
public:
    /**
     * Laurent Pulse Approximation table to use when generating the vector
     */
    enum LaurentPATable {
	LaurentPADef = 0,
	LaurentPANone
    };
	
    /**
     * Constructor
     */
    SignalProcessing();

    /**
     * Retrieve the length of modulated data (BITS_PER_TIMESLOT multiplied by
     *  oversampling value)
     * This value is the length of a GSM slot sampled at 'oversample'
     * @return Length of modulated data
     */
    inline unsigned int gsmSlotLen() const
	{ return m_gsmSlotLen; }

    /**
     * Convert a timestamp to timeslots
     * @param ts Timestamp to convert
     * @return The number of timeslots in given timestamp
     */
    inline uint64_t ts2slots(uint64_t ts) const
	{ return ts / m_gsmSlotLen; }

    /**
     * Convert an absolute timeslots value to timestamp
     * @param timeslots Value to convert
     * @return The data timestamp
     */
    inline uint64_t slots2ts(uint64_t timeslots) const
	{ return timeslots * m_gsmSlotLen; }

    /**
     * Round up a given timestamp to slot boundary
     * @param ts Timestamp to adjust
     */
    inline void alignTs(uint64_t& ts) const
	{ ts = (ts2slots(ts) + 1) * m_gsmSlotLen; }

    /**
     * Retrieve the Laurent Pulse Approximation vector
     * @return Laurent Pulse Approximation vector
     */
    inline const FloatVector& laurentPA() const
	{ return m_laurentPA; }

    /**
     * Retrieve ARFCN frequency shifting vector
     * @param arfcn Requested ARFCN number
     * @return Requested vector, 0 if given 'arfcn' don't exist
     */
    inline const ComplexVector* arfcnFS(unsigned int arfcn) const
	{ return (arfcn < m_arfcnFS.length()) ? (m_arfcnFS.data() + arfcn) : 0; }

    /**
     * Initialize internal data
     * @param oversample The oversampling value of the transceiver
     * @param arfcns The number of arfcns used by the transceiver
     * @param lpaTbl Laurent Pulse Approximation table to use
     */
    void initialize(unsigned int oversample, unsigned int arfcns,
	LaurentPATable lpaTbl = LaurentPADef);

    /**
     * Modulate bits using optimized algorithm
     * @param out Destination vector for modulated data
     * @param bits Input bits (it will be used as received, no 0/1 checking is done)
     * @param len Input bits buffer length
     * @param tmpV Optional temporary buffer (used to avoid re-allocating memory)
     * @param tmpW Optional temporary buffer (used to avoid re-allocating memory)
     */
    void modulate(ComplexVector& out, const uint8_t* bits, unsigned int len,
	FloatVector* tmpV = 0, ComplexVector* tmpW = 0) const;

    /**
     * Modulate bits using common convolution
     * @param out Destination vector for modulated data
     * @param bits Input bits (it will be used as received, no 0/1 checking is done)
     * @param len Input bits buffer length
     * @param tmpV Optional temporary buffer (used to avoid re-allocating memory)
     * @param tmpW Optional temporary buffer (used to avoid re-allocating memory)
     */
    void modulateCommon(ComplexVector& out, const uint8_t* bits, unsigned int len,
	FloatVector* tmpV = 0, ComplexVector* tmpW = 0) const;

    /**
     * Frequency shift ARFCN data (out[n] = data[n] * arfcnFS(arfcn)[n]).
     * Multiply data with ARFCN shifting values and put the result in output
     * @param out Output vector. Data must be already allocated
     * @param data Modulated data
     * @param arfcn Requested ARFCN
     */
    inline void freqShift(ComplexVector& out, const ComplexVector& data,
	    unsigned int arfcn) const {
	    const ComplexVector* fs = arfcnFS(arfcn);
	    if (fs)
		Complex::multiply(out.data(),out.length(),
		    data.data(),data.length(),fs->data(),fs->length());
	}

    /**
     * Frequency shift ARFCN data.
     * Multiply data with ARFCN shifting values and put the result in output
     * @param data Modulated data
     * @param arfcn Requested ARFCN
     */
    inline void freqShift(ComplexVector& data, unsigned int arfcn) const
	{ freqShift(data,data,arfcn); }

    /**
     * Sum the ARFCN frequency shift vectors
     * out[n] = SUM(i=0..k)(m_arfcnFS[i][n])
     * @param out Output vector; it will be resized to requested length
     * @param arfcnMask ARFCNs to use. A bit set will indicate the ARFCN to
     *  use (e.g. the value 0x05 - binary 101 - would sum ARFCN 0 and 2)
     */
    void sumFreqShift(ComplexVector& out, unsigned int arfcnMask = 0xffffffff) const;

    /**
     * Retrieve the length of modulated data (BITS_PER_TIMESLOT multiplied by
     *  oversampling value)
     * This value is the length of a GSM slot sampled at 'oversample'
     * @param oversample The oversample value
     * @return Length of modulated data
     */
    static inline unsigned int gsmSlotLen(unsigned int oversample)
	{ return (unsigned int)::ceil(BITS_PER_TIMESLOT * oversample); }

    /**
     * Generate Laurent pulse aproximation vector
     * @param out The output vector. It will be resized to required length
     * @param lpaTbl The table to use to generate
     * @param oversample The oversample value used to generate the vector.
     *  This parameter is ignored if a valid index (less then LaurentPANone)
     */
    static void generateLaurentPulseAproximation(FloatVector& out,
	LaurentPATable lpaTbl = LaurentPADef, unsigned int oversample = 1);

    /**
     * Generate ARFCNs frequency shifting vectors
     * @param out The output vector. It will be resized to required length
     * @param arfcns The number of arfcns
     * @param oversample The oversample value used to generate the vectors
     */
    static void generateARFCNsFreqShift(ComplexVectorVector& out, unsigned int arfcns,
	unsigned int oversample = 1);

    /**
     * Calculate the convolution of 2 vectors.
     * Result: out[n] = SUM(i=0..Lp)(f[n + i] * g[Lp - 1 - i]).
     * Where Lp is the g vector length
     * @param out The output vector. It will be resized to 'f' length - Lp
     * @param f The first vector. It MUST be padded with (0,0) elements:
     *  leading: Lp / 2, trailing: Lp - Lp / 2
     * @param g The second vector
     */
    static void convolution(ComplexVector& out, const ComplexVector& f,
	const FloatVector& g);

    /**
     * Generate a frequency shifting vector
     * Each array element at index n will be set with e ^ (-j * n * val)
     *   where -j is the complex number (0,-1)
     * @param data Destination vector
     * @param val Frequency shifting value
     * @param setLen Optional vector length to set, 0 to use current array length
     */
    static void setFreqShifting(ComplexVector* data, float val, unsigned int setLen = 0);

    /**
     * Compute the power level from Complex array middle
     * @param data Array data start
     * @param len Array length
     * @param use Number of elements to use
     * @param param Parameter to multiply with the elements sum result
     * @param middle Optional the middle value
     * @return Power value
     */
    static inline float computePower(const Complex* data, unsigned int len,
	unsigned int use, float param = 1.0, int startFrom = -1) {
	    if (startFrom == -1)
		startFrom = (len - use) / 2;
	    if (data && len && (startFrom + use) <= len)
		return param * Complex::sumMulConj(data + startFrom,use);
	    return 0;
	}

    /**
     * Compute the power level from float array middle elements real part value
     * @param data Array data start
     * @param len Array length
     * @param middle Middle number of elements to use
     * @param param Parameter to multiply with the elements sum result
     * @return Power value
     */
    static float computePower(const float* data, unsigned int len,
	unsigned int middle, float param = 1.0);

    /**
     * Convert a linear power value to decibels, limit down to -120dB
     * @param power Linear power value
     * @return Power in decibels
     */
    static inline float power2db(float power)
	{ return (power >= 1e-12) ? (10.0 * ::log10f(power)) : -120.0; }

    /**
     * Convert a power value from decibels to linear
     * @param db Power in decibels
     * @return Linear value of power
     */
    static inline float db2power(float db)
	{ return ::powf(10.0, 0.1 * db); }

    /**
     * Sum 'out' with input 'data'.
     * Avoid 'out' data realloc (uses copy if it's the first time to sum)
     * @param out Destination vector
     * @param data Input vector
     * @param first IN/OUT first flag
     * @return True if output data was changed
     */
    static inline bool sum(ComplexVector& out, const ComplexVector& data,
	bool& first) {
	    if (!data.length())
		return false;
	    if (first) {
		if (out.length() == data.length())
		    out.copy(data);
		else
		    out = data;
		first = false;
	    }
	    else
		Complex::sum(out.data(),out.length(),data.data(),data.length());
	    return true;
	}

    /**
     * Multiply the input data with a -PI / 2 frequency shift array
     * @param array The input data to be multiplyed
     */
    static void applyMinusPIOverTwoFreqShift(ComplexVector& array);

    /**
     * Correlate two vectors.
     * @param out The output data
     * @param a1 First input vector
     * @param a1Start Position to start correlation in the first input vector
     * @param a1Len Data length to be used from first input array starting from a1Start index
     * @param a2 The second input vector
     */
    static void correlate(ComplexVector& out, const ComplexVector& a1, unsigned int a1Start, unsigned int a1Len, const FloatVector& a2);
private:
    // Init oversampling rate and related data
    void setOversample(unsigned int oversample);
    // Modulate: prepare W vector
    // Return true on success, false on failure (no input data)
    bool modulatePrepareW(ComplexVector& out, ComplexVector& tmpW,
	const uint8_t* b, unsigned int len, FloatVector* tmpV) const;

    unsigned int m_oversample;           // Oversampling
    unsigned int m_gsmSlotLen;           // GSM slot length
    FloatVector m_laurentPA;             // Laurent Pulse Approximation
    ComplexVectorVector m_arfcnFS;       // ARFCN Frequency Shifting
    // Modulate data
    unsigned int m_rampOffset;           // Power ramping offset
    unsigned int m_rampTrailIdx;         // Index of power ramping trailing edge
};

class Equalizer
{
public:
    enum {
	Default,
    };
    /**
     * "The equalizer" TODO ask David for a better description!
     * @param dataOut Data to be filled (u vector)
     * @param in1 First input data
     * @param in2 Second input data
     * @param mode The operating mode
     * @return True if the mode is recognized..
     */
    static bool equalize(FloatVector& dataOut, const ComplexVector& in1, const ComplexVector& in2, int in2len,int mode = Default) {
	if (mode != Default)
	    return false;
	defaultEqualize(dataOut,in1,in2,in2len);
	return true;
    }
private:
    static void defaultEqualize(FloatVector& dataOut, const ComplexVector& in1, const ComplexVector& in2, int in2len);
};


}; // namespace TelEngine

#endif // SIGPROC_H

/* vi: set ts=8 sw=4 sts=4 noet: */
