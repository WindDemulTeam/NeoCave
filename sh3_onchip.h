#pragma once

#include <cstdint>

#include "counters.h"

namespace sh3 {
#define PTEH OnchipRef32(0xFFFFFFF0)
#define PTEL OnchipRef32(0xFFFFFFF4)
#define TTB OnchipRef32(0xFFFFFFF8)
#define TEA OnchipRef32(0xFFFFFFFC)
#define MMUCR OnchipRef32(0xFFFFFFE0)
#define BASRA OnchipRef8(0xFFFFFFE4)
#define BASRB OnchipRef8(0xFFFFFFE8)
#define CCR OnchipRef32(0xFFFFFFEC)
#define CCR2 OnchipRef32(0xA40000B0)
#define TRA OnchipRef32(0xFFFFFFD0)
#define EXPEVT OnchipRef32(0xFFFFFFD4)
#define INTEVT OnchipRef32(0xFFFFFFD8)
#define BARA OnchipRef32(0xFFFFFFB0)
#define BAMRA OnchipRef32(0xFFFFFFB4)
#define BBRA OnchipRef16(0xFFFFFFB8)
#define BARB OnchipRef32(0xFFFFFFA0)
#define BAMRB OnchipRef32(0xFFFFFFA4)
#define BBRB OnchipRef16(0xFFFFFFA8)
#define BDRB OnchipRef32(0xFFFFFF90)
#define BDMRB OnchipRef32(0xFFFFFF94)
#define BRCR OnchipRef32(0xFFFFFF98)
#define BETR OnchipRef16(0xFFFFFF9C)
#define BRSR OnchipRef32(0xFFFFFFAC)
#define BRDR OnchipRef32(0xFFFFFFBC)
#define FRQCR OnchipRef16(0xFFFFFF80)
#define STBCR OnchipRef8(0xFFFFFF82)
#define STBCR2 OnchipRef8(0xFFFFFF88)
#define WTCNT OnchipRef8(0xFFFFFF84)
#define WTCSR OnchipRef8(0xFFFFFF86)
#define BCR1 OnchipRef16(0xFFFFFF60)
#define BCR2 OnchipRef16(0xFFFFFF62)
#define WCR1 OnchipRef16(0xFFFFFF64)
#define WCR2 OnchipRef16(0xFFFFFF66)
#define MCR OnchipRef16(0xFFFFFF68)
#define PCR OnchipRef16(0xFFFFFF6C)
#define RTCSR OnchipRef16(0xFFFFFF6E)
#define RTCNT OnchipRef16(0xFFFFFF70)
#define RTCOR OnchipRef16(0xFFFFFF72)
#define RFCR OnchipRef16(0xFFFFFF74)
#define SDMR OnchipRef8(0xFFFFD000)
#define R64CNT OnchipRef8(0xFFFFFEC0)
#define RSECCNT OnchipRef8(0xFFFFFEC2)
#define RMINCNT OnchipRef8(0xFFFFFEC4)
#define RHRCNT OnchipRef8(0xFFFFFEC6)
#define RWKCNT OnchipRef8(0xFFFFFEC8)
#define RDAYCNT OnchipRef8(0xFFFFFECA)
#define RMONCNT OnchipRef8(0xFFFFFECC)
#define RYRCNT OnchipRef8(0xFFFFFECE)
#define RSECAR OnchipRef8(0xFFFFFED0)
#define RMINAR OnchipRef8(0xFFFFFED2)
#define RHRAR OnchipRef8(0xFFFFFED4)
#define RWKAR OnchipRef8(0xFFFFFED6)
#define RDAYAR OnchipRef8(0xFFFFFED8)
#define RMONAR OnchipRef8(0xFFFFFEDA)
#define RCR1 OnchipRef8(0xFFFFFEDC)
#define RCR2 OnchipRef8(0xFFFFFEDE)
#define ICR0 OnchipRef16(0xFFFFFEE0)
#define IPRA OnchipRef16(0xFFFFFEE2)
#define IPRB OnchipRef16(0xFFFFFEE4)
#define TOCR OnchipRef8(0xFFFFFE90)
#define TSTR OnchipRef8(0xFFFFFE92)
#define TCOR_0 OnchipRef32(0xFFFFFE94)
#define TCNT_0 OnchipRef32(0xFFFFFE98)
#define TCR_0 OnchipRef16(0xFFFFFE9C)
#define TCOR_1 OnchipRef32(0xFFFFFEA0)
#define TCNT_1 OnchipRef32(0xFFFFFEA4)
#define TCR_1 OnchipRef16(0xFFFFFEA8)
#define TCOR_2 OnchipRef32(0xFFFFFEAC)
#define TCNT_2 OnchipRef32(0xFFFFFEB0)
#define TCR_2 OnchipRef16(0xFFFFFEB4)
#define TCPR_2 OnchipRef32(0xFFFFFEB8)
#define SCSMR OnchipRef8(0xFFFFFE80)
#define SCBRR OnchipRef8(0xFFFFFE82)
#define SCSCR OnchipRef8(0xFFFFFE84)
#define SCTDR OnchipRef8(0xFFFFFE86)
#define SCSSR OnchipRef8(0xFFFFFE88)
#define SCRDR OnchipRef8(0xFFFFFE8A)
#define SCSCMR OnchipRef8(0xFFFFFE8C)
#define INTEVT2 OnchipRef32(0xA4000000)
#define IRR0 OnchipRef16(0xA4000004)
#define IRR1 OnchipRef16(0xA4000006)
#define IRR2 OnchipRef16(0xA4000008)
#define ICR1 OnchipRef16(0xA4000010)
#define IPRC OnchipRef16(0xA4000016)
#define IPRD OnchipRef16(0xA4000018)
#define IPRE OnchipRef16(0xA400001A)
#define SAR_0 OnchipRef32(0xA4000020)
#define DAR_0 OnchipRef32(0xA4000024)
#define DMATCR_0 OnchipRef32(0xA4000028)
#define CHCR_0 OnchipRef32(0xA400002C)
#define SAR_1 OnchipRef32(0xA4000030)
#define DAR_1 OnchipRef32(0xA4000034)
#define DMATCR_1 OnchipRef32(0xA4000038)
#define CHCR_1 OnchipRef32(0xA400003C)
#define SAR_2 OnchipRef32(0xA4000040)
#define DAR_2 OnchipRef32(0xA4000044)
#define DMATCR_2 OnchipRef32(0xA4000048)
#define CHCR_2 OnchipRef32(0xA400004C)
#define SAR_3 OnchipRef32(0xA4000050)
#define DAR_3 OnchipRef32(0xA4000054)
#define DMATCR_3 OnchipRef32(0xA4000058)
#define CHCR_3 OnchipRef32(0xA400005C)
#define DMAOR OnchipRef16(0xA4000060)
#define CMSTR OnchipRef16(0xA4000070)
#define CMCSR OnchipRef16(0xA4000072)
#define CMCNT OnchipRef16(0xA4000074)
#define CMCOR OnchipRef16(0xA4000076)
#define ADDRAH OnchipRef8(0xA4000080)
#define ADDRAL OnchipRef8(0xA4000082)
#define ADDRBH OnchipRef8(0xA4000084)
#define ADDRBL OnchipRef8(0xA4000086)
#define ADDRCH OnchipRef8(0xA4000088)
#define ADDRCL OnchipRef8(0xA400008A)
#define ADDRDH OnchipRef8(0xA400008C)
#define ADDRDL OnchipRef8(0xA400008E)
#define ADCSR OnchipRef8(0xA4000090)
#define ADCR OnchipRef8(0xA4000092)
#define DADR0 OnchipRef8(0xA40000A0)
#define DADR1 OnchipRef8(0xA40000A2)
#define DACR OnchipRef8(0xA40000A4)
#define PACR OnchipRef16(0xA4000100)
#define PBCR OnchipRef16(0xA4000102)
#define PCCR OnchipRef16(0xA4000104)
#define PDCR OnchipRef16(0xA4000106)
#define PECR OnchipRef16(0xA4000108)
#define PFCR OnchipRef16(0xA400010A)
#define PGCR OnchipRef16(0xA400010C)
#define PHCR OnchipRef16(0xA400010E)
#define PJCR OnchipRef16(0xA4000110)
#define SCPCR OnchipRef16(0xA4000116)
#define PADR OnchipRef8(0xA4000120)
#define PBDR OnchipRef8(0xA4000122)
#define PCDR OnchipRef8(0xA4000124)
#define PDDR OnchipRef8(0xA4000126)
#define PEDR OnchipRef8(0xA4000128)
#define PFDR OnchipRef8(0xA400012A)
#define PGDR OnchipRef8(0xA400012C)
#define PHDR OnchipRef8(0xA400012E)
#define PJDR OnchipRef8(0xA4000130)
#define PKDR OnchipRef8(0xA4000132)
#define PLDR OnchipRef8(0xA4000134)
#define SCPDR OnchipRef8(0xA4000136)
#define SCSMR2 OnchipRef8(0xA4000150)
#define SCBRR2 OnchipRef8(0xA4000152)
#define SCSCR2 OnchipRef8(0xA4000154)
#define SCFTDR2 OnchipRef8(0xA4000156)
#define SCSSR2 OnchipRef16(0xA4000158)
#define SCFRDR2 OnchipRef8(0xA400015A)
#define SCFCR2 OnchipRef8(0xA400015C)
#define SCFDR2 OnchipRef16(0xA400015E)
#define SDIR OnchipRef16(0xA4000200)

enum : int32_t {
  kIrl0 = 0,
  kIrl1 = 1,
  kIrl2 = 2,

