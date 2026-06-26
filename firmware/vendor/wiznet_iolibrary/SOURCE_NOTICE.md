# WIZnet ioLibrary Driver 来源说明

- 原始库名称：WIZnet ioLibrary_Driver
- 原始来源：https://github.com/Wiznet/ioLibrary_Driver
- 引入方式：作为 Git submodule 添加到 `external/ioLibrary_Driver`，再复制必要源码到本目录参与固件构建
- 复制日期：2026-06-26
- 许可证：MIT License，见原始库 `license.txt`
- 本地用途：为 W5500 以太网控制器提供官方寄存器访问、网络配置和 socket API
- 本地修改：本目录下复制的官方源码当前保持原样；项目相关 SPI/CS/复位适配放在 `firmware/drivers/net/src/net_w5500.c`

当前参与构建的文件：

- `Ethernet/wizchip_conf.c`
- `Ethernet/wizchip_conf.h`
- `Ethernet/socket.c`
- `Ethernet/socket.h`
- `Ethernet/W5500/w5500.c`
- `Ethernet/W5500/w5500.h`
- `Internet/DHCP/dhcp.c`
- `Internet/DHCP/dhcp.h`

编译时必须显式定义：

- `_WIZCHIP_=W5500`
