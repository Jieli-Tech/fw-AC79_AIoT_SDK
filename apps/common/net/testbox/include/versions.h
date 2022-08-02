#ifndef _WB_VERSIONS_H_
#define _WB_VERSIONS_H_

/*****************************CLIENT*****************************
 *      版本号   ：v1.0.0
 *      更新日期 ：2021/04/05
 *      更新内容 ：建立初始版本
 *
 *
 *      版本号   ：v1.0.1
 *      更新日期 ：2021/05/14
 *      更新内容 ：1.优化频偏测试
 *                 2.增加完成测试后不重启选项
 *
 *      版本号   ：v1.0.2
 *      更新日期 ：2021/06/09
 *      更新内容 ：1.优化干扰问题
 *                 2.优化测试时间
 *
 *      版本号   ：v1.0.3
 *      更新日期 ：
 *      更新内容 ：1.优化wbcp、测试流程
 *
 */

#define WB_MAJOR_VERSION(v)       (((v) >> 16) & 0xff)
#define WB_MINOR_VERSION(v)       (((v) >> 8) & 0xff)
#define WB_REVISION_VERSION(v)    ((v) & 0xff)
#define WB_VERSIONS(major, minor, revision)   ((major) << 16 | (minor) << 8 | (revision))

#define WB_CLIENT_VERSION         WB_VERSIONS(1, 0, 2)

#endif


