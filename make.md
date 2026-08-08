# 制作流程

> 模型和电路在拍照留念后有小幅改动，最新版本细节可能与图片有所差异。

## 电路

使用[嘉立创 `EDA`](https://lceda.cn/)绘制，电路图 `DRC` 生成 `1` 个电源网络短接警告，`PCB` `DRC` 生成多个板载天线的连接和间距警告，均可忽略，希望软件后续版本可以妥善处理这些场景。

<p align="center">
  <img src="pictures/make/SCH_Schematic1_1-P1_2026-08-03.svg" alt="Schematic">
</p>

<p align="center">
  <img src="pictures/make/board-3d-front.jpg" alt="Board 3d front" width="50%"><img src="pictures/make/board-3d-back.jpg" alt="Board 3d back" width="50%">
</p>

焊接，安装电池。

<p align="center">
  <img src="pictures/make/board-solider.jpg" alt="Board solider" width="50%"><img src="pictures/make/battery.jpg" alt="Add battery" width="50%">
</p>

成品电路板。

<p align="center">
  <img src="pictures/make/board-front.jpg" alt="Board front" width="50%"><img src="pictures/make/board-back.jpg" alt="Board back" width="50%">
</p>

### 注意事项：

- 材质使用 `FR-4` 即可。
- 板厚 `1mm`。
- `USB Type-C` 座子要选用加长款，当前用料 `MC-121-L124`。
- 用于 `ADC` 电池电压检测的分压电阻精度必须不低于 `1%`。
- 复位按钮在板子正反面都留了位置，功能相同，正面的只用于调试，成品只焊接背面的即可。
- 固定电池使用的 `2 mm` 厚双面胶建议使用 `3M VHB 4991` 无痕双面胶。常见的白色泡棉双面胶质量太差，一段时间后胶面会脱离泡棉导致电池脱落。

## 固件

电路板上预留了两排 `2.54mm` 的孔位，分别是 `SWD` 和串口。因为外壳空间有限，所以建议使用测试夹，不要焊接排针。

安装环境，编译，使用 `JLINK` 或者 `DAPLINK` 给芯片烧录 `bootloader` 然后下载 `uf2` 固件。

<p align="center">
  <img src="pictures/make/board-flash.jpg" alt="Debugging clip" width="70%">
</p>

## 外壳

外壳使用 `Fusion 360` 绘制，分为三部分：按钮盖，壳体和底盖。委托[嘉立创 3D 打印](https://www.jlc-3dp.cn/)代加工，使用光固化 3D 打印工艺，按钮盖选用 `JLC` 全彩树脂，颜色 `RGB 245,14,22`；壳体和底盖选用 `8228` 浅绿色树脂，打磨。

除壳体外的材料：

- 碳钢 `M2*5` 螺栓 `3`个。
- 圆柱形磁铁，直径 `4 mm`，高度 `8 mm` `3` 个。
- 半球形硅胶防滑垫 `6*2mm` `3` 个。

<p align="center">
  <img src="pictures/make/model-splited.jpg" alt="Model splited" width="50%"><img src="pictures/make/model-unified.jpg" alt="Model unified" width="50%">
</p>

壳体上有 `3` 个 `M2` 螺栓孔，需要手动攻丝。因为底座的磁铁需要吸附到螺栓，所以螺栓材质一定要使用强磁性的，比如碳钢，不能用弱磁性材料，比如不锈钢。

<p align="center">
  <img src="pictures/make/tap.jpg" alt="Tap" width="50%"><img src="pictures/make/tapping.jpg" alt="Tapping" width="50%">
</p>

把圆柱磁铁安装到底盖的空位里。磁铁孔位做成通孔，方便拆卸磁铁。

<p align="center">
  <img src="pictures/make/button-base-magnet.jpg" alt="Magnet" width="50%"><img src="pictures/make/button-base-front.jpg" alt="Added view" width="50%">
</p>

给底盖贴装防滑硅胶垫。

<p align="center">
  <img src="pictures/make/button-base-back.jpg" alt="Add anti-slip pad" width="70%">
</p>

### 注意事项：

- 丝锥太小，塑料材质也比较软，排屑困难，大约每进给 `3 mm` 左右需要退出来手动清理下丝锥排屑槽，不然容易滑牙。

## 总装

<p align="center">
  <img src="pictures/make/button-bottom.jpg" alt="Buttom view" width="50%"><img src="pictures/make/button-key.jpg" alt="Key view" width="50%">
</p>

扫描蓝牙设备，连接 `RhythmDoctorButton`，也可以不用蓝牙，直接用有线连接。如果同时连接，使用有线传输数据。

<p align="center">
  <img src="pictures/make/ble-advertise.jpg" alt="Ble advertise" width="50%"><img src="pictures/make/ble-connected.jpg" alt="Ble connected" width="50%">
</p>

Rhythm Doctor, 启动！

<p align="center">
  <img src="pictures/make/rhythmdoctor-overview.jpg" alt="Rhythm Doctor overview">
</p>

<p align="center">
  <img src="pictures/make/rdb-unpressed.png" alt="Ingame unpressed snapshot" width="50%"><img src="pictures/make/rdb-pressed.png" alt="Ingame pressed snapshot" width="50%">
</p>

<p align="center">
  <img src="pictures/make/button-unpressed.jpg" alt="Test unpressed" width="50%"><img src="pictures/make/button-pressed.jpg" alt="Test pressed" width="50%">
</p>
