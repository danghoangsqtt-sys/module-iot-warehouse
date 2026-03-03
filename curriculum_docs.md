# TÀI LIỆU GIÁO TRÌNH: MODULE 1 - HỆ THỐNG KHO THÔNG MINH (v2.0)

Tài liệu này hướng dẫn chi tiết cách xây dựng và vận hành hệ thống giám sát kho thông minh sử dụng vi điều khiển ESP32, màn hình OLED, cảm biến môi trường (DHT22), ánh sáng (LDR), và phát hiện đột nhập (HC-SR04).

---

## 3. Sơ đồ đấu nối chi tiết theo từng chân

Dưới đây là bảng đấu nối các thành phần linh kiện trong hệ thống với board ESP32 (DevKit V4).

| Linh kiện               | Chân linh kiện |  Chân ESP32   | Ghi chú quan trọng                                           |
| :---------------------- | :------------: | :-----------: | :----------------------------------------------------------- |
| **OLED SSD1306**        |      VCC       |      3V3      | Nguồn cho màn hình OLED.                                     |
| (I2C)                   |      GND       |      GND      | Nối đất chung (Common Ground).                               |
|                         |      SCL       |    GPIO 22    | Chân xung nhịp I2C.                                          |
|                         |      SDA       |    GPIO 21    | Chân dữ liệu I2C.                                            |
| **Cảm biến DHT22**      |      VCC       |      3V3      | Nguồn cảm biến.                                              |
| (Temp & Humid)          |      DATA      |    GPIO 15    | Dữ liệu 1-Wire.                                              |
|                         |                | Pull-up 10kΩ  | Trở kéo lên 10kΩ giữa DATA và 3V3 (nếu module không có sẵn). |
|                         |      GND       |      GND      | Nối đất.                                                     |
| **Siêu âm HC-SR04**     |      VCC       | 3V3 (hoặc 5V) | Nguồn cấp cho cảm biến.                                      |
| (Khoảng cách)           |      TRIG      |    GPIO 5     | Chân kích hoạt phát sóng.                                    |
|                         |      ECHO      |    GPIO 18    | Chân nhận sóng phản hồi.                                     |
|                         |      GND       |      GND      | Nối đất.                                                     |
| **Cảm biến LDR**        |      VCC       |      3V3      | Nguồn cấp (Phân áp).                                         |
| (Ánh sáng)              |       AO       |    GPIO 34    | Chân đọc tín hiệu Analog.                                    |
|                         |      GND       |      GND      | Nối đất.                                                     |
| **Servo Motor**         |      PWM       |    GPIO 13    | Điều khiển góc quay (Quạt/Cửa).                              |
|                         |       V+       |      5V       | Nguồn động lực cho Servo.                                    |
|                         |      GND       |      GND      | Nối đất.                                                     |
| **Nút nhấn (Button)**   |     Pin 1      |    GPIO 4     | Chuyển đổi chế độ (AUTO/MAN).                                |
|                         |     Pin 2      |      GND      | Sử dụng `INPUT_PULLUP` trong code.                           |
| **Đèn LED (Green/Red)** |   Anode (+)    |  GPIO 2 & 14  | Qua điện trở 220Ω để hạn dòng.                               |
|                         |  Cathode (-)   |      GND      | Nối đất.                                                     |

---

## 6. Giải thích chi tiết từng khối code

### a) Khai báo Thư viện và Định nghĩa phần cứng

Các thư viện cung cấp hàm điều khiển linh kiện. Macro `#define` giúp quản lý chân kết nối dễ dàng.

```cpp
#include <DHT.h>              // Thư viện cảm biến nhiệt độ & độ ẩm
#include <Adafruit_SSD1306.h> // Thư viện màn hình OLED
#include <ESP32Servo.h>       // Thư viện điều khiển Servo trên ESP32

#define BUTTON_PIN 4          // Nút nhấn tại chân số 4
#define NGUONG_NHIET 30.0     // Ngưỡng nhiệt báo cháy
```

**\*Giải thích kỹ thuật:**

- **Adafruit_SSD1306.h:** Hỗ trợ vẽ hình, viết chữ lên màn hình OLED thông qua giao tiếp I2C.
- **NGUONG_NHIET:** Giá trị hằng số dùng để so sánh với dữ liệu cảm biến, giúp hệ thống tự động ra quyết định.

### b) Biến toàn cục và Đối tượng điều khiển

Nơi lưu trữ trạng thái của hệ thống trong suốt quá trình chạy.

```cpp
int cheDo = 0;  // 0: Tự động, 1: Thủ công
unsigned long thoiGianTruoc = 0;
const long chuKyCapNhat = 2000; // Cập nhật dữ liệu mỗi 2 giây
```

**\*Giải thích kỹ thuật:**

- **cheDo:** Biến quan trọng điều hướng logic. Ở chế độ `AUTO`, hệ thống tự mở cửa khi có biến cố.
- **chuKyCapNhat:** Sử dụng thay cho `delay()` để tránh làm treo hệ thống, giúp ESP32 xử lý đa nhiệm (vừa đọc cảm biến, vừa nhận lệnh Serial).

### c) Hàm xử lý chính: docVaBaoCaoCamBien()

Đây là "bộ não" của hệ thống, thực hiện thu thập và phân tích dữ liệu.

```cpp
void docVaBaoCaoCamBien() {
    float t = dht.readTemperature(); // Đọc nhiệt độ
    int d = duration * 0.034 / 2;    // Tính khoảng cách (cm)

    if (t > NGUONG_NHIET || d < NGUONG_KHOANG_CACH) {
        // Kích hoạt cảnh báo LED và Servo
    }
}
```

---

## 7. Phân tích lỗi thường gặp và cách khắc phục

| Hiện tượng                             | Nguyên nhân có thể                                            | Cách kiểm tra và khắc phục                                                                        |
| :------------------------------------- | :------------------------------------------------------------ | :------------------------------------------------------------------------------------------------ |
| **Màn hình OLED không sáng**           | 1. Sai địa chỉ I2C.<br>2. Lỏng dây VCC/GND.                   | 1. Kiểm tra `if(!oled.begin(...))`. Địa chỉ thường là 0x3C.<br>2. Đảm bảo nguồn 3.3V ổn định.     |
| **Nhiệt độ hiện "nan" (Not a Number)** | 1. Lỏng chân DATA của DHT22.<br>2. Chưa có trở kéo lên (10k). | 1. Kiểm tra dây nối GPIO 15.<br>2. Đảm bảo thư viện DHT được cài đúng version.                    |
| **Servo rung nhưng không quay**        | 1. Thiếu nguồn (Sụt áp).<br>2. Sai chân PWM.                  | 1. Cấp nguồn 5V riêng cho Servo (chung GND với ESP32).<br>2. Kiểm tra `hieuChinhCua.attach(13)`.  |
| **Gõ lệnh Serial không phản hồi**      | 1. Sai Baudrate.<br>2. Buffer bị đầy.                         | 1. Chỉnh Serial Monitor về **115200 baud**.<br>2. Thêm hàm xả buffer `while(Serial.available())`. |

---

**Giảng viên soạn thảo:** [Tên của bạn/Antigravity]
**Ngày cập nhật:** 03/03/2026
