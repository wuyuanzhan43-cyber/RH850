/* pwm9.c - RH850/F1KM-S1 9-channel PWM driver (CS+ / CCRH850)
 *
 * Frequency range: 200 kHz - 400 kHz
 * Duty range:      0 - 1000 permille
 *
 * Channel allocation (all frequencies independent):
 *   P10_2  : TAUD0 CH4 (master) -> CH5 (slave)   TAUD0O5
 *   P10_0  : TAUJ1 CH0 (master) -> CH3 (slave)   TAUJ1O3
 *   P0_2   : TAUJ2 CH0 (master) -> CH3 (slave)   TAUJ2O3
 *   P10_3  : TAUD0 CH6 (master) -> CH7 (slave)   TAUD0O7
 *   P0_6   : GPIO + OSTM0 software PWM (64-pin)
 *   P10_14 : PWGA19                              PWGA19O
 *   P8_5   : TAUJ0 CH0 (master) -> CH3 (slave)   TAUJ0O3
 *   P10_6  : TAUD0 CH12 (master) -> CH13 (slave) TAUD0O13
 *   P10_8  : TAUJ3 CH0 (master) -> CH2 (slave)   TAUJ3O2
 *
 * Comments are ASCII-only to avoid CS+ source encoding issues.
 */
#include "pwm9_64.h"

/* ------------------------------------------------------------------ */
/* Timer base addresses                                               */
/* ------------------------------------------------------------------ */
#define TAUD0_BASE   0xFFE20000UL
#define TAUB0_BASE   0xFFE30000UL
#define TAUJ0_BASE   0xFFE50000UL
#define TAUJ1_BASE   0xFFE51000UL
#define TAUJ2_BASE   0xFFE50100UL
#define TAUJ3_BASE   0xFFE51100UL
#define PWBA0_BASE   0xFFE72800UL
#define PWGA19_BASE  (0xFFE71000UL + 0x40UL * 19UL)
#define SLPW_BASE    0xFFE73000UL
#define PWSA0_BASE   0xFFE70000UL
#define PORT_BASE    0xFFC10000UL

/* ------------------------------------------------------------------ */
/* Port registers (Section 2C.9: offset from <PORTn_base>)             */
/* ------------------------------------------------------------------ */
#define PORT_PM(n)     (*(volatile uint16_t *)(PORT_BASE + 0x0300UL + (n) * 4UL))
#define PORT_PMC(n)    (*(volatile uint16_t *)(PORT_BASE + 0x0400UL + (n) * 4UL))
#define PORT_PFC(n)    (*(volatile uint16_t *)(PORT_BASE + 0x0500UL + (n) * 4UL))
#define PORT_PFCE(n)   (*(volatile uint16_t *)(PORT_BASE + 0x0600UL + (n) * 4UL))
#define PORT_PFCAE(n)  (*(volatile uint16_t *)(PORT_BASE + 0x0A00UL + (n) * 4UL))
#define PORT_PIBC(n)   (*(volatile uint16_t *)(PORT_BASE + 0x4000UL + (n) * 4UL))
#define PORT_PBDC(n)   (*(volatile uint16_t *)(PORT_BASE + 0x4100UL + (n) * 4UL))
#define PORT_PIPC(n)   (*(volatile uint16_t *)(PORT_BASE + 0x4200UL + (n) * 4UL))
#define P0             (*(volatile uint16_t *)(PORT_BASE + 0x0000UL))

/* ------------------------------------------------------------------ */
/* TAUD0 registers (Section 32.3, offsets from <TAUD0_base>)           */
/* ------------------------------------------------------------------ */
#define TAUD_CDR(m)    (*(volatile uint16_t *)(TAUD0_BASE + (m) * 4UL))
#define TAUD_CMOR(m)   (*(volatile uint16_t *)(TAUD0_BASE + 0x200UL + (m) * 4UL))
#define TAUD_CMUR(m)   (*(volatile uint16_t *)(TAUD0_BASE + 0x0C0UL + (m) * 4UL))
#define TAUD_TOE       (*(volatile uint16_t *)(TAUD0_BASE + 0x05CUL))
#define TAUD_TOM       (*(volatile uint16_t *)(TAUD0_BASE + 0x248UL))
#define TAUD_TOC       (*(volatile uint16_t *)(TAUD0_BASE + 0x24CUL))
#define TAUD_TOL       (*(volatile uint16_t *)(TAUD0_BASE + 0x040UL))
#define TAUD_TDE       (*(volatile uint16_t *)(TAUD0_BASE + 0x250UL))
#define TAUD_TDM       (*(volatile uint16_t *)(TAUD0_BASE + 0x254UL))
#define TAUD_TDL       (*(volatile uint16_t *)(TAUD0_BASE + 0x054UL))
#define TAUD_TRO       (*(volatile uint16_t *)(TAUD0_BASE + 0x04CUL))
#define TAUD_TRE       (*(volatile uint16_t *)(TAUD0_BASE + 0x258UL))
#define TAUD_TRC       (*(volatile uint16_t *)(TAUD0_BASE + 0x25CUL))
#define TAUD_TME       (*(volatile uint16_t *)(TAUD0_BASE + 0x050UL))
#define TAUD_RDE       (*(volatile uint16_t *)(TAUD0_BASE + 0x260UL))
#define TAUD_RDM       (*(volatile uint16_t *)(TAUD0_BASE + 0x264UL))
#define TAUD_RDS       (*(volatile uint16_t *)(TAUD0_BASE + 0x268UL))
#define TAUD_RDC       (*(volatile uint16_t *)(TAUD0_BASE + 0x26CUL))
#define TAUD_RDT       (*(volatile uint16_t *)(TAUD0_BASE + 0x044UL))
#define TAUD_RSF       (*(volatile uint16_t *)(TAUD0_BASE + 0x048UL))
#define TAUD_TS        (*(volatile uint16_t *)(TAUD0_BASE + 0x1C4UL))
#define TAUD_TE        (*(volatile uint16_t *)(TAUD0_BASE + 0x1C0UL))
#define TAUD_TT        (*(volatile uint16_t *)(TAUD0_BASE + 0x1C8UL))
#define TAUD_TPS       (*(volatile uint16_t *)(TAUD0_BASE + 0x240UL))

/* TAUB0 is not available on the 64-pin package. */
#if 0
/* ------------------------------------------------------------------ */
/* TAUB0 registers (Section 31.3, offsets from <TAUB0_base>)           */
/* ------------------------------------------------------------------ */
#define TAUB_CDR(m)    (*(volatile uint16_t *)(TAUB0_BASE + (m) * 4UL))
#define TAUB_CMOR(m)   (*(volatile uint16_t *)(TAUB0_BASE + 0x200UL + (m) * 4UL))
#define TAUB_CMUR(m)   (*(volatile uint16_t *)(TAUB0_BASE + 0x0C0UL + (m) * 4UL))
#define TAUB_TOE       (*(volatile uint16_t *)(TAUB0_BASE + 0x05CUL))
#define TAUB_TOM       (*(volatile uint16_t *)(TAUB0_BASE + 0x248UL))
#define TAUB_TOC       (*(volatile uint16_t *)(TAUB0_BASE + 0x24CUL))
#define TAUB_TOL       (*(volatile uint16_t *)(TAUB0_BASE + 0x040UL))
#define TAUB_TDE       (*(volatile uint16_t *)(TAUB0_BASE + 0x250UL))
#define TAUB_TDL       (*(volatile uint16_t *)(TAUB0_BASE + 0x054UL))
#define TAUB_RDE       (*(volatile uint16_t *)(TAUB0_BASE + 0x260UL))
#define TAUB_RDM       (*(volatile uint16_t *)(TAUB0_BASE + 0x264UL))
#define TAUB_RDS       (*(volatile uint16_t *)(TAUB0_BASE + 0x268UL))
#define TAUB_RDC       (*(volatile uint16_t *)(TAUB0_BASE + 0x26CUL))
#define TAUB_RDT       (*(volatile uint16_t *)(TAUB0_BASE + 0x044UL))
#define TAUB_RSF       (*(volatile uint16_t *)(TAUB0_BASE + 0x048UL))
#define TAUB_TS        (*(volatile uint16_t *)(TAUB0_BASE + 0x1C4UL))
#define TAUB_TE        (*(volatile uint16_t *)(TAUB0_BASE + 0x1C0UL))
#define TAUB_TT        (*(volatile uint16_t *)(TAUB0_BASE + 0x1C8UL))
#define TAUB_TPS       (*(volatile uint16_t *)(TAUB0_BASE + 0x240UL))

#endif /* TAUB0 not available on 64-pin */

/* ------------------------------------------------------------------ */
/* TAUJ registers (Section 33.3, offsets from <TAUJn_base>)            */
/* ------------------------------------------------------------------ */
#define TAUJ_CDR(b, m)    (*(volatile uint32_t *)((b) + (m) * 4UL))
#define TAUJ_CNT(b, m)    (*(volatile uint32_t *)((b) + 0x10UL + (m) * 4UL))
#define TAUJ_CMOR(b, m)   (*(volatile uint16_t *)((b) + 0x80UL + (m) * 4UL))
#define TAUJ_CMUR(b, m)   (*(volatile uint16_t *)((b) + 0x20UL + (m) * 4UL))
#define TAUJ_TOE(b)       (*(volatile uint16_t *)((b) + 0x60UL))
#define TAUJ_TOM(b)       (*(volatile uint16_t *)((b) + 0x98UL))
#define TAUJ_TOC(b)       (*(volatile uint16_t *)((b) + 0x9CUL))
#define TAUJ_TOL(b)       (*(volatile uint16_t *)((b) + 0x64UL))
#define TAUJ_RDE(b)       (*(volatile uint16_t *)((b) + 0xA0UL))
#define TAUJ_RDM(b)       (*(volatile uint16_t *)((b) + 0xA4UL))
#define TAUJ_RDT(b)       (*(volatile uint16_t *)((b) + 0x68UL))
#define TAUJ_RSF(b)       (*(volatile uint16_t *)((b) + 0x6CUL))
#define TAUJ_TS(b)        (*(volatile uint16_t *)((b) + 0x54UL))
#define TAUJ_TE(b)        (*(volatile uint16_t *)((b) + 0x50UL))
#define TAUJ_TT(b)        (*(volatile uint16_t *)((b) + 0x58UL))
#define TAUJ_TPS(b)       (*(volatile uint16_t *)((b) + 0x90UL))

/* ------------------------------------------------------------------ */
/* PWGA19 / PWBA0 / PWSA0 registers (Section 37.2)                     */
/* ------------------------------------------------------------------ */
#define PWGA19_CSDR    (*(volatile uint16_t *)(PWGA19_BASE + 0x00UL))
#define PWGA19_CRDR    (*(volatile uint16_t *)(PWGA19_BASE + 0x04UL))
#define PWGA19_RDT     (*(volatile uint8_t  *)(PWGA19_BASE + 0x0CUL))
#define PWGA19_RSF     (*(volatile uint8_t  *)(PWGA19_BASE + 0x10UL))
#define PWGA19_TCR     (*(volatile uint8_t  *)(PWGA19_BASE + 0x18UL))
#define PWGA19_CTL     (*(volatile uint8_t  *)(PWGA19_BASE + 0x20UL))

#define PWBA0_BRS0     (*(volatile uint16_t *)(PWBA0_BASE + 0x00UL))
#define PWBA0_TE       (*(volatile uint16_t *)(PWBA0_BASE + 0x10UL))
#define PWBA0_TS       (*(volatile uint16_t *)(PWBA0_BASE + 0x14UL))
#define PWBA0_TT       (*(volatile uint16_t *)(PWBA0_BASE + 0x18UL))

#define PWSA0_CTL      (*(volatile uint8_t  *)(PWSA0_BASE + 0x00UL))
#define PWGAPRD        (*(volatile uint16_t *)(SLPW_BASE + 0x0CUL))
#define PWGAPRDSL0     (*(volatile uint32_t *)(SLPW_BASE + 0x10UL))
#define SLPWGA0        (*(volatile uint32_t *)(SLPW_BASE + 0x00UL))

/* OSTM0 for software PWM on P0_6 (Section 30) */
#define OSTM0_BASE   0xFFD70000UL
#define OSTM0_CMP    (*(volatile uint32_t *)(OSTM0_BASE + 0x00UL))
#define OSTM0_CNT    (*(volatile uint32_t *)(OSTM0_BASE + 0x04UL))
#define OSTM0_TE     (*(volatile uint8_t  *)(OSTM0_BASE + 0x10UL))
#define OSTM0_TS     (*(volatile uint8_t  *)(OSTM0_BASE + 0x14UL))
#define OSTM0_TT     (*(volatile uint8_t  *)(OSTM0_BASE + 0x18UL))
#define OSTM0_CTL    (*(volatile uint8_t  *)(OSTM0_BASE + 0x20UL))

/* ------------------------------------------------------------------ */
/* Channel hardware table                                             */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  type;      /* 0 = TAUD0, 2 = TAUJ, 3 = PWGA19, 4 = SW PWM */
    uint32_t base;      /* TAUJ base address (0 for others)            */
    uint8_t  master;    /* master channel                              */
    uint8_t  slave;     /* slave/output channel                        */
    uint32_t pclk;      /* timer input clock in Hz                     */
} pwm_hw_t;

static const pwm_hw_t g_hw[PWM_CH_COUNT] = {
    { 0U, TAUD0_BASE,  4U,  5U, PWM_PCLK_IPERI1_HZ },  /* P10_2  */
    { 2U, TAUJ1_BASE,  0U,  3U, PWM_PCLK_IPERI1_HZ },  /* P10_0  */
    { 2U, TAUJ2_BASE,  0U,  3U, PWM_PCLK_ATAUJ_HZ  },  /* P0_2   */
    { 0U, TAUD0_BASE,  6U,  7U, PWM_PCLK_IPERI1_HZ },  /* P10_3  */
#if PWM9_USE_P10_4_FOR_P0_6
    { 0U, TAUD0_BASE,  8U,  9U, PWM_PCLK_IPERI1_HZ },  /* P0_6 net -> P10_4 */
#else
    { 4U, 0UL,         0U,  0U, 0UL },                  /* P0_6: software PWM (OSTM0) */
#endif
    { 3U, 0UL,         0U,  0U, PWM_PCLK_IPERI2_HZ },  /* P10_14: PWGA19 */
    { 2U, TAUJ0_BASE,  0U,  3U, PWM_PCLK_ATAUJ_HZ  },  /* P8_5   */
    { 0U, TAUD0_BASE, 12U, 13U, PWM_PCLK_IPERI1_HZ },  /* P10_6  */
    { 2U, TAUJ3_BASE,  0U,  2U, PWM_PCLK_IPERI1_HZ }   /* P10_8  */
};

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */
static uint16_t duty_counts16(uint16_t period, uint16_t duty_permille)
{
    uint32_t high;

    if (duty_permille >= 1000U) {
        if (period == 0xFFFFU) {
            return 0xFFFFU;
        }
        return (uint16_t)(period + 1U);
    }
    high = ((uint32_t)period + 1U) * (uint32_t)duty_permille;
    high = (high + 500U) / 1000U;
    if (high > (uint32_t)period + 1U) {
        high = (uint32_t)period + 1U;
    }
    return (uint16_t)high;
}

static uint32_t duty_counts32(uint32_t period, uint16_t duty_permille)
{
    uint64_t high;

    if (duty_permille >= 1000U) {
        return period + 1U;
    }
    high = ((uint64_t)period + 1ULL) * (uint64_t)duty_permille;
    high = (high + 500ULL) / 1000ULL;
    if (high > (uint64_t)period + 1ULL) {
        high = (uint64_t)period + 1ULL;
    }
    return (uint32_t)high;
}

static uint8_t choose_prs16(uint32_t pclk, uint32_t freq, uint16_t *period)
{
    uint8_t  prs;
    uint32_t clk;
    uint32_t n;

    for (prs = 0U; prs < 16U; prs++) {
        clk = pclk >> prs;
        n = (clk + (freq / 2U)) / freq;
        if ((n >= 2U) && (n <= 65536U)) {
            *period = (uint16_t)(n - 1U);
            return prs;
        }
    }
    return 0xFFU;
}

static uint8_t choose_prs32(uint32_t pclk, uint32_t freq, uint32_t *period)
{
    uint8_t  prs;
    uint32_t clk;
    uint32_t n;

    for (prs = 0U; prs < 16U; prs++) {
        clk = pclk >> prs;
        n = (clk + (freq / 2U)) / freq;
        if (n >= 2U) {
            *period = (n - 1U);
            return prs;
        }
    }
    return 0xFFU;
}

/* alt: 0 = 1st, 1 = 2nd, 2 = 3rd, 3 = 4th,
 *      4 = 5th, 5 = 6th, 6 = 7th alternative function.
 * Note: PFC = bit0, PFCE = bit1, PFCAE = bit2.
 */
static void port_alt(uint8_t n, uint8_t m, uint8_t alt)
{
    uint16_t bit = (uint16_t)(1U << m);

    PORT_PIBC(n)  &= (uint16_t)~bit;
    PORT_PBDC(n)  &= (uint16_t)~bit;
    PORT_PM(n)    |= bit;
    PORT_PMC(n)   &= (uint16_t)~bit;

    if ((n == 0U) || (n == 10U)) {
        PORT_PIPC(n) &= (uint16_t)~bit;
    }

    if ((alt & 1U) != 0U) { PORT_PFC(n)   |= bit; } else { PORT_PFC(n)   &= (uint16_t)~bit; }
    if ((alt & 2U) != 0U) { PORT_PFCE(n)  |= bit; } else { PORT_PFCE(n)  &= (uint16_t)~bit; }
    if ((alt & 4U) != 0U) { PORT_PFCAE(n) |= bit; } else { PORT_PFCAE(n) &= (uint16_t)~bit; }

    PORT_PM(n)  &= (uint16_t)~bit;
    PORT_PMC(n) |= bit;
}

/* ------------------------------------------------------------------ */
/* TAUD0 PWM: master = interval timer, slave = one-count              */
/* ------------------------------------------------------------------ */
static int set_taud(uint8_t master, uint8_t slave, uint32_t pclk,
                    uint32_t freq, uint16_t duty)
{
    uint16_t period;
    uint8_t  prs;
    uint16_t mbit;
    uint16_t sbit;
    uint16_t both;

    prs = choose_prs16(pclk, freq, &period);
    if (prs == 0xFFU) {
        return -1;
    }

    mbit = (uint16_t)(1U << master);
    sbit = (uint16_t)(1U << slave);
    both = (uint16_t)(mbit | sbit);

    TAUD_TT = both;
    while ((TAUD_TE & both) != 0U) {
        /* wait until the channels are stopped */
    }

    /* CK0 = PCLK / 2^prs. All TAUD0 channels share PRS0. */
    TAUD_TPS = (uint16_t)((TAUD_TPS & (uint16_t)~0x000FU) | (uint16_t)prs);

    /* Master channel: interval timer mode, no output. */
    TAUD_CMOR(master) = (uint16_t)((1U << 11) | 0x0001U);
    TAUD_CMUR(master) = 0x0000U;
    TAUD_CDR(master)  = period;
    TAUD_TOE &= (uint16_t)~mbit;
    TAUD_TOM &= (uint16_t)~mbit;
    TAUD_TOC &= (uint16_t)~mbit;
    TAUD_TOL &= (uint16_t)~mbit;
    TAUD_TDE &= (uint16_t)~mbit;
    TAUD_TDM &= (uint16_t)~mbit;
    TAUD_TDL &= (uint16_t)~mbit;
    TAUD_TRE &= (uint16_t)~mbit;
    TAUD_TRO &= (uint16_t)~mbit;
    TAUD_TRC &= (uint16_t)~mbit;
    TAUD_TME &= (uint16_t)~mbit;

    /* Slave channel: one-count mode, start trigger from master. */
    TAUD_CMOR(slave) = (uint16_t)((4U << 8) | (4U << 1) | 0x0001U);
    TAUD_CMUR(slave) = 0x0000U;
    TAUD_CDR(slave)  = duty_counts16(period, duty);

    /* Simultaneous rewrite: master and slave must use the same settings. */
    TAUD_RDE |= both;
    TAUD_RDS &= (uint16_t)~both;
    TAUD_RDM &= (uint16_t)~both;
    TAUD_RDC &= (uint16_t)~both;

    /* Slave output: synchronous channel output mode 1, positive logic. */
    TAUD_TOE |= sbit;
    TAUD_TOM |= sbit;
    TAUD_TOC &= (uint16_t)~sbit;
    TAUD_TOL &= (uint16_t)~sbit;
    TAUD_TDE &= (uint16_t)~sbit;
    TAUD_TDM &= (uint16_t)~sbit;
    TAUD_TDL &= (uint16_t)~sbit;
    TAUD_TRE &= (uint16_t)~sbit;
    TAUD_TRO &= (uint16_t)~sbit;
    TAUD_TRC &= (uint16_t)~sbit;
    TAUD_TME &= (uint16_t)~sbit;

    TAUD_TS = both;
    return 0;
}

#if 0 /* TAUB0 is not available on the 64-pin package */
/* ------------------------------------------------------------------ */
/* TAUB0 PWM: master = interval timer, slave = one-count              */
/* ------------------------------------------------------------------ */
static int set_taub(uint8_t master, uint8_t slave, uint32_t pclk,
                    uint32_t freq, uint16_t duty)
{
    uint16_t period;
    uint8_t  prs;
    uint16_t mbit;
    uint16_t sbit;
    uint16_t both;

    prs = choose_prs16(pclk, freq, &period);
    if (prs == 0xFFU) {
        return -1;
    }

    mbit = (uint16_t)(1U << master);
    sbit = (uint16_t)(1U << slave);
    both = (uint16_t)(mbit | sbit);

    TAUB_TT = both;
    while ((TAUB_TE & both) != 0U) {
        /* wait until the channels are stopped */
    }

    TAUB_TPS = (uint16_t)((TAUB_TPS & (uint16_t)~0x000FU) | (uint16_t)prs);

    TAUB_CMOR(master) = (uint16_t)((1U << 11) | 0x0001U);
    TAUB_CMUR(master) = 0x0000U;
    TAUB_CDR(master)  = period;
    TAUB_TOE &= (uint16_t)~mbit;
    TAUB_TOM &= (uint16_t)~mbit;
    TAUB_TOC &= (uint16_t)~mbit;
    TAUB_TOL &= (uint16_t)~mbit;
    TAUB_TDE &= (uint16_t)~mbit;
    TAUB_TDL &= (uint16_t)~mbit;

    TAUB_CMOR(slave) = (uint16_t)((4U << 8) | (4U << 1) | 0x0001U);
    TAUB_CMUR(slave) = 0x0000U;
    TAUB_CDR(slave)  = duty_counts16(period, duty);

    TAUB_RDE |= both;
    TAUB_RDS &= (uint16_t)~both;
    TAUB_RDM &= (uint16_t)~both;
    TAUB_RDC &= (uint16_t)~both;

    TAUB_TOE |= sbit;
    TAUB_TOM |= sbit;
    TAUB_TOC &= (uint16_t)~sbit;
    TAUB_TOL &= (uint16_t)~sbit;
    TAUB_TDE &= (uint16_t)~sbit;
    TAUB_TDL &= (uint16_t)~sbit;

    TAUB_TS = both;
    return 0;
}

#endif /* TAUB0 not available on 64-pin */

