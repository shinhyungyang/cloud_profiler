#ifndef __COUNTERS_AARCH64_H__
#define __COUNTERS_AARCH64_H__

/* All counters, including PMCCNTR_EL0, are disabled/enabled */
#define QUADD_ARMV8_PMCR_E      (1 << 0)
/* Reset all event counters, not including PMCCNTR_EL0, to 0 */
#define QUADD_ARMV8_PMCR_P      (1 << 1)
/* Reset PMCCNTR_EL0 to 0 */
#define QUADD_ARMV8_PMCR_C      (1 << 2)
/* Clock divider: PMCCNTR_EL0 counts every clock cycle/every 64 clock cycles */
#define QUADD_ARMV8_PMCR_D      (1 << 3)
/* Export of events is disabled/enabled */
#define QUADD_ARMV8_PMCR_X      (1 << 4)
/* Disable cycle counter, PMCCNTR_EL0 when event counting is prohibited */
#define QUADD_ARMV8_PMCR_DP     (1 << 5)
/* Long cycle count enable */
#define QUADD_ARMV8_PMCR_LC     (1 << 6)

/* Mask for writable bits */
#define	QUADD_ARMV8_PMCR_WR_MASK	0x3f

#endif
