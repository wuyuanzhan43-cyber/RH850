# RH850/F1KM-S1 九路 PWM 配置指南

> **200 kHz – 400 kHz · 独立频率 / 占空比 · CS+ / CCRH850**  
> 按 100-pin LQFP 封装整理，重点核对 P0_6、P10_14、P10_6。

---

## 0. TL;DR

| 项目 | 结论 |
|---|---|
| P10_6 | **TAUD0O13**，第 1 复用；TAUD0 CH12 主 / CH13 从；80 MHz 时钟域 |
| P10_14 | **TAUB0O7**，第 4 复用；TAUB0 CH4 主 / CH7 从；40 MHz 时钟域 |
| P0_6 | **没有 TAU/TAUB/TAUD 输出**，只有 **PWGA35O**（第 5 复用）；40 MHz 时钟域 |
| 九路方案 | 8 路定时器 PWM + 1 路 PWGA35，全部独立频率/占空比 |
| 分辨率 | 80 MHz 通道 @400 kHz：占空比步进 0.5%；40 MHz 通道：1% |
| 代码 | `pwm9.c` / `pwm9.h`，CS+ CCRH850，默认 250 kHz / 50% |

> ⚠️ **最大风险**：P0_6 只能走 PWGA35（PWM-Diag），不能按 TAUD/TAUB/TAUJ 配置。  
> ⚠️ **分辨率先天限制**：P0_6、P10_14、P8_5、P0_2（TAUJ2）最大 40 MHz，400 kHz 时只有 100 个计数周期。

---

## 1. 引脚映射（100-pin）

| 引脚 | 100-pin 脚号 | PWM 复用 | Alternative | PFC/PFCE/PFCAE | 定时器/通道 | 备注 |
|---|---:|---|---:|---|---|---|
| P10_2 | 100 | TAUD0O5 | 1st | 0/0/0 | TAUD0 CH5 | 从通道 |
| P10_0 | 98 | TAUD0O1 / TAUJ1O3 | 1st / 6th | 0/0/0 或 1/0/1 | TAUD0 CH1 / TAUJ1 CH3 | 方案 B 用 TAUJ1O3 |
| P0_2 | 8 | TAUD0O6 / TAUJ2O3 | 1st / 6th | 0/0/0 或 1/0/1 | TAUD0 CH6 / TAUJ2 CH3 | 方案 B 用 TAUJ2O3 |
| P10_3 | 1 | TAUD0O7 | 1st | 0/0/0 | TAUD0 CH7 | 从通道 |
| **P0_6** | 13 | **PWGA35O** | **5th** | **0/0/1** | **PWGA35** | **无 TAU/TAUB/TAUD** |
| **P10_14** | 88 | **TAUB0O7** | **4th** | **1/1/0** | **TAUB0 CH7** | 从通道，主通道 CH4 |
| P8_5 | 46 | TAUJ0O3 | 1st | 0/0/0 | TAUJ0 CH3 | 40 MHz 时钟域 |
| **P10_6** | 80 | **TAUD0O13** | **1st** | **0/0/0** | **TAUD0 CH13** | 主通道 CH12，80 MHz |
| P10_8 | 82 | TAUD0O10 / TAUJ3O2 | 1st / 7th | 0/0/0 或 0/1/1 | TAUD0 CH10 / TAUJ3 CH2 | 方案 B 用 TAUJ3O2 |

**复用编号 → PFC/PFCE/PFCAE：**

| Alternative | PFC | PFCE | PFCAE |
|---:|---:|---:|---:|
| 1st | 0 | 0 | 0 |
| 2nd | 1 | 0 | 0 |
| 3rd | 0 | 1 | 0 |
| 4th | 1 | 1 | 0 |
| 5th | 0 | 0 | 1 |
| 6th | 1 | 0 | 1 |
| 7th | 0 | 1 | 1 |

输出模式还要满足：`PM = 0`、`PMC = 1`、`PIPC = 0`、`PIBC = 0`、`PBDC = 0`。

---

## 2. 通道分配：九路独立频率（方案 B）