  kTmu0Tuni0 = 3,
  kTmu1Tuni1 = 4,
  kTmu2Tuni2 = 5,
  kTmu2Ticpi2 = 6,

  kScifEri = 21,
  kScifRxi = 22,
  kScifBri = 23,
  kScifTxi = 24,
};

enum : uint32_t {
  kPteh = 0xFFFFFFF0,
  kPtel = 0xFFFFFFF4,
  kTtb = 0xFFFFFFF8,
  kTea = 0xFFFFFFFC,
  kMmucr = 0xFFFFFFE0,
  kBasra = 0xFFFFFFE4,
  kBasrb = 0xFFFFFFE8,
  kCcr = 0xFFFFFFEC,
  kCcr2 = 0xA40000B0,
  kTra = 0xFFFFFFD0,
  kExpevt = 0xFFFFFFD4,
  kIntevt = 0xFFFFFFD8,
  kBara = 0xFFFFFFB0,
  kBamra = 0xFFFFFFB4,
  kBbra = 0xFFFFFFB8,
  kBarb = 0xFFFFFFA0,
  kBamrb = 0xFFFFFFA4,
  kBbrb = 0xFFFFFFA8,
  kBdrb = 0xFFFFFF90,
  kBdmrb = 0xFFFFFF94,
  kBrcr = 0xFFFFFF98,
  kBetr = 0xFFFFFF9C,
  kBrsr = 0xFFFFFFAC,
  kBrdr = 0xFFFFFFBC,
  kFrqcr = 0xFFFFFF80,
  kStbcr = 0xFFFFFF82,
  kStbcr2 = 0xFFFFFF88,
  kWtcnt = 0xFFFFFF84,
  kWtcsr = 0xFFFFFF86,
  kBcr1 = 0xFFFFFF60,
  kBcr2 = 0xFFFFFF62,
  kWcr1 = 0xFFFFFF64,
  kWcr2 = 0xFFFFFF66,
  kMcr = 0xFFFFFF68,
  kPcr = 0xFFFFFF6C,
  kRtcsr = 0xFFFFFF6E,
  kRtcnt = 0xFFFFFF70,
  kRtcor = 0xFFFFFF72,
  kRfcr = 0xFFFFFF74,
  kSdmr = 0xFFFFD000,
  kR64Cnt = 0xFFFFFEC0,
  kRseccnt = 0xFFFFFEC2,
  kRmincnt = 0xFFFFFEC4,
  kRhrcnt = 0xFFFFFEC6,
  kRwkcnt = 0xFFFFFEC8,
  kRdaycnt = 0xFFFFFECA,
  kRmoncnt = 0xFFFFFECC,
  kRyrcnt = 0xFFFFFECE,
  kRsecar = 0xFFFFFED0,
  kRminar = 0xFFFFFED2,
  kRhrar = 0xFFFFFED4,
  kRwkar = 0xFFFFFED6,
  kRdayar = 0xFFFFFED8,
  kRmonar = 0xFFFFFEDA,
  kRcr1 = 0xFFFFFEDC,
  kRcr2 = 0xFFFFFEDE,
  kIcr0 = 0xFFFFFEE0,
  kIpra = 0xFFFFFEE2,
  kIprb = 0xFFFFFEE4,
  kTocr = 0xFFFFFE90,
  kTstr = 0xFFFFFE92,
  kTcor_0 = 0xFFFFFE94,
  kTcnt_0 = 0xFFFFFE98,
  kTcr_0 = 0xFFFFFE9C,
  kTcor_1 = 0xFFFFFEA0,
  kTcnt_1 = 0xFFFFFEA4,
  kTcr_1 = 0xFFFFFEA8,
  kTcor_2 = 0xFFFFFEAC,
  kTcnt_2 = 0xFFFFFEB0,
  kTcr_2 = 0xFFFFFEB4,
  kTcpr_2 = 0xFFFFFEB8,
  kScsmr = 0xFFFFFE80,
  kScbrr = 0xFFFFFE82,
  kScscr = 0xFFFFFE84,
  kSctdr = 0xFFFFFE86,
  kScssr = 0xFFFFFE88,
  kScrdr = 0xFFFFFE8A,
  kScscmr = 0xFFFFFE8C,
  kIntevt2 = 0x04000000,
  kIrr0 = 0xA4000004,
  kIrr1 = 0xA4000006,
  kIrr2 = 0xA4000008,
  kIcr1 = 0xA4000010,
  kIprc = 0xA4000016,
  kIprd = 0xA4000018,
  kIpre = 0xA400001A,
  kSar_0 = 0xA4000020,
  kDar_0 = 0xA4000024,
  kDmatcr_0 = 0xA4000028,
  kChcr_0 = 0xA400002C,
  kSar_1 = 0xA4000030,
  kDar_1 = 0xA4000034,
  kDmatcr_1 = 0xA4000038,
  kChcr_1 = 0xA400003C,
  kSar_2 = 0xA4000040,
  kDar_2 = 0xA4000044,
  kDmatcr_2 = 0xA4000048,
  kChcr_2 = 0xA400004C,
  kSar_3 = 0xA4000050,
  kDar_3 = 0xA4000054,
  kDmatcr_3 = 0xA4000058,
  kChcr_3 = 0xA400005C,
  kDmaor = 0xA4000060,
  kCmstr = 0xA4000070,
  kCmcsr = 0xA4000072,
  kCmcnt = 0xA4000074,
  kCmcor = 0xA4000076,
  kAddrah = 0xA4000080,
  kAddral = 0xA4000082,
  kAddrbh = 0xA4000084,
  kAddrbl = 0xA4000086,
  kAddrch = 0xA4000088,
  kAddrcl = 0xA400008A,
  kAddrdh = 0xA400008C,
  kAddrdl = 0xA400008E,
  kAdcsr = 0xA4000090,
  kAdcr = 0xA4000092,
  kDadr0 = 0xA40000A0,
  kDadr1 = 0xA40000A2,
  kDacr = 0xA40000A4,
  kPacr = 0xA4000100,
  kPbcr = 0xA4000102,
  kPccr = 0xA4000104,
  kPdcr = 0xA4000106,
  kPecr = 0xA4000108,
  kPfcr = 0xA400010A,
  kPgcr = 0xA400010C,
  kPhcr = 0xA400010E,
  kPjcr = 0xA4000110,
  kScpcr = 0xA4000116,
  kPadr = 0xA4000120,
  kPbdr = 0xA4000122,
  kPcdr = 0xA4000124,
  kPddr = 0xA4000126,
  kPedr = 0xA4000128,
  kPfdr = 0xA400012A,
  kPgdr = 0xA400012C,
  kPhdr = 0xA400012E,
  kPjdr = 0xA4000130,
  kPkdr = 0xA4000132,
  kPldr = 0xA4000134,
  kScpdr = 0xA4000136,
  kScsmr2 = 0xA4000150,
  kScbrr2 = 0xA4000152,
  kScscr2 = 0xA4000154,
  kScftdr2 = 0xA4000156,
  kScssr2 = 0xA4000158,
  kScfrdr2 = 0xA400015A,
  kScfcr2 = 0xA400015C,
  kScfdr2 = 0xA400015E,
  kSdir = 0xA4000200,
};
}  // namespace sh3