#ifndef __WDT_H__
#define __WDT_H__

///  \cond DO_NOT_DOCUMENT
#define WDT_1MS 		0x00
#define WDT_2MS			0x01
#define WDT_4MS			0x02
#define WDT_8MS 		0x03
#define WDT_16MS 		0x04
#define WDT_32MS 		0x05
#define WDT_64MS 		0x06
#define WDT_128MS 		0x07
#define WDT_256MS 		0x08
#define WDT_512MS 		0x09
#define WDT_1S 			0x0A
#define WDT_2S 			0x0B
#define WDT_4S 			0x0C
#define WDT_8S 			0x0D
#define WDT_16S 		0x0E
#define WDT_32S 		0x0F
/// \endcond

/**
 * @brief wdt_init：看门狗初始化
 *
 * @param time 看门狗清狗时间
 *         |宏|数值|时间|
 *         |- |- |- |
 *         |WDT_1MS|0x00|1ms|
 *         |WDT_2MS|0x01|2ms|
 *         |WDT_4MS|0x02|4ms|
 *         |WDT_8MS|0x03|8ms|
 *         |WDT_16MS|0x04|16ms|
 *         |WDT_32MS|0x05|32ms|
 *         |WDT_64MS|0x06|64ms|
 *         |WDT_128MS|0x07|128ms|
 *         |WDT_256MS|0x08|256ms|
 *         |WDT_512MS|0x09|512ms|
 *         |WDT_1S|0x0A|1s|
 *         |WDT_2S|0x0B|2s|
 *         |WDT_4S|0x0C|4s|
 *         |WDT_8S|0x0D|8s|
 *         |WDT_16S|0x0E|16s|
 *         |WDT_32S|0x0F|32s|
 */
void wdt_init(u8 time);

/**
 * @brief wdt_close：关闭看门狗
 */
void wdt_close(void);

/**
 * @brief wdt_clear：清除看门狗
 */
void wdt_clear(void);

/**
 * @brief wdt_enable：使能看门狗
 */
void wdt_enable(void);

/**
 * @brief wdt_disable：不使能看门狗
 */
void wdt_disable(void);

#endif