| 引脚 | 定时器 | 主通道 | 从通道 | 时钟域 | 最大时钟 | PFC alt |
|---|---|---|---:|---|---:|---:|
| P10_2 | TAUD0 | CH4 | CH5 | IPERI1 | 80 MHz | 0 |
| P10_0 | TAUJ1 | CH0 | CH3 | IPERI1 | 80 MHz | 5 |
| P0_2 | TAUJ2 | CH0 | CH3 | ATAUJ | 40 MHz | 5 |
| P10_3 | TAUD0 | CH6 | CH7 | IPERI1 | 80 MHz | 0 |
| P0_6 | PWGA35 | — | PWGA35 | IPERI2 | 40 MHz | 4 |
| P10_14 | TAUB0 | CH4 | CH7 | IPERI2 | 40 MHz | 3 |
| P8_5 | TAUJ0 | CH0 | CH3 | ATAUJ | 40 MHz | 0 |
| P10_6 | TAUD0 | CH12 | CH13 | IPERI1 | 80 MHz | 0 |
| P10_8 | TAUJ3 | CH0 | CH2 | IPERI1 | 80 MHz | 6 |

### 为什么 P10_3 用 CH6 主通道？

方案 B 把 P0_2 改用 TAUJ2O3，TAUD0 CH6 就空出来了。CH6 作为偶数主通道、CH7 作为从通道，P10_3 就能独立调频，不再和 P10_2 共用 CH4。

### 如果频率共用也可以（方案 A）

- P10_0 → TAUD0O1（1st）
- P0_2 → TAUD0O6（1st）
- P10_8 → TAUD0O10（1st）
- P10_2 / P0_2 / P10_3 共用 TAUD0 CH4 周期
- 优点：P10_0 / P0_2 / P10_8 都能用 80 MHz，占空比分辨率更好
- 缺点：三路同频

---

## 3. 200–400 kHz 可行性

### 3.1 时钟上限

| 时钟域 | 使用它的通道 | 最大频率 | 200 kHz 计数 | 400 kHz 计数 | 400 kHz 占空比步进 |
|---|---|---:|---:|---:|---:|
| CKSCLK_IPERI1 | TAUD0、TAUJ1、TAUJ3 | 80 MHz | 400 | 200 | 0.50% |
| CKSCLK_IPERI2 | TAUB0、PWBA/PWGA/PWSA | 40 MHz | 200 | 100 | 1.00% |
| CKSCLK_ATAUJ | TAUJ0、TAUJ2 | 40 MHz | 200 | 100 | 1.00% |

### 3.2 典型参数（50% 占空比，80 MHz）

| 目标 | N | CDR_M | CDR_S | 实际频率 | 误差 |
|---|---:|---:|---:|---:|---:|
| 200 kHz | 400 | 399 | 200 | 200.000 kHz | 0% |
| 250 kHz | 320 | 319 | 160 | 250.000 kHz | 0% |
| 300 kHz | 267 | 266 | 134 | 299.625 kHz | -0.125% |
| 350 kHz | 229 | 228 | 115 | 349.345 kHz | -0.187% |
| 400 kHz | 200 | 199 | 100 | 400.000 kHz | 0% |

### 3.3 典型参数（50% 占空比，40 MHz）

| 目标 | N | CDR_M | CDR_S | 实际频率 | 误差 |
|---|---:|---:|---:|---:|---:|
| 200 kHz | 200 | 199 | 100 | 200.000 kHz | 0% |
| 250 kHz | 160 | 159 | 80 | 250.000 kHz | 0% |
| 300 kHz | 133 | 132 | 67 | 300.752 kHz | +0.251% |
| 350 kHz | 114 | 113 | 57 | 350.877 kHz | +0.251% |
| 400 kHz | 100 | 99 | 50 | 400.000 kHz | 0% |

### 3.4 分辨率结论

- 80 MHz 通道在 400 kHz 时仍有 **0.5%** 的占空比步进；
- 40 MHz 通道只有 **1%**；
- 如果 P0_6 / P10_14 / P8_5 需要更细的占空比，硬件上只能换引脚或外部 PWM；软件只能做占空比抖动（dithering），但会引入额外纹波。

---

## 4. 公式与寄存器

### 4.1 TAUD / TAUB / TAUJ

```text
计数时钟 = PCLK / 2^PRS      (CK0, PRS = 0..15)
周期     = (CDR_master + 1) × 计数时钟周期
频率     = 计数时钟 / (CDR_master + 1)
占空比   = CDR_slave / (CDR_master + 1) × 100%

0%   : CDR_slave = 0
100% : CDR_slave ≥ CDR_master + 1
```

- TAUD0 / TAUB0 是 **16 位**；
- TAUJ 是 **32 位**；
- 同一 group 的所有通道必须使用相同 CKS；
- 不同 group 可以用不同 CKS。

### 4.2 PWGA35

