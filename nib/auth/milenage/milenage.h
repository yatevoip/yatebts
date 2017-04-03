/*-------------------------------------------------------------------
 *          Example algorithms f1, f1*, f2, f3, f4, f5, f5*
 *-------------------------------------------------------------------
 *
 *  A sample implementation of the example 3GPP authentication and
 *  key agreement functions f1, f1*, f2, f3, f4, f5 and f5*.  This is
 *  a byte-oriented implementation of the functions, and of the block
 *  cipher kernel function Rijndael.
 *
 *  This has been coded for clarity, not necessarily for efficiency.
 *
 *  The functions f2, f3, f4 and f5 share the same inputs and have
 *  been coded together as a single function.  f1, f1* and f5* are
 *  all coded separately.
 *
 * This code is extracted from ETSI TS 135 206
 *-----------------------------------------------------------------*/

#ifndef MILENAGE_H
#define MILENAGE_H

typedef unsigned char u8;


void f1Opc    ( u8 k[16], u8 rand[16], u8 sqn[6], u8 amf[2],
                u8 mac_a[8], u8 op[16], u8 opc );
void f2345Opc ( u8 k[16], u8 rand[16],
                u8 res[8], u8 ck[16], u8 ik[16], u8 ak[6], u8 op[16], u8 opc );
void f1starOpc( u8 k[16], u8 rand[16], u8 sqn[6], u8 amf[2],
                u8 mac_s[8], u8 op[16], u8 opc );
void f5starOpc( u8 k[16], u8 rand[16],
                u8 ak[6], u8 op[16], u8 opc );
void ComputeOPc( u8 op_c[16], u8 op[16] );

#define f1(k,rand,sqn,amf,mac_a,op) f1Opc(k,rand,sqn,amf,mac_a,op,0)
#define f2345(k,rand,res,ck,ik,ak,op) f2345Opc(k,rand,res,ck,ik,ak,op,0)
#define f1star(k,rand,sqn,amf,mac_s,op) f1starOpc(k,rand,sqn,amf,mac_s,op,0)
#define f5star(k,rand,ak,op) f5starOpc(k,rand,ak,op,0)

#endif
