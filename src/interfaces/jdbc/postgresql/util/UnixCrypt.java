package postgresql.util;

/**
 * This class provides us with the ability to encrypt passwords when sent
 * over the network stream
 *
 * <P>Contains static methods to encrypt and compare
 * passwords with Unix encrypted passwords.</P>
 *
 * <P>See <A HREF="http://www.zeh.com/local/jfd/crypt.html">
 * John Dumas's Java Crypt page</A> for the original source.</P>
 *
 * @author jdumas@zgs.com (John Dumas)
 */
public class UnixCrypt extends Object
{
  //
  // Null constructor - can't instantiate class
  private UnixCrypt()
  {
  }
  
  private static final char[] saltChars =
  ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./".toCharArray());
  
  private static final int ITERATIONS = 16;
  
  private static final int con_salt[] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 
    0x0A, 0x0B, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 
    0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 
    0x23, 0x24, 0x25, 0x20, 0x21, 0x22, 0x23, 0x24, 
    0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 
    0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 
    0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 
    0x3D, 0x3E, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 
  };
  
  private static final boolean shifts2[] =
  {
    false, false, true, true, true, true, true, true,
    false, true,  true, true, true, true, true, false
  };

   private static final int skb[][] =
   {
      {
         /* for C bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
         0x00000000, 0x00000010, 0x20000000, 0x20000010, 
         0x00010000, 0x00010010, 0x20010000, 0x20010010, 
         0x00000800, 0x00000810, 0x20000800, 0x20000810, 
         0x00010800, 0x00010810, 0x20010800, 0x20010810, 
         0x00000020, 0x00000030, 0x20000020, 0x20000030, 
         0x00010020, 0x00010030, 0x20010020, 0x20010030, 
         0x00000820, 0x00000830, 0x20000820, 0x20000830, 
         0x00010820, 0x00010830, 0x20010820, 0x20010830, 
         0x00080000, 0x00080010, 0x20080000, 0x20080010, 
         0x00090000, 0x00090010, 0x20090000, 0x20090010, 
         0x00080800, 0x00080810, 0x20080800, 0x20080810, 
         0x00090800, 0x00090810, 0x20090800, 0x20090810, 
         0x00080020, 0x00080030, 0x20080020, 0x20080030, 
         0x00090020, 0x00090030, 0x20090020, 0x20090030, 
         0x00080820, 0x00080830, 0x20080820, 0x20080830, 
         0x00090820, 0x00090830, 0x20090820, 0x20090830, 
      },
      {
         /* for C bits (numbered as per FIPS 46) 7 8 10 11 12 13 */
         0x00000000, 0x02000000, 0x00002000, 0x02002000, 
         0x00200000, 0x02200000, 0x00202000, 0x02202000, 
         0x00000004, 0x02000004, 0x00002004, 0x02002004, 
         0x00200004, 0x02200004, 0x00202004, 0x02202004, 
         0x00000400, 0x02000400, 0x00002400, 0x02002400, 
         0x00200400, 0x02200400, 0x00202400, 0x02202400, 
         0x00000404, 0x02000404, 0x00002404, 0x02002404, 
         0x00200404, 0x02200404, 0x00202404, 0x02202404, 
         0x10000000, 0x12000000, 0x10002000, 0x12002000, 
         0x10200000, 0x12200000, 0x10202000, 0x12202000, 
         0x10000004, 0x12000004, 0x10002004, 0x12002004, 
         0x10200004, 0x12200004, 0x10202004, 0x12202004, 
         0x10000400, 0x12000400, 0x10002400, 0x12002400, 
         0x10200400, 0x12200400, 0x10202400, 0x12202400, 
         0x10000404, 0x12000404, 0x10002404, 0x12002404, 
         0x10200404, 0x12200404, 0x10202404, 0x12202404, 
      },
      {
         /* for C bits (numbered as per FIPS 46) 14 15 16 17 19 20 */
         0x00000000, 0x00000001, 0x00040000, 0x00040001, 
         0x01000000, 0x01000001, 0x01040000, 0x01040001, 
         0x00000002, 0x00000003, 0x00040002, 0x00040003, 
         0x01000002, 0x01000003, 0x01040002, 0x01040003, 
         0x00000200, 0x00000201, 0x00040200, 0x00040201, 
         0x01000200, 0x01000201, 0x01040200, 0x01040201, 
         0x00000202, 0x00000203, 0x00040202, 0x00040203, 
         0x01000202, 0x01000203, 0x01040202, 0x01040203, 
         0x08000000, 0x08000001, 0x08040000, 0x08040001, 
         0x09000000, 0x09000001, 0x09040000, 0x09040001, 
         0x08000002, 0x08000003, 0x08040002, 0x08040003, 
         0x09000002, 0x09000003, 0x09040002, 0x09040003, 
         0x08000200, 0x08000201, 0x08040200, 0x08040201, 
         0x09000200, 0x09000201, 0x09040200, 0x09040201, 
         0x08000202, 0x08000203, 0x08040202, 0x08040203, 
         0x09000202, 0x09000203, 0x09040202, 0x09040203, 
      },
      {
         /* for C bits (numbered as per FIPS 46) 21 23 24 26 27 28 */
         0x00000000, 0x00100000, 0x00000100, 0x00100100, 
         0x00000008, 0x00100008, 0x00000108, 0x00100108, 
         0x00001000, 0x00101000, 0x00001100, 0x00101100, 
         0x00001008, 0x00101008, 0x00001108, 0x00101108, 
         0x04000000, 0x04100000, 0x04000100, 0x04100100, 
         0x04000008, 0x04100008, 0x04000108, 0x04100108, 
         0x04001000, 0x04101000, 0x04001100, 0x04101100, 
         0x04001008, 0x04101008, 0x04001108, 0x04101108, 
         0x00020000, 0x00120000, 0x00020100, 0x00120100, 
         0x00020008, 0x00120008, 0x00020108, 0x00120108, 
         0x00021000, 0x00121000, 0x00021100, 0x00121100, 
         0x00021008, 0x00121008, 0x00021108, 0x00121108, 
         0x04020000, 0x04120000, 0x04020100, 0x04120100, 
         0x04020008, 0x04120008, 0x04020108, 0x04120108, 
         0x04021000, 0x04121000, 0x04021100, 0x04121100, 
         0x04021008, 0x04121008, 0x04021108, 0x04121108, 
      },
      {
         /* for D bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
         0x00000000, 0x10000000, 0x00010000, 0x10010000, 
         0x00000004, 0x10000004, 0x00010004, 0x10010004, 
         0x20000000, 0x30000000, 0x20010000, 0x30010000, 
         0x20000004, 0x30000004, 0x20010004, 0x30010004, 
         0x00100000, 0x10100000, 0x00110000, 0x10110000, 
         0x00100004, 0x10100004, 0x00110004, 0x10110004, 
         0x20100000, 0x30100000, 0x20110000, 0x30110000, 
         0x20100004, 0x30100004, 0x20110004, 0x30110004, 
         0x00001000, 0x10001000, 0x00011000, 0x10011000, 
         0x00001004, 0x10001004, 0x00011004, 0x10011004, 
         0x20001000, 0x30001000, 0x20011000, 0x30011000, 
         0x20001004, 0x30001004, 0x20011004, 0x30011004, 
         0x00101000, 0x10101000, 0x00111000, 0x10111000, 
         0x00101004, 0x10101004, 0x00111004, 0x10111004, 
         0x20101000, 0x30101000, 0x20111000, 0x30111000, 
         0x20101004, 0x30101004, 0x20111004, 0x30111004, 
      },
      {
         /* for D bits (numbered as per FIPS 46) 8 9 11 12 13 14 */
         0x00000000, 0x08000000, 0x00000008, 0x08000008, 
         0x00000400, 0x08000400, 0x00000408, 0x08000408, 
         0x00020000, 0x08020000, 0x00020008, 0x08020008, 
         0x00020400, 0x08020400, 0x00020408, 0x08020408, 
         0x00000001, 0x08000001, 0x00000009, 0x08000009, 
         0x00000401, 0x08000401, 0x00000409, 0x08000409, 
         0x00020001, 0x08020001, 0x00020009, 0x08020009, 
         0x00020401, 0x08020401, 0x00020409, 0x08020409, 
         0x02000000, 0x0A000000, 0x02000008, 0x0A000008, 
         0x02000400, 0x0A000400, 0x02000408, 0x0A000408, 
         0x02020000, 0x0A020000, 0x02020008, 0x0A020008, 
         0x02020400, 0x0A020400, 0x02020408, 0x0A020408, 
         0x02000001, 0x0A000001, 0x02000009, 0x0A000009, 
         0x02000401, 0x0A000401, 0x02000409, 0x0A000409, 
         0x02020001, 0x0A020001, 0x02020009, 0x0A020009, 
         0x02020401, 0x0A020401, 0x02020409, 0x0A020409, 
      },
      {
         /* for D bits (numbered as per FIPS 46) 16 17 18 19 20 21 */
         0x00000000, 0x00000100, 0x00080000, 0x00080100, 
         0x01000000, 0x01000100, 0x01080000, 0x01080100, 
         0x00000010, 0x00000110, 0x00080010, 0x00080110, 
         0x01000010, 0x01000110, 0x01080010, 0x01080110, 
         0x00200000, 0x00200100, 0x00280000, 0x00280100, 
         0x01200000, 0x01200100, 0x01280000, 0x01280100, 
         0x00200010, 0x00200110, 0x00280010, 0x00280110, 
         0x01200010, 0x01200110, 0x01280010, 0x01280110, 
         0x00000200, 0x00000300, 0x00080200, 0x00080300, 
         0x01000200, 0x01000300, 0x01080200, 0x01080300, 
         0x00000210, 0x00000310, 0x00080210, 0x00080310, 
         0x01000210, 0x01000310, 0x01080210, 0x01080310, 
         0x00200200, 0x00200300, 0x00280200, 0x00280300, 
         0x01200200, 0x01200300, 0x01280200, 0x01280300, 
         0x00200210, 0x00200310, 0x00280210, 0x00280310, 
         0x01200210, 0x01200310, 0x01280210, 0x01280310, 
      },
      {
         /* for D bits (numbered as per FIPS 46) 22 23 24 25 27 28 */
         0x00000000, 0x04000000, 0x00040000, 0x04040000, 
         0x00000002, 0x04000002, 0x00040002, 0x04040002, 
         0x00002000, 0x04002000, 0x00042000, 0x04042000, 
         0x00002002, 0x04002002, 0x00042002, 0x04042002, 
         0x00000020, 0x04000020, 0x00040020, 0x04040020, 
         0x00000022, 0x04000022, 0x00040022, 0x04040022, 
         0x00002020, 0x04002020, 0x00042020, 0x04042020, 
         0x00002022, 0x04002022, 0x00042022, 0x04042022, 
         0x00000800, 0x04000800, 0x00040800, 0x04040800, 
         0x00000802, 0x04000802, 0x00040802, 0x04040802, 
         0x00002800, 0x04002800, 0x00042800, 0x04042800, 
         0x00002802, 0x04002802, 0x00042802, 0x04042802, 
         0x00000820, 0x04000820, 0x00040820, 0x04040820, 
         0x00000822, 0x04000822, 0x00040822, 0x04040822, 
         0x00002820, 0x04002820, 0x00042820, 0x04042820, 
         0x00002822, 0x04002822, 0x00042822, 0x04042822, 
      },
   };

   private static final int SPtrans[][] =
   {
      {
         /* nibble 0 */
         0x00820200, 0x00020000, 0x80800000, 0x80820200,
         0x00800000, 0x80020200, 0x80020000, 0x80800000,
         0x80020200, 0x00820200, 0x00820000, 0x80000200,
         0x80800200, 0x00800000, 0x00000000, 0x80020000,
         0x00020000, 0x80000000, 0x00800200, 0x00020200,
         0x80820200, 0x00820000, 0x80000200, 0x00800200,
         0x80000000, 0x00000200, 0x00020200, 0x80820000,
         0x00000200, 0x80800200, 0x80820000, 0x00000000,
         0x00000000, 0x80820200, 0x00800200, 0x80020000,
         0x00820200, 0x00020000, 0x80000200, 0x00800200,
         0x80820000, 0x00000200, 0x00020200, 0x80800000,
         0x80020200, 0x80000000, 0x80800000, 0x00820000,
         0x80820200, 0x00020200, 0x00820000, 0x80800200,
         0x00800000, 0x80000200, 0x80020000, 0x00000000,
         0x00020000, 0x00800000, 0x80800200, 0x00820200,
         0x80000000, 0x80820000, 0x00000200, 0x80020200,
      },
      {
         /* nibble 1 */
         0x10042004, 0x00000000, 0x00042000, 0x10040000,
         0x10000004, 0x00002004, 0x10002000, 0x00042000,
         0x00002000, 0x10040004, 0x00000004, 0x10002000,
         0x00040004, 0x10042000, 0x10040000, 0x00000004,
         0x00040000, 0x10002004, 0x10040004, 0x00002000,
         0x00042004, 0x10000000, 0x00000000, 0x00040004,
         0x10002004, 0x00042004, 0x10042000, 0x10000004,
         0x10000000, 0x00040000, 0x00002004, 0x10042004,
         0x00040004, 0x10042000, 0x10002000, 0x00042004,
         0x10042004, 0x00040004, 0x10000004, 0x00000000,
         0x10000000, 0x00002004, 0x00040000, 0x10040004,
         0x00002000, 0x10000000, 0x00042004, 0x10002004,
         0x10042000, 0x00002000, 0x00000000, 0x10000004,
         0x00000004, 0x10042004, 0x00042000, 0x10040000,
         0x10040004, 0x00040000, 0x00002004, 0x10002000,
         0x10002004, 0x00000004, 0x10040000, 0x00042000,
      },
      {
         /* nibble 2 */
         0x41000000, 0x01010040, 0x00000040, 0x41000040,
         0x40010000, 0x01000000, 0x41000040, 0x00010040,
         0x01000040, 0x00010000, 0x01010000, 0x40000000,
         0x41010040, 0x40000040, 0x40000000, 0x41010000,
         0x00000000, 0x40010000, 0x01010040, 0x00000040,
         0x40000040, 0x41010040, 0x00010000, 0x41000000,
         0x41010000, 0x01000040, 0x40010040, 0x01010000,
         0x00010040, 0x00000000, 0x01000000, 0x40010040,
         0x01010040, 0x00000040, 0x40000000, 0x00010000,
         0x40000040, 0x40010000, 0x01010000, 0x41000040,
         0x00000000, 0x01010040, 0x00010040, 0x41010000,
         0x40010000, 0x01000000, 0x41010040, 0x40000000,
         0x40010040, 0x41000000, 0x01000000, 0x41010040,
         0x00010000, 0x01000040, 0x41000040, 0x00010040,
         0x01000040, 0x00000000, 0x41010000, 0x40000040,
         0x41000000, 0x40010040, 0x00000040, 0x01010000,
      },
      {
         /* nibble 3 */
         0x00100402, 0x04000400, 0x00000002, 0x04100402,
         0x00000000, 0x04100000, 0x04000402, 0x00100002,
         0x04100400, 0x04000002, 0x04000000, 0x00000402,
         0x04000002, 0x00100402, 0x00100000, 0x04000000,
         0x04100002, 0x00100400, 0x00000400, 0x00000002,
         0x00100400, 0x04000402, 0x04100000, 0x00000400,
         0x00000402, 0x00000000, 0x00100002, 0x04100400,
         0x04000400, 0x04100002, 0x04100402, 0x00100000,
         0x04100002, 0x00000402, 0x00100000, 0x04000002,
         0x00100400, 0x04000400, 0x00000002, 0x04100000,
         0x04000402, 0x00000000, 0x00000400, 0x00100002,
         0x00000000, 0x04100002, 0x04100400, 0x00000400,
         0x04000000, 0x04100402, 0x00100402, 0x00100000,
         0x04100402, 0x00000002, 0x04000400, 0x00100402,
         0x00100002, 0x00100400, 0x04100000, 0x04000402,
         0x00000402, 0x04000000, 0x04000002, 0x04100400,
      },
      {
         /* nibble 4 */
         0x02000000, 0x00004000, 0x00000100, 0x02004108,
         0x02004008, 0x02000100, 0x00004108, 0x02004000,
         0x00004000, 0x00000008, 0x02000008, 0x00004100,
         0x02000108, 0x02004008, 0x02004100, 0x00000000,
         0x00004100, 0x02000000, 0x00004008, 0x00000108,
         0x02000100, 0x00004108, 0x00000000, 0x02000008,
         0x00000008, 0x02000108, 0x02004108, 0x00004008,
         0x02004000, 0x00000100, 0x00000108, 0x02004100,
         0x02004100, 0x02000108, 0x00004008, 0x02004000,
         0x00004000, 0x00000008, 0x02000008, 0x02000100,
         0x02000000, 0x00004100, 0x02004108, 0x00000000,
         0x00004108, 0x02000000, 0x00000100, 0x00004008,
         0x02000108, 0x00000100, 0x00000000, 0x02004108,
         0x02004008, 0x02004100, 0x00000108, 0x00004000,
         0x00004100, 0x02004008, 0x02000100, 0x00000108,
         0x00000008, 0x00004108, 0x02004000, 0x02000008,
      },
      {
         /* nibble 5 */
         0x20000010, 0x00080010, 0x00000000, 0x20080800,
         0x00080010, 0x00000800, 0x20000810, 0x00080000,
         0x00000810, 0x20080810, 0x00080800, 0x20000000,
         0x20000800, 0x20000010, 0x20080000, 0x00080810,
         0x00080000, 0x20000810, 0x20080010, 0x00000000,
         0x00000800, 0x00000010, 0x20080800, 0x20080010,
         0x20080810, 0x20080000, 0x20000000, 0x00000810,
         0x00000010, 0x00080800, 0x00080810, 0x20000800,
         0x00000810, 0x20000000, 0x20000800, 0x00080810,
         0x20080800, 0x00080010, 0x00000000, 0x20000800,
         0x20000000, 0x00000800, 0x20080010, 0x00080000,
         0x00080010, 0x20080810, 0x00080800, 0x00000010,
         0x20080810, 0x00080800, 0x00080000, 0x20000810,
         0x20000010, 0x20080000, 0x00080810, 0x00000000,
         0x00000800, 0x20000010, 0x20000810, 0x20080800,
         0x20080000, 0x00000810, 0x00000010, 0x20080010,
      },
      {
         /* nibble 6 */
         0x00001000, 0x00000080, 0x00400080, 0x00400001,
         0x00401081, 0x00001001, 0x00001080, 0x00000000,
         0x00400000, 0x00400081, 0x00000081, 0x00401000,
         0x00000001, 0x00401080, 0x00401000, 0x00000081,
         0x00400081, 0x00001000, 0x00001001, 0x00401081,
         0x00000000, 0x00400080, 0x00400001, 0x00001080,
         0x00401001, 0x00001081, 0x00401080, 0x00000001,
         0x00001081, 0x00401001, 0x00000080, 0x00400000,
         0x00001081, 0x00401000, 0x00401001, 0x00000081,
         0x00001000, 0x00000080, 0x00400000, 0x00401001,
         0x00400081, 0x00001081, 0x00001080, 0x00000000,
         0x00000080, 0x00400001, 0x00000001, 0x00400080,
         0x00000000, 0x00400081, 0x00400080, 0x00001080,
         0x00000081, 0x00001000, 0x00401081, 0x00400000,
         0x00401080, 0x00000001, 0x00001001, 0x00401081,
         0x00400001, 0x00401080, 0x00401000, 0x00001001,
      },
      {
         /* nibble 7 */
         0x08200020, 0x08208000, 0x00008020, 0x00000000,
         0x08008000, 0x00200020, 0x08200000, 0x08208020,
         0x00000020, 0x08000000, 0x00208000, 0x00008020,
         0x00208020, 0x08008020, 0x08000020, 0x08200000,
         0x00008000, 0x00208020, 0x00200020, 0x08008000,
         0x08208020, 0x08000020, 0x00000000, 0x00208000,
         0x08000000, 0x00200000, 0x08008020, 0x08200020,
         0x00200000, 0x00008000, 0x08208000, 0x00000020,
         0x00200000, 0x00008000, 0x08000020, 0x08208020,
         0x00008020, 0x08000000, 0x00000000, 0x00208000,
         0x08200020, 0x08008020, 0x08008000, 0x00200020,
         0x08208000, 0x00000020, 0x00200020, 0x08008000,
         0x08208020, 0x00200000, 0x08200000, 0x08000020,
         0x00208000, 0x00008020, 0x08008020, 0x08200000,
         0x00000020, 0x08208000, 0x00208020, 0x00000000,
         0x08000000, 0x08200020, 0x00008000, 0x00208020
      }
   };

   private static final int cov_2char[] =
   {
      0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 
      0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 
      0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 
      0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 
      0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x61, 0x62, 
      0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 
      0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 
      0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A
   };

   private static final int byteToUnsigned(byte b)
   {
      int value = (int)b;

      return(value >= 0 ? value : value + 256);
   }

   private static int fourBytesToInt(byte b[], int offset)
   {
      int value;

      value  =  byteToUnsigned(b[offset++]);
      value |= (byteToUnsigned(b[offset++]) <<  8);
      value |= (byteToUnsigned(b[offset++]) << 16);
      value |= (byteToUnsigned(b[offset++]) << 24);

      return(value);
   }

   private static final void intToFourBytes(int iValue, byte b[], int offset)
   {
      b[offset++] = (byte)((iValue)        & 0xff);
      b[offset++] = (byte)((iValue >>> 8 ) & 0xff);
      b[offset++] = (byte)((iValue >>> 16) & 0xff);
      b[offset++] = (byte)((iValue >>> 24) & 0xff);
   }

   private static final void PERM_OP(int a, int b, int n, int m, int results[])
   {
      int t;

      t = ((a >>> n) ^ b) & m;
      a ^= t << n;
      b ^= t;

      results[0] = a;
      results[1] = b;
   }

   private static final int HPERM_OP(int a, int n, int m)
   {
      int t;

      t = ((a << (16 - n)) ^ a) & m;
      a = a ^ t ^ (t >>> (16 - n));

      return(a);
   }

   private static int [] des_set_key(byte key[])
   {
      int schedule[] = new int[ITERATIONS * 2];

      int c = fourBytesToInt(key, 0);
      int d = fourBytesToInt(key, 4);

      int results[] = new int[2];

      PERM_OP(d, c, 4, 0x0f0f0f0f, results);
      d = results[0]; c = results[1];

      c = HPERM_OP(c, -2, 0xcccc0000);
      d = HPERM_OP(d, -2, 0xcccc0000);

      PERM_OP(d, c, 1, 0x55555555, results);
      d = results[0]; c = results[1];

      PERM_OP(c, d, 8, 0x00ff00ff, results);
      c = results[0]; d = results[1];

      PERM_OP(d, c, 1, 0x55555555, results);
      d = results[0]; c = results[1];

      d = (((d & 0x000000ff) <<  16) |  (d & 0x0000ff00)     |
           ((d & 0x00ff0000) >>> 16) | ((c & 0xf0000000) >>> 4));
      c &= 0x0fffffff;

      int s, t;
      int j = 0;

      for(int i = 0; i < ITERATIONS; i ++)
      {
         if(shifts2[i])
         {
            c = (c >>> 2) | (c << 26);
            d = (d >>> 2) | (d << 26);
         }
         else
         {
            c = (c >>> 1) | (c << 27);
            d = (d >>> 1) | (d << 27);
         }

         c &= 0x0fffffff;
         d &= 0x0fffffff;

         s = skb[0][ (c       ) & 0x3f                       ]|
             skb[1][((c >>>  6) & 0x03) | ((c >>>  7) & 0x3c)]|
             skb[2][((c >>> 13) & 0x0f) | ((c >>> 14) & 0x30)]|
             skb[3][((c >>> 20) & 0x01) | ((c >>> 21) & 0x06) |
                                          ((c >>> 22) & 0x38)];

         t = skb[4][ (d     )  & 0x3f                       ]|
             skb[5][((d >>> 7) & 0x03) | ((d >>>  8) & 0x3c)]|
             skb[6][ (d >>>15) & 0x3f                       ]|
             skb[7][((d >>>21) & 0x0f) | ((d >>> 22) & 0x30)];

         schedule[j++] = ((t <<  16) | (s & 0x0000ffff)) & 0xffffffff;
         s             = ((s >>> 16) | (t & 0xffff0000));

         s             = (s << 4) | (s >>> 28);
         schedule[j++] = s & 0xffffffff;
      }
      return(schedule);
   }

   private static final int D_ENCRYPT
   (
      int L, int R, int S, int E0, int E1, int s[]
   )
   {
      int t, u, v;

      v = R ^ (R >>> 16);
      u = v & E0;
      v = v & E1;
      u = (u ^ (u << 16)) ^ R ^ s[S];
      t = (v ^ (v << 16)) ^ R ^ s[S + 1];
      t = (t >>> 4) | (t << 28);

      L ^= SPtrans[1][(t       ) & 0x3f] |
           SPtrans[3][(t >>>  8) & 0x3f] |
           SPtrans[5][(t >>> 16) & 0x3f] |
           SPtrans[7][(t >>> 24) & 0x3f] |
           SPtrans[0][(u       ) & 0x3f] |
           SPtrans[2][(u >>>  8) & 0x3f] |
           SPtrans[4][(u >>> 16) & 0x3f] |
           SPtrans[6][(u >>> 24) & 0x3f];

      return(L);
   }

   private static final int [] body(int schedule[], int Eswap0, int Eswap1)
   {
      int left = 0;
      int right = 0;
      int t     = 0;

      for(int j = 0; j < 25; j ++)
      {
         for(int i = 0; i < ITERATIONS * 2; i += 4)
         {
            left  = D_ENCRYPT(left,  right, i,     Eswap0, Eswap1, schedule);
            right = D_ENCRYPT(right, left,  i + 2, Eswap0, Eswap1, schedule);
         }
         t     = left; 
         left  = right; 
         right = t;
      }

      t = right;

      right = (left >>> 1) | (left << 31);
      left  = (t    >>> 1) | (t    << 31);

      left  &= 0xffffffff;
      right &= 0xffffffff;

      int results[] = new int[2];

      PERM_OP(right, left, 1, 0x55555555, results); 
      right = results[0]; left = results[1];

      PERM_OP(left, right, 8, 0x00ff00ff, results); 
      left = results[0]; right = results[1];

      PERM_OP(right, left, 2, 0x33333333, results); 
      right = results[0]; left = results[1];

      PERM_OP(left, right, 16, 0x0000ffff, results);
      left = results[0]; right = results[1];

      PERM_OP(right, left, 4, 0x0f0f0f0f, results);
      right = results[0]; left = results[1];

      int out[] = new int[2];

      out[0] = left; out[1] = right;

      return(out);
   }

  /**
   * <P>Encrypt a password given the cleartext password and a "salt".</P>
   * @param salt A two-character string representing the salt used to
   * iterate the encryption engine in lots of different ways. If you
   * are generating a new encryption then this value should be
   * randomised.
   * @param original The password to be encrypted.
   * @return A string consisting of the 2-character salt followed by the
   * encrypted password.
   */
   public static final String crypt(String salt, String original)
   {
      while(salt.length() < 2)
         salt += "A";

      StringBuffer buffer = new StringBuffer("             ");

      char charZero = salt.charAt(0);
      char charOne  = salt.charAt(1);

      buffer.setCharAt(0, charZero);
      buffer.setCharAt(1, charOne);

      int Eswap0 = con_salt[(int)charZero];
      int Eswap1 = con_salt[(int)charOne] << 4;
 
      byte key[] = new byte[8];

      for(int i = 0; i < key.length; i ++)
         key[i] = (byte)0;

      for(int i = 0; i < key.length && i < original.length(); i ++)
      {
         int iChar = (int)original.charAt(i);

         key[i] = (byte)(iChar << 1);
      }

      int schedule[] = des_set_key(key);
      int out[]      = body(schedule, Eswap0, Eswap1);

      byte b[] = new byte[9];

      intToFourBytes(out[0], b, 0);
      intToFourBytes(out[1], b, 4);
      b[8] = 0;

      for(int i = 2, y = 0, u = 0x80; i < 13; i ++)
      {
         for(int j = 0, c = 0; j < 6; j ++)
         {
            c <<= 1;

            if(((int)b[y] & u) != 0)
               c |= 1;

            u >>>= 1;

            if(u == 0)
            {
               y++;
               u = 0x80;
            }
            buffer.setCharAt(i, (char)cov_2char[c]);
         }
      }
      return(buffer.toString());
   }
  
  /**
   * <P>Encrypt a password given the cleartext password. This method
   * generates a random salt using the 'java.util.Random' class.</P>
   * @param original The password to be encrypted.
   * @return A string consisting of the 2-character salt followed by the
   * encrypted password.
   */
  public static final String crypt(String original)
  {
    java.util.Random randomGenerator = new java.util.Random();
    int numSaltChars = saltChars.length;
    String salt;
    
    salt = (new StringBuffer()).append(saltChars[Math.abs(randomGenerator.nextInt()) % numSaltChars]).append(saltChars[Math.abs(randomGenerator.nextInt()) % numSaltChars]).toString();
    
    return crypt(salt, original);
  }
  
  /**
   * <P>Check that <I>enteredPassword</I> encrypts to
   * <I>encryptedPassword</I>.</P>
   * @param encryptedPassword The <I>encryptedPassword</I>. The first
   * two characters are assumed to be the salt. This string would
   * be the same as one found in a Unix <U>/etc/passwd</U> file.
   * @param enteredPassword The password as entered by the user (or
   * otherwise aquired).
   * @return <B>true</B> if the password should be considered correct.
   */
  public final static boolean matches(String encryptedPassword, String enteredPassword)
  {
    String salt = encryptedPassword.substring(0, 3);
    String newCrypt = crypt(salt, enteredPassword);
    
    return newCrypt.equals(encryptedPassword);
  }
}