```text
PWMCLK = PCLK                    (BRS = 0)
PWMCLK = PCLK / (2 × BRS)        (BRS = 1..2047)

PWM周期 = (PWGAPRD + 1) × PWMCLK周期
占空比  = (CRDR - CSDR) / (PWGAPRD + 1) × 100%

CSDR = 0，CRDR = 高电平计数
CRDR 的 bit12 = 1 → 输出强制高（100%）
```

- PWGAPRD 是 12 位；
- PWGAPRD 在计数期间禁止修改；改频率要先停 PWGA35，再改周期并重启。

### 4.3 关键寄存器

| 寄存器 | 作用 | 本方案设置 |
|---|---|---|
| `TAUD0.CMORn` | 通道模式 | 主：MAS=1、STS=000、MD=0000、MD0=1；从：MAS=0、STS=100、MD=0100、MD0=1 |
| `TAUD0.CDRn` | 周期/占空比数据 | 主 = 周期-1；从 = 占空计数 |
| `TAUD0.TOE/TOM/TOC/TOL` | 从通道输出模式 | TOE=1、TOM=1、TOC=0、TOL=0（正逻辑） |
| `TAUD0.RDE/RDM/RDS/RDC` | 同时重写 | RDE=1，其余 0（主从一致） |
| `TAUD0.TS/TT` | 启动/停止触发 | 主从同时置 1 |
| `TAUB0.*` | TAUB 同构 | CH4 主 / CH7 从 |
| `TAUJ*.CMOR/CDR/TOE/TOM/TOC/TOL` | 32 位 PWM | CH0 主 / CH3 或 CH2 从 |
| `PWGA35.CTL/TCR/CSDR/CRDR` | PWGA 控制与占空比 | CKS=00、TOE=1、CSDR=0、CRDR=占空计数 |
| `PWBA0.BRS0/TS` | PWGA 时钟分频/启动 | BRS0=0（PCLK）、TS0=1 |
| `SLPWGA1 / PWGAPRDSL1` | 启动 PWGA35 / 选择周期源 | bit3 = 1 |

---

## 5. 代码

### 5.1 pwm9.h

```c
/* pwm9.h - RH850/F1KM-S1 9-channel PWM driver (CS+ / CCRH850)
 *
 * Target pins:
 *   P10_2, P10_0, P0_2, P10_3, P0_6, P10_14, P8_5, P10_6, P10_8
 *
 * Frequency range: 200 kHz - 400 kHz
 * Duty range:      0 - 1000 permille (0.0% - 100.0%)
 *
 * The clock macros below are maximum values from the F1KM-S1 electrical
 * characteristics. Replace them with the actual clock-tree values of your
 * project if the clocks are divided down.
 */
#ifndef PWM9_H
#define PWM9_H

#include <stdint.h>

typedef enum {
    PWM_P10_2 = 0,
    PWM_P10_0,
    PWM_P0_2,
    PWM_P10_3,
    PWM_P0_6,
    PWM_P10_14,
    PWM_P8_5,
    PWM_P10_6,
    PWM_P10_8,
    PWM_CH_COUNT
} pwm_ch_t;

#ifndef PWM_PCLK_IPERI1_HZ
#define PWM_PCLK_IPERI1_HZ  80000000UL   /* TAUD0, TAUJ1, TAUJ3 */
#endif

#ifndef PWM_PCLK_IPERI2_HZ
#define PWM_PCLK_IPERI2_HZ  40000000UL   /* TAUB0, PWBA/PWGA/PWSA */
#endif

#ifndef PWM_PCLK_ATAUJ_HZ
#define PWM_PCLK_ATAUJ_HZ   40000000UL   /* TAUJ0, TAUJ2 */
#endif

/* Configure all nine pins and start every channel at 250 kHz / 50%. */
void pwm9_init(void);

/* Set frequency and duty of one channel.
 * freq_hz: 200000 .. 400000
 * duty_permille: 0 .. 1000
 * Returns 0 on success, -1 on error.
 */
int pwm9_set_freq_duty(pwm_ch_t ch, uint32_t freq_hz, uint16_t duty_permille);

/* Update duty only. The frequency is kept unchanged.
 * For TAUD/TAUB/TAUJ the new value is applied at the next period.
 * For PWGA35 the new value is applied at the next falling edge.
 */
int pwm9_set_duty(pwm_ch_t ch, uint16_t duty_permille);

/* Read back the actual generated frequency/duty (after integer rounding). */
uint32_t pwm9_get_actual_freq(pwm_ch_t ch);
uint16_t pwm9_get_actual_duty(pwm_ch_t ch);

#endif /* PWM9_H */

```

