# LWB 测试完成后的代码更改清单

这个文件记录暂时不要动的后置修改。当前阶段优先保证：

- STM32L476 与 SX1280 通信正常
- Gloria/LWB 在新 PCB 上跑通
- radio SPI1、DIO1、BUSY、NRESET 先稳定

下面两个问题等 LWB 基础测试完成后再改。

## 1. 禁用 `RADIO_ANTSEL`

当前新 PCB 没有放 `SKY13587-378LF` 天线选择芯片，RF 路径是固定的：

```text
SX1280 RFIO -> 匹配/滤波网络 -> 外部 SMA/U.FL 天线座
```

因此新 PCB 不需要 `RADIO_ANTSEL`。

当前固件里还保留了旧定义：

```text
RADIO_ANTSEL -> PA9
```

但当前新 PCB 的后续规划是：

```text
PA9 -> COM_TXD
```

这就是 `PA9` 的冲突来源：

```text
同一个 MCU 引脚不能同时做 RADIO_ANTSEL 和 COM_TXD。
```

后续代码需要做：

1. 不再把 `PA9` 当成 `RADIO_ANTSEL` GPIO 输出。
2. 删除或禁用 `MX_GPIO_Init()` 里 `RADIO_ANTSEL_Pin` 的初始化。
3. 把 `SX1280AntSwOn()` 和 `SX1280AntSwOff()` 改成空函数。

目标代码形式：

```c
void SX1280AntSwOn(void)
{
    /* No antenna switch on this PCB. */
}

void SX1280AntSwOff(void)
{
    /* No antenna switch on this PCB. */
}
```

注意：这一步完成前，`PA9` 仍然会被当前代码当作 `RADIO_ANTSEL` 使用。

## 2. 增加 `COM_TXD/COM_RXD` 的 USART1 支持

PCB 上已经预留：

```text
COM_TXD -> PA9
COM_RXD -> PA10
```

在 STM32L476 上，这组引脚可以作为：

```text
PA9  -> USART1_TX
PA10 -> USART1_RX
```

当前代码只初始化了调试串口：

```text
USART2_TX -> PA2
USART2_RX -> PA3
```

也就是说，J400 上的 `COM_TXD/COM_RXD` 现在只是硬件连接好了，软件还没有真正启用。

后续代码需要做：

1. 新增 `UART_HandleTypeDef huart1;`
2. 新增 `MX_USART1_UART_Init()`
3. 在 `main()` 初始化流程中调用 `MX_USART1_UART_Init()`
4. 在 `HAL_UART_MspInit()` 中把 `PA9/PA10` 配成 `GPIO_AF7_USART1`
5. 在 `HAL_UART_MspDeInit()` 中加入 USART1 反初始化
6. 根据实际协议，把 COM 通信逻辑接到 `huart1`

关键限制：

```text
PA9 不能同时做 RADIO_ANTSEL 和 USART1_TX。
```

所以正确顺序是：

```text
先禁用 RADIO_ANTSEL
再启用 USART1 PA9/PA10
```

## 当前不要做的事

在 LWB 基础测试完成前，不要改：

- `RADIO_SPI = hspi1`
- SX1280 的 SPI1 引脚：`PA5/PA6/PA7/PA8`
- SX1280 的 `BUSY/DIO1/NRESET`
- LWB/Gloria 主流程

这样可以先把无线链路测稳，再处理 J400 的 COM 串口功能。