/* ------------------------------------------------------------------ */
/* TAUJ PWM: 32-bit master/slave                                      */
/* ------------------------------------------------------------------ */
static int set_tauj(uint32_t base, uint8_t master, uint8_t slave,
                    uint32_t pclk, uint32_t freq, uint16_t duty)
{
    uint32_t period;
    uint8_t  prs;
    uint16_t mbit;
    uint16_t sbit;
    uint16_t both;

    prs = choose_prs32(pclk, freq, &period);
    if (prs == 0xFFU) {
        return -1;
    }

    mbit = (uint16_t)(1U << master);
    sbit = (uint16_t)(1U << slave);
    both = (uint16_t)(mbit | sbit);

    TAUJ_TT(base) = both;
    while ((TAUJ_TE(base) & both) != 0U) {
        /* wait until the channels are stopped */
    }

    TAUJ_TPS(base) = (uint16_t)((TAUJ_TPS(base) & (uint16_t)~0x000FU) | (uint16_t)prs);

    TAUJ_CMOR(base, master) = (uint16_t)((1U << 11) | 0x0001U);
    TAUJ_CMUR(base, master) = 0x0000U;
    TAUJ_CDR(base, master)  = period;
    TAUJ_TOE(base) &= (uint16_t)~mbit;
    TAUJ_TOM(base) &= (uint16_t)~mbit;
    TAUJ_TOC(base) &= (uint16_t)~mbit;
    TAUJ_TOL(base) &= (uint16_t)~mbit;

    TAUJ_CMOR(base, slave) = (uint16_t)((4U << 8) | (4U << 1) | 0x0001U);
    TAUJ_CMUR(base, slave) = 0x0000U;
    TAUJ_CDR(base, slave)  = duty_counts32(period, duty);

    TAUJ_RDE(base) |= both;
    TAUJ_RDM(base) &= (uint16_t)~both;

    TAUJ_TOE(base) |= sbit;
    TAUJ_TOM(base) |= sbit;
    TAUJ_TOC(base) &= (uint16_t)~sbit;
    TAUJ_TOL(base) &= (uint16_t)~sbit;

    TAUJ_TS(base) = both;
    return 0;
}

/* ------------------------------------------------------------------ */
/* PWGA19 PWM                                                         */
/* ------------------------------------------------------------------ */
static uint16_t pwga_duty_counts(uint16_t period, uint16_t duty_permille)
{
    uint32_t high;

    if (duty_permille == 0U) {
        return 0U;
    }
    if (duty_permille >= 1000U) {
        return 0x1000U;   /* PWGA19_CRDR bit12 = 1 -> fixed high */
    }
    high = ((uint32_t)period + 1U) * (uint32_t)duty_permille;
    high = (high + 500U) / 1000U;
    if (high > (uint32_t)period) {
        high = (uint32_t)period;
    }
    return (uint16_t)high;
}

static int set_pwga(uint32_t pclk, uint32_t freq, uint16_t duty)
{
    uint32_t brs;
    uint32_t n;
    uint32_t pwmclk;
    uint16_t period;
    uint16_t crdr;

    brs = 0U;
    n = 0U;
    for (brs = 0U; brs < 2048U; brs++) {
        pwmclk = (brs == 0U) ? pclk : (pclk / (2U * brs));
        n = (pwmclk + (freq / 2U)) / freq;
        if ((n >= 1U) && (n <= 4096U)) {
            break;
        }
    }
    if (brs >= 2048U) {
        return -1;
    }

    period = (uint16_t)(n - 1U);
    crdr = pwga_duty_counts(period, duty);

    /* Stop PWGA19 and PWMCLK0 before changing the period. */
    SLPWGA0 &= ~(1UL << 19);
    PWBA0_TT = 0x0001U;
    while ((PWBA0_TE & 0x0001U) != 0U) {
        /* wait until PWMCLK0 is stopped */
    }
    PWSA0_CTL = 0x00U;

    /* Start PWMCLK0. BRS0 = 0 -> PWMCLK0 = PCLK. */
    PWBA0_BRS0 = (uint16_t)brs;
    PWSA0_CTL = 0x01U;
    PWBA0_TS = 0x0001U;

    PWGA19_CTL = 0x00U;                  /* CKS=00: PWMCLK0 */
    PWGAPRDSL0 |= (1UL << 19);            /* PWGA19 uses PWGAPRD */
    PWGAPRD = period;
    PWGA19_CSDR = 0x0000U;
    PWGA19_CRDR = crdr;
    PWGA19_TCR = 0x01U;                  /* TOE = 1 */
    SLPWGA0 |= (1UL << 19);               /* start PWGA19 */
    return 0;
}

static void pwga_set_duty(uint16_t duty_permille)
{
    uint16_t period = PWGAPRD;
    uint16_t crdr = pwga_duty_counts(period, duty_permille);

    PWGA19_CRDR = crdr;
    PWGA19_RDT = 0x01U;                  /* request simultaneous rewrite */
    while ((PWGA19_RSF & 0x01U) != 0U) {
        /* wait until the compare buffer is updated */
    }
}