### 5.2 pwm9.c

```c
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
 *   P0_6   : PWGA35                              PWGA35O
 *   P10_14 : TAUB0 CH4 (master) -> CH7 (slave)   TAUB0O7
 *   P8_5   : TAUJ0 CH0 (master) -> CH3 (slave)   TAUJ0O3
 *   P10_6  : TAUD0 CH12 (master) -> CH13 (slave) TAUD0O13
 *   P10_8  : TAUJ3 CH0 (master) -> CH2 (slave)   TAUJ3O2
 *
 * Comments are ASCII-only to avoid CS+ source encoding issues.
 */
#include "pwm9.h"

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
#define PWGA35_BASE  (0xFFE71000UL + 0x40UL * 35UL)
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
/* PWGA35 / PWBA0 / PWSA0 registers (Section 37.2)                     */
/* ------------------------------------------------------------------ */
#define PWGA35_CSDR    (*(volatile uint16_t *)(PWGA35_BASE + 0x00UL))
#define PWGA35_CRDR    (*(volatile uint16_t *)(PWGA35_BASE + 0x04UL))
#define PWGA35_RDT     (*(volatile uint8_t  *)(PWGA35_BASE + 0x0CUL))
#define PWGA35_RSF     (*(volatile uint8_t  *)(PWGA35_BASE + 0x10UL))
#define PWGA35_TCR     (*(volatile uint8_t  *)(PWGA35_BASE + 0x18UL))
#define PWGA35_CTL     (*(volatile uint8_t  *)(PWGA35_BASE + 0x20UL))

#define PWBA0_BRS0     (*(volatile uint16_t *)(PWBA0_BASE + 0x00UL))
#define PWBA0_TE       (*(volatile uint16_t *)(PWBA0_BASE + 0x10UL))
#define PWBA0_TS       (*(volatile uint16_t *)(PWBA0_BASE + 0x14UL))
#define PWBA0_TT       (*(volatile uint16_t *)(PWBA0_BASE + 0x18UL))

#define PWSA0_CTL      (*(volatile uint8_t  *)(PWSA0_BASE + 0x00UL))
#define PWGAPRD        (*(volatile uint16_t *)(SLPW_BASE + 0x0CUL))
#define PWGAPRDSL1     (*(volatile uint32_t *)(SLPW_BASE + 0x14UL))
#define SLPWGA1        (*(volatile uint32_t *)(SLPW_BASE + 0x04UL))

/* ------------------------------------------------------------------ */
/* Channel hardware table                                             */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  type;      /* 0 = TAUD0, 1 = TAUB0, 2 = TAUJ, 3 = PWGA35 */
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
    { 3U, 0UL,         0U,  0U, PWM_PCLK_IPERI2_HZ },  /* P0_6   */
    { 1U, TAUB0_BASE,  4U,  7U, PWM_PCLK_IPERI2_HZ },  /* P10_14 */
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
/* PWGA35 PWM                                                         */
/* ------------------------------------------------------------------ */
static uint16_t pwga_duty_counts(uint16_t period, uint16_t duty_permille)
{
    uint32_t high;

    if (duty_permille == 0U) {
        return 0U;
    }
    if (duty_permille >= 1000U) {
        return 0x1000U;   /* PWGA35_CRDR bit12 = 1 -> fixed high */
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

    /* Stop PWGA35 and PWMCLK0 before changing the period. */
    SLPWGA1 &= ~(1UL << 3);
    PWBA0_TT = 0x0001U;
    while ((PWBA0_TE & 0x0001U) != 0U) {
        /* wait until PWMCLK0 is stopped */
    }
    PWSA0_CTL = 0x00U;

    /* Start PWMCLK0. BRS0 = 0 -> PWMCLK0 = PCLK. */
    PWBA0_BRS0 = (uint16_t)brs;
    PWSA0_CTL = 0x01U;
    PWBA0_TS = 0x0001U;

    PWGA35_CTL = 0x00U;                  /* CKS=00: PWMCLK0 */
    PWGAPRDSL1 |= (1UL << 3);            /* PWGA35 uses PWGAPRD */
    PWGAPRD = period;
    PWGA35_CSDR = 0x0000U;
    PWGA35_CRDR = crdr;
    PWGA35_TCR = 0x01U;                  /* TOE = 1 */
    SLPWGA1 |= (1UL << 3);               /* start PWGA35 */
    return 0;
}

static void pwga_set_duty(uint16_t duty_permille)
{
    uint16_t period = PWGAPRD;
    uint16_t crdr = pwga_duty_counts(period, duty_permille);

    PWGA35_CRDR = crdr;
    PWGA35_RDT = 0x01U;                  /* request simultaneous rewrite */
    while ((PWGA35_RSF & 0x01U) != 0U) {
        /* wait until the compare buffer is updated */
    }
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
    /* P0_6   : PWGA35O,   5th alternative */
    port_alt(0U, 6U, 4U);
    /* P10_14 : TAUB0O7,   4th alternative */
    port_alt(10U, 14U, 3U);
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
        return set_taub(h->master, h->slave, h->pclk, freq_hz, duty_permille);
    case 2U:
        return set_tauj(h->base, h->master, h->slave, h->pclk, freq_hz, duty_permille);
    case 3U:
        return set_pwga(h->pclk, freq_hz, duty_permille);
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
        TAUB_CDR(h->slave) = duty_counts16(TAUB_CDR(h->master), duty_permille);
        return 0;
    case 2U:
        TAUJ_CDR(h->base, h->slave) =
            duty_counts32(TAUJ_CDR(h->base, h->master), duty_permille);
        return 0;
    case 3U:
        pwga_set_duty(duty_permille);
        return 0;
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
        clk = h->pclk >> (TAUB_TPS & 0x000FU);
        period = (uint32_t)TAUB_CDR(h->master) + 1UL;
        break;
    case 2U:
        clk = h->pclk >> (TAUJ_TPS(h->base) & 0x000FU);
        period = TAUJ_CDR(h->base, h->master) + 1UL;
        break;
    case 3U:
        brs = (uint32_t)PWBA0_BRS0;
        clk = (brs == 0UL) ? h->pclk : (h->pclk / (2UL * brs));
        period = (uint32_t)PWGAPRD + 1UL;
        break;
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
        period = (uint32_t)TAUB_CDR(h->master) + 1UL;
        high = (uint32_t)TAUB_CDR(h->slave);
        break;
    case 2U:
        period = TAUJ_CDR(h->base, h->master) + 1UL;
        high = TAUJ_CDR(h->base, h->slave);
        break;
    case 3U:
        if ((PWGA35_CRDR & 0x1000U) != 0U) {
            return 1000U;
        }
        period = (uint32_t)PWGAPRD + 1UL;
        high = (uint32_t)PWGA35_CRDR - (uint32_t)PWGA35_CSDR;
        break;
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

```

