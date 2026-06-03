// Some types in USBHIDKeyboard.h is conflicting with BleKeyboard.h, hide
// implementation to resolve.
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <cstdint>
#include <memory>

namespace rdb {

class IKeyboard {
 public:
  virtual ~IKeyboard() {};
  virtual bool is_connected() const = 0;
  virtual void Begin() = 0;
  virtual void End() = 0;
  virtual void Press(uint8_t key) = 0;
  virtual void Release(uint8_t key) = 0;
  virtual void ReleaseAll() = 0;
  virtual void SetBatteryLevel(uint8_t lvl) = 0;
};

class BleKeyboardImpl;

class BleKeyboard : public IKeyboard {
 public:
  explicit BleKeyboard(std::string_view device_name,
                       std::string_view device_manufacturer);
  ~BleKeyboard() override;
  bool is_connected() const override;
  void Begin() override;
  void End() override;
  void Press(uint8_t key) override;
  void Release(uint8_t key) override;
  void ReleaseAll() override;
  void SetBatteryLevel(uint8_t lvl) override;
 
 private:
  std::unique_ptr<BleKeyboardImpl> impl_;
};

class UsbKeyboardImpl;

class UsbKeyboard : public IKeyboard {
 public:
  UsbKeyboard(std::string_view device_name,
              std::string_view device_manufacturer);
  ~UsbKeyboard() override;
  bool is_connected() const override;
  void Begin() override;
  void End() override;
  void Press(uint8_t key) override;
  void Release(uint8_t key) override;
  void ReleaseAll() override;
  void SetBatteryLevel(uint8_t lvl) override;
 
 private:
  std::unique_ptr<UsbKeyboardImpl> impl_;
};

}  // namespace rdb

#endif