/* ------------------------------------------------------------------ */
/* Software PWM on P0_6 (OSTM0 free-run compare)                       */
/* ------------------------------------------------------------------ */
static volatile uint32_t sw_period_counts = 0UL;
static volatile uint32_t sw_high_counts = 0UL;
static volatile uint32_t sw_low_counts = 0UL;
static volatile uint32_t sw_freq_hz = 0UL;
static volatile uint16_t sw_duty_permille = 0U;
static volatile uint8_t  sw_level = 0U;

static void p0_6_gpio_init(void)
{
    uint16_t bit = (uint16_t)(1U << 6);

    PORT_PIBC(0)  &= (uint16_t)~bit;
    PORT_PBDC(0)  &= (uint16_t)~bit;
    PORT_PM(0)    &= (uint16_t)~bit;   /* output */
    PORT_PMC(0)   &= (uint16_t)~bit;   /* GPIO */
    PORT_PIPC(0)  &= (uint16_t)~bit;   /* software I/O */
    PORT_PFC(0)   &= (uint16_t)~bit;
    PORT_PFCE(0)  &= (uint16_t)~bit;
    PORT_PFCAE(0) &= (uint16_t)~bit;
    P0 &= (uint16_t)~bit;              /* start low */
}

static void sw_pwm_stop(void)
{
    OSTM0_TT = 0x01U;
    while ((OSTM0_TE & 0x01U) != 0U) {
        /* wait until OSTM0 is stopped */
    }
}

static int sw_pwm_apply(uint32_t freq, uint16_t duty)
{
    uint32_t n;
    uint32_t high;
    uint32_t low;

    if ((freq < 200000UL) || (freq > 400000UL)) {
        return -1;
    }
    if (duty > 1000U) {
        duty = 1000U;
    }

    n = (PWM_SW_PCLK_HZ + (freq / 2U)) / freq;
    if (n < 4U) {
        return -1;
    }

    sw_freq_hz = freq;
    sw_period_counts = n;
    sw_duty_permille = duty;

    if (duty == 0U) {
        sw_pwm_stop();
        sw_level = 0U;
        P0 &= (uint16_t)~(1U << 6);
        sw_high_counts = 0U;
        sw_low_counts = n;
        return 0;
    }
    if (duty >= 1000U) {
        sw_pwm_stop();
        sw_level = 1U;
        P0 |= (uint16_t)(1U << 6);
        sw_high_counts = n;
        sw_low_counts = 0U;
        return 0;
    }

    high = ((uint32_t)n * (uint32_t)duty + 500U) / 1000U;
    if (high < 1U) {
        high = 1U;
    }
    if (high >= n) {
        high = n - 1U;
    }
    low = n - high;
    sw_high_counts = high;
    sw_low_counts = low;

    sw_pwm_stop();
    P0 |= (uint16_t)(1U << 6);   /* start high */
    sw_level = 1U;
    OSTM0_CTL = 0x02U;           /* free-run compare, no start interrupt */
    OSTM0_CMP = high;            /* first edge after high counts */
    OSTM0_TS = 0x01U;
    return 0;
}

static int sw_pwm_set_duty(uint16_t duty)
{
    if (sw_freq_hz == 0UL) {
        return -1;
    }
    return sw_pwm_apply(sw_freq_hz, duty);
}