### 5.3 main_pwm_example.c

```c
/* main_pwm_example.c - CS+ / CCRH850 example
 *
 * Call pwm9_init() after the clock tree, module clocks and ports are ready.
 * In a Smart Configurator project this is normally after R_SystemInit().
 */
#include "pwm9.h"

void main(void)
{
    /* TODO: enable the module clocks for TAUD0, TAUB0, TAUJ0..3,
     *       PWBA0, PWGA35 and PWSA0 (Smart Configurator or MSTPCR).
     */

    pwm9_init();   /* 250 kHz, 50% on all nine channels */

    /* P10_6: 350 kHz, 25% */
    (void)pwm9_set_freq_duty(PWM_P10_6, 350000UL, 250U);

    /* P10_14: keep frequency, change duty to 75% */
    (void)pwm9_set_duty(PWM_P10_14, 750U);

    /* P10_0: 400 kHz, 10% */
    (void)pwm9_set_freq_duty(PWM_P10_0, 400000UL, 100U);

    for (;;) {
        /* application loop */
    }
}

```

### 5.4 使用说明

`pwm9_init()` 默认把九路设为 **250 kHz / 50%**。之后：

```c
/* 设置频率 + 占空比：350 kHz, 25% */
pwm9_set_freq_duty(PWM_P10_6, 350000UL, 250U);

/* 只改占空比：75% */
pwm9_set_duty(PWM_P10_14, 750U);

/* 读回实际值 */
uint32_t f = pwm9_get_actual_freq(PWM_P10_6);
uint16_t d = pwm9_get_actual_duty(PWM_P10_6);
```

占空比单位是**千分比**：500 = 50.0%，1000 = 100%。

---

## 6. CS+ 集成步骤

1. **新建 / 打开 CS+ 工程**  
   选择 RH850/F1KM-S1 对应型号（R7F7016xx，按具体料号选择），确认使用 CCRH850 编译器。

