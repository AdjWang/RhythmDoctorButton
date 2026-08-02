# 制作流程

## 电路

使用[立创 `EDA`](https://lceda.cn/)绘制，电路图 `DRC` 生成 `1` 个电源网络短接警告，`PCB` `DRC` 生成多个板载天线的连接和间距警告，均可忽略，希望软件后续版本可以妥善处理这些场景。

![电路图](pictures/make/SCH_Schematic1_1-P1_2026-08-03.svg)

![PCB](pictures/make/PCB_PCB1_2026-08-03.png)

`NRF52840` 植球。

<p align="center">
  <img src="pictures/make/nrf52840-bm.jpg" alt="Ball mounting" width="50%">
</p>

焊接电路。加热台真好用，可惜只能焊一面。

<p align="center">
  <img src="pictures/make/board-solider.jpg" alt="Ball mounting">
</p>

安装电池。
# TODO: add battery tape

成品电路板。

<img src="pictures/make/board-front.jpg" alt="Board front" width="50%"><img src="pictures/make/board-back.jpg" alt="Board back" width="50%">

### 注意事项：

- 材质使用 `FC2` 即可。
- 板厚 `1mm`。
- `USB Type-C` 座子要选用加长款，当前用料 `MC-121-L124`。
- 用于 `ADC` 电池电压检测的分压电阻精度必须不低于 `1%`。
- 复位按钮在板子正反面都留了位置，功能相同，正面的只用于调试，成品只焊接背面的即可。
- 固定电池使用的 `2 mm` 厚双面胶建议使用 `3M VHB 4991` 无痕双面胶。常见的白色泡棉双面胶质量太差，一段时间后胶面会脱离泡棉导致电池脱落。

## 固件

安装环境，编译，烧录 bootloader，下载 `uf2` 格式固件

# TODO: add debugging clip

## 外壳

除壳体外的材料：

- 碳钢 `M2*5` 螺栓 `3`个。
- 圆柱形磁铁，直径 `4 mm`，高度 `8 mm` `3` 个。
- 半球形硅胶防滑垫 `6*2mm` `3` 个。

外壳使用 `Fusion 360` 绘制，分为三部分：按钮，壳体和底座。使用光固化 3D 打印制作。

壳体上有 `3` 个 `M2` 螺栓孔，需要手动攻丝。因为底座的磁铁需要吸附到螺栓，所以螺栓材质一定要使用强磁性的，比如碳钢，不能用弱磁性材料，比如不锈钢。

<img src="pictures/make/tap.jpg" alt="Tap" width="50%"><img src="pictures/make/tapping.jpg" alt="Tapping" width="50%">

### 注意事项：

- 丝锥太小，塑料材质也比较软，排屑困难，大约每进给 `3 mm` 左右需要退出来手动清理下丝锥排屑槽，不然容易滑牙。