void pwm_sw_isr(void)
{
    uint32_t interval;
    uint32_t next;

    if (sw_level != 0U) {
        sw_level = 0U;
        P0 &= (uint16_t)~(1U << 6);
        interval = sw_low_counts;
    } else {
        sw_level = 1U;
        P0 |= (uint16_t)(1U << 6);
        interval = sw_high_counts;
    }

    if (interval == 0UL) {
        sw_pwm_stop();
        return;
    }

    next = OSTM0_CMP + interval;
    if ((int32_t)(next - OSTM0_CNT) <= 0) {
        next = OSTM0_CNT + interval;   /* resync after a long ISR delay */
    }
    OSTM0_CMP = next;
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */
void pwm9_init(void)
{
    uint8_t i;

    /* If the Port Protection Cluster is enabled, unlock PPCMD here. */
    /* P10_2  : TAUD0O5,   1st alternative */
    port_alt(10U, 2U, 0U);
    /* P10_0  : TAUJ1O3,   6th alternative */
    port_alt(10U, 0U, 5U);
    /* P0_2   : TAUJ2O3,   6th alternative */
    port_alt(0U, 2U, 5U);
    /* P10_3  : TAUD0O7,   1st alternative */
    port_alt(10U, 3U, 0U);
#if PWM9_USE_P10_4_FOR_P0_6
    /* P0_6 net reworked to P10_4: TAUD0O9, 1st alternative */
    port_alt(10U, 4U, 0U);
#else
    /* P0_6: GPIO + OSTM0 software PWM */
    p0_6_gpio_init();
#endif
    /* P10_14 : PWGA19O,   1st alternative */
    port_alt(10U, 14U, 0U);
    /* P8_5   : TAUJ0O3,   1st alternative */
    port_alt(8U, 5U, 0U);
    /* P10_6  : TAUD0O13,  1st alternative */
    port_alt(10U, 6U, 0U);
    /* P10_8  : TAUJ3O2,   7th alternative */
    port_alt(10U, 8U, 6U);

    /* Default: 250 kHz, 50% duty. */
    for (i = 0U; i < (uint8_t)PWM_CH_COUNT; i++) {
        (void)pwm9_set_freq_duty((pwm_ch_t)i, 250000UL, 500U);
    }
}

int pwm9_set_freq_duty(pwm_ch_t ch, uint32_t freq_hz, uint16_t duty_permille)
{
    const pwm_hw_t *h;

    if (ch >= PWM_CH_COUNT) {
        return -1;
    }
    if ((freq_hz < 200000UL) || (freq_hz > 400000UL)) {
        return -1;
    }
    if (duty_permille > 1000U) {
        duty_permille = 1000U;
    }

    h = &g_hw[ch];
    switch (h->type) {
    case 0U:
        return set_taud(h->master, h->slave, h->pclk, freq_hz, duty_permille);
    case 1U:
        return -1;   /* TAUB0 is not available on the 64-pin package */
    case 2U:
        return set_tauj(h->base, h->master, h->slave, h->pclk, freq_hz, duty_permille);
    case 3U:
        return set_pwga(h->pclk, freq_hz, duty_permille);
    case 4U:
        return sw_pwm_apply(freq_hz, duty_permille);
    default:
        return -1;
    }
}

int pwm9_set_duty(pwm_ch_t ch, uint16_t duty_permille)
{
    const pwm_hw_t *h;

    if (ch >= PWM_CH_COUNT) {
        return -1;
    }
    if (duty_permille > 1000U) {
        duty_permille = 1000U;
    }

    h = &g_hw[ch];
    switch (h->type) {
    case 0U:
        TAUD_CDR(h->slave) = duty_counts16(TAUD_CDR(h->master), duty_permille);
        return 0;
    case 1U:
        return -1;   /* TAUB0 is not available on the 64-pin package */
    case 2U:
        TAUJ_CDR(h->base, h->slave) =
            duty_counts32(TAUJ_CDR(h->base, h->master), duty_permille);
        return 0;
    case 3U:
        pwga_set_duty(duty_permille);
        return 0;
    case 4U:
        return sw_pwm_set_duty(duty_permille);
    default:
        return -1;
    }
}

uint32_t pwm9_get_actual_freq(pwm_ch_t ch)
{
    const pwm_hw_t *h;
    uint32_t clk;
    uint32_t period;
    uint32_t brs;

    if (ch >= PWM_CH_COUNT) {
        return 0UL;
    }

    h = &g_hw[ch];
    switch (h->type) {
    case 0U:
        clk = h->pclk >> (TAUD_TPS & 0x000FU);
        period = (uint32_t)TAUD_CDR(h->master) + 1UL;
        break;
    case 1U:
        return 0UL;  /* TAUB0 is not available on the 64-pin package */
    case 2U:
        clk = h->pclk >> (TAUJ_TPS(h->base) & 0x000FU);
        period = TAUJ_CDR(h->base, h->master) + 1UL;
        break;
    case 3U:
        brs = (uint32_t)PWBA0_BRS0;
        clk = (brs == 0UL) ? h->pclk : (h->pclk / (2UL * brs));
        period = (uint32_t)PWGAPRD + 1UL;
        break;
    case 4U:
        return (sw_period_counts == 0UL) ? 0UL : (PWM_SW_PCLK_HZ / sw_period_counts);
    default:
        return 0UL;
    }

    if (period == 0UL) {
        return 0UL;
    }
    return clk / period;
}

uint16_t pwm9_get_actual_duty(pwm_ch_t ch)
{
    const pwm_hw_t *h;
    uint32_t period;
    uint32_t high;

    if (ch >= PWM_CH_COUNT) {
        return 0U;
    }

    h = &g_hw[ch];
    switch (h->type) {
    case 0U:
        period = (uint32_t)TAUD_CDR(h->master) + 1UL;
        high = (uint32_t)TAUD_CDR(h->slave);
        break;
    case 1U:
        return 0U;   /* TAUB0 is not available on the 64-pin package */
    case 2U:
        period = TAUJ_CDR(h->base, h->master) + 1UL;
        high = TAUJ_CDR(h->base, h->slave);
        break;
    case 3U:
        if ((PWGA19_CRDR & 0x1000U) != 0U) {
            return 1000U;
        }
        period = (uint32_t)PWGAPRD + 1UL;
        high = (uint32_t)PWGA19_CRDR - (uint32_t)PWGA19_CSDR;
        break;
    case 4U:
        return sw_duty_permille;
    default:
        return 0U;
    }

    if (period == 0UL) {
        return 0U;
    }
    if (high > period) {
        high = period;
    }
    return (uint16_t)((high * 1000UL + (period / 2UL)) / period);
}
