# FlashDB 来源说明

- 原始库名称：FlashDB
- 原始来源：https://github.com/armink/FlashDB
- 引入方式：Git submodule，位于 `external/FlashDB`
- 当前 commit：`8236571f6e29273a16bba62061bf0405e4186878`
- 许可证：Apache-2.0
- 复制日期：2026-06-23
- 本地修改：`firmware/vendor/flashdb` 下仅作为参与固件编译的源码副本，源码内容保持上游原样；本项目配置和移植代码放在 `firmware/external_port/flashdb_port`

## 参与编译的文件

- `inc/flashdb.h`
- `inc/fdb_def.h`
- `inc/fdb_low_lvl.h`
- `src/fdb.c`
- `src/fdb_kvdb.c`
- `src/fdb_tsdb.c`
- `src/fdb_utils.c`
- `port/fal/inc/fal.h`
- `port/fal/inc/fal_def.h`
- `port/fal/src/fal.c`
- `port/fal/src/fal_flash.c`
- `port/fal/src/fal_partition.c`