2. **加入文件**  
   把 `pwm9.c`、`pwm9.h` 加入工程；`main_pwm_example.c` 只作参考，把内容合并到你的 `main.c`。

3. **使能模块时钟**  
   在 Smart Configurator 或 `MSTPCR` 中使能 TAUD0、TAUB0、TAUJ0–3、PWBA0、PWGA35、PWSA0。  
   时钟没开时，寄存器写入会被忽略，引脚也不会有波形。

4. **核对 PCLK 宏**  
   打开 `pwm9.h`，把 `PWM_PCLK_IPERI1_HZ`、`PWM_PCLK_IPERI2_HZ`、`PWM_PCLK_ATAUJ_HZ` 改成工程时钟树里的实际频率。  
   默认值是手册最大值：80 / 40 / 40 MHz。

5. **调用初始化**  
   在时钟、模块时钟、端口初始化完成之后调用 `pwm9_init()`。不要在 `R_SystemInit()` 之前调用。

6. **设置目标参数**  
   例如 `pwm9_set_freq_duty(PWM_P10_6, 350000UL, 250U)` 表示 P10_6 输出 350 kHz、25%。

7. **编译 / 烧录**  
   Clean Build → Build → 烧录。若端口保护开启，先按手册 Section 5 解锁 `PPCMD`；否则端口复用寄存器可能写不进去。

8. **测量确认**  
   用示波器/逻辑分析仪测 P10_6、P10_14、P0_6。先看频率，再看占空比，最后看上升沿抖动。

> ⚠️ 如果工程里已经用 Smart Configurator 生成了 TAUJ/TAUD/TAUB 的初始化代码，不要再让两套代码同时配置同一通道。保留一套。

---

## 7. 调试清单

| 现象 | 最可能原因 | 处理 |
|---|---|---|
| 引脚完全没有波形 | PMC/PM/PFC 没配对，或模块时钟没开 | 检查 PMC=1、PM=0、PIPC=0、PFC/PFCE/PFCAE；检查 MSTPCR / Smart Configurator 时钟 |
| P0_6 没有波形 | PWGA 启动顺序不完整 | 确认 PWSAnCTL=1 → PWBAnTS0=1 → SLPWGA1 bit3=1；PWGA35_TCR.TOE=1 |
| P10_14 没有波形 | TAUB0 CH7 被当成主通道，或 PFC 用了 1st | CH4 主 / CH7 从；PFC10_14=1、PFCE10_14=1、PFCAE10_14=0 |
| 频率偏大/偏小一倍 | PCLK 宏写错，或预分频 PRS 不是 0 | 核对实际 IPERI1/IPERI2/ATAUJ 频率；检查 TPS 的 PRS0 |
| 占空比反相 | TOL 极性设反 | TOL=0 正逻辑；TOL=1 负逻辑 |
| 占空比 0% / 100% 不干净 | CDR_slave 边界处理 | 0% 用 CDR_slave=0；100% 用 CDR_slave ≥ CDR_master+1（PWGA 用 CRDR bit12） |
| 改频率时波形抖动/中断 | PWGA 改 PWGAPRD 必须先停 | 停 SLPWGA1 → 改 PWGAPRD → 重启；或用 RDT/RSF 只改占空比 |
| 两路互相影响 | 它们共用了同一个主通道 | 方案 B 已把 P10_2/P10_3 分到 CH4/CH6；检查是否误用了方案 A 的 CH4 |

---

## 8. 参考资料

- Renesas **RH850/F1KH, RH850/F1KM User's Manual: Hardware**，R01UH0684EJ0130 Rev.1.30，2021-09-30。
- 引脚复用：Section 2C.10，Table 2C.41（P0）、Table 2C.43（P8）、Table 2C.47（P10）。
- TAUB PWM：Section 31.14.1；TAUD PWM：Section 32.15.1；TAUJ PWM：Section 33.13.1。
- PWM-Diag / PWGA：Section 37.1–37.3，寄存器 37.2.1。
- 时钟上限：Section 47A.3.1 电气特性。
- 手册：Renesas R01UH0684EJ0130（因版权/体积未随仓库分发）
- 官方下载页：<https://www.renesas.com/en/document/mah/rh850f1km-rh850f1kh-users-manual-hardware>

---

> 本文按 100-pin LQFP 和“200–400 kHz、频率/占空比可调”整理。若实际封装或时钟树不同，请以对应型号手册和 Smart Configurator 配置为准。
