#include <Adafruit_GFX.h>     // Thư viện đồ họa cơ bản cho màn hình
#include <Adafruit_SSD1306.h> // Thư viện điều khiển màn hình OLED SSD1306
#include <DHT.h>              // Thư viện cảm biến nhiệt độ và độ ẩm DHT
#include <ESP32Servo.h>       // Thư viện điều khiển động cơ Servo cho ESP32
#include <arduino.h>

/**
 * ĐỊNH NGHĨA CÁC CHÂN KẾT NỐI (Hardware Pin Assignment)
 * Việc định nghĩa rõ ràng các chân giúp dễ dàng thay đổi phần cứng mà không cần
 * sửa code ở nhiều nơi.
 */
#define BUTTON_PIN 4   // Nút nhấn chuyển chế độ (Số 4 - GPIO4)
#define LDR_PIN 34     // Cảm biến quang trở (Analog 34 - GPIO34)
#define DHT_PIN 15     // Cảm biến DHT22 (Số 15 - GPIO15)
#define DHT_TYPE DHT22 // Loại cảm biến là DHT22 (độ chính xác cao hơn DHT11)
#define TRIG_PIN 5     // Chân phát tín hiệu siêu âm (Trigger)
#define ECHO_PIN 18    // Chân nhận tín hiệu siêu âm (Echo)
#define SERVO_PIN 13   // Chân điều khiển Servo (Cửa/Quạt)
#define LED_GREEN 2    // Đèn LED xanh - Trạng thái an toàn
#define LED_RED 14     // Đèn LED đỏ - Trạng thái cảnh báo

/**
 * CÁC NGƯỠNG THIẾT LẬP (System Thresholds)
 * Các giá trị này dùng để so sánh với dữ liệu cảm biến để ra quyết định.
 */
#define NGUONG_NHIET 30.0 // Nhiệt độ trên 30.0°C sẽ cảnh báo cháy/nóng
#define NGUONG_ANH_SANG                                                        \
  1500 // Giá trị quang trở (giả lập) để bật cảnh báo chiếu sáng
#define NGUONG_KHOANG_CACH                                                     \
  20 // Khoảng cách vật cản dưới 20cm coi như có đột nhập

// Các thông số vật lý của màn hình OLED
#define SCREEN_WIDTH 128 // Độ rộng màn hình (pixels)
#define SCREEN_HEIGHT 64 // Độ cao màn hình (pixels)

/**
 * KHỞI TẠO CÁC ĐỐI TƯỢNG (Object Initialization)
 * Tạo ra các "thực thể" phần mềm đại diện cho các thiết bị phần cứng.
 */
DHT dht(DHT_PIN, DHT_TYPE); // Đối tượng cảm biến DHT
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,
                      -1); // Đối tượng màn hình OLED
Servo hieuChinhCua;        // Đối tượng Servo điều khiển thực thi

/**
 * KHAI BÁO NGUYÊN MẪU HÀM (Function Prototypes)
 * Giúp trình biên dịch biết trước sự tồn tại của các hàm sẽ định nghĩa ở phía
 * dưới.
 */
void kickHoatSieuAm();
void capNhatOLED(String dong1, String dong2, String dong3, String dong4);
void xuLyLenhSerial();
void hienThiTroGiup();
void docVaBaoCaoCamBien();

/**
 * BIẾN TRẠNG THÁI TOÀN CỤC (Global State Variables)
 * Lưu trữ các thông số vận hành của hệ thống trong suốt thời gian chạy.
 */
int cheDo = 0;    // Chế độ vận hành: 0 = Tự động (Auto), 1 = Thủ công (Manual)
int trangThaiNut; // Trạng thái hiện tại của nút bấm
int trangThaiNutCu =
    HIGH; // Trạng thái trước đó của nút bấm (để phát hiện sự thay đổi)
unsigned long thoiGianDebounce =
    0;                           // Thời điểm bắt đầu rung phím (để lọc nhiễu)
unsigned long thoiGianTruoc = 0; // Thời điểm cập nhật cảm biến lần cuối
const long chuKyCapNhat = 2000;  // Chu kỳ đọc cảm biến mỗi 2 giây (2000ms)

/**
 * HÀM THIẾT LẬP (Setup Function)
 * Chạy duy nhất một lần khi khởi động hoặc Reset ESP32.
 */
void setup() {
  Serial.begin(
      115200); // Khởi tạo giao tiếp Serial với máy tính (baudrate 115200)
  delay(100);

  // In banner chào mừng ra Serial Monitor
  Serial.println("\n**************************************");
  Serial.println("*  HE THONG KHO THONG MINH (v2.0)    *");
  Serial.println("*       XIN CHAO QUY KHACH!          *");
  Serial.println("**************************************");
  Serial.println("Dang khoi dong cac cam bien...");

  // Thiết lập chế độ cho các chân (Vào/Ra)
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Chế độ INPUT_PULLUP để không cần điện
                                     // trở ngoài cho nút bấm
  pinMode(LDR_PIN, INPUT);           // Cảm biến quang trở là đầu vào Analog
  pinMode(TRIG_PIN, OUTPUT);         // Chân phát siêu âm là đầu ra
  pinMode(ECHO_PIN, INPUT);          // Chân nhận siêu âm là đầu vào
  pinMode(LED_GREEN, OUTPUT);        // Đèn LED là đầu ra
  pinMode(LED_RED, OUTPUT);

  dht.begin(); // Bắt đầu hoạt động cảm biến DHT

  // Khởi tạo màn hình OLED, kiểm tra xem màn hình có phản hồi không
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("LOI: Khong tim thay man hinh OLED SSD1306!");
    for (;;)
      ; // Dừng chương trình nếu không có màn hình
  }

  // Thiết lập font chữ và màu sắc mặc định cho OLED
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  hieuChinhCua.attach(SERVO_PIN); // Gán chân điều khiển cho Servo
  hieuChinhCua.write(0);          // Đặt Servo về vị trí 0 độ (Cửa đóng)

  // Hiệu ứng kiểm tra đèn LED khi khởi động (Nháy 3 lần)
  Serial.println("Kiem tra den tin hieu LED...");
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    delay(300);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
    delay(300);
  }
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN,
               HIGH); // Kết thúc kiểm tra, để đèn xanh sáng (Sẵn sàng)

  Serial.println("HE THONG DA SAN SANG!");
  hienThiTroGiup(); // Hiển thị các lệnh điều khiển qua Serial

  // Hiển thị thông tin ban đầu lên OLED
  capNhatOLED("XIN CHAO!", "KHO THONG MINH", "Trang thai: OK",
              "Go 'h' de xem lenh");
}

/**
 * VÒNG LẶP CHÍNH (Main Loop)
 * Chạy liên tục để xử lý các sự kiện và cập nhật dữ liệu.
 */
void loop() {
  xuLyLenhSerial(); // Luôn lắng nghe lệnh từ máy tính

  /**
   * XỬ LÝ NÚT BẤM VỚI KỸ THUẬT DEBOUNCE (Chống nhiễu phím)
   * Nút bấm cơ học khi nhấn sẽ tạo ra nhiều xung ảo, ta cần lọc bỏ chúng.
   */
  int docNut = digitalRead(BUTTON_PIN);
  if (docNut != trangThaiNutCu) {
    thoiGianDebounce = millis(); // Ghi nhận thời điểm trạng thái phím thay đổi
  }

  // Nếu trạng thái phím ổn định đủ lâu (50ms) thì mới ghi nhận là một lần nhấn
  // thực sự
  if ((millis() - thoiGianDebounce) > 50) {
    if (docNut != trangThaiNut) {
      trangThaiNut = docNut;
      // Phát hiện trạng thái LOW (vì dùng PULLUP nên khi nhấn phím chân sẽ
      // xuống mức LOW)
      if (trangThaiNut == LOW) {
        cheDo = !cheDo; // Đảo trạng thái chế độ (0 sang 1 hoặc ngược lại)
        String tenCheDo = cheDo ? "THU CONG" : "TU DONG";
        Serial.println("\n[THONG BAO] Chuyen sang che do: " + tenCheDo);
        capNhatOLED("DOI CHE DO", "Sang:", tenCheDo, "---");
      }
    }
  }
  trangThaiNutCu = docNut; // Lưu lại để so sánh ở vòng lặp kế tiếp

  /**
   * CẬP NHẬT DỮ LIỆU CẢM BIẾN ĐỊNH KỲ
   * Sử dụng millis() để không làm dừng chương trình (thay vì delay()).
   */
  unsigned long thoiGianHienTai = millis();
  if (thoiGianHienTai - thoiGianTruoc >= chuKyCapNhat) {
    thoiGianTruoc = thoiGianHienTai;
    docVaBaoCaoCamBien(); // Gọi hàm đọc và phân tích dữ liệu
  }
}

/**
 * Kích hoạt phát sóng siêu âm để đo khoảng cách
 */
void kickHoatSieuAm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); // Phát xung 10 micro giây
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
}

/**
 * Hiển thị 4 dòng văn bản lên màn hình OLED
 */
void capNhatOLED(String dong1, String dong2, String dong3, String dong4) {
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println(dong1);
  oled.setCursor(0, 15);
  oled.println(dong2);
  oled.setCursor(0, 30);
  oled.println(dong3);
  oled.setCursor(0, 45);
  oled.println(dong4);
  oled.display(); // Cần gọi display() để dữ liệu thực sự xuất hiện trên màn
                  // hình
}

/**
 * Xử lý các ký tự lệnh nhận được từ Serial Monitor
 */
void xuLyLenhSerial() {
  if (Serial.available() > 0) {
    char lenh = Serial.read(); // Đọc ký tự đầu tiên
    while (Serial.available() > 0)
      Serial.read(); // Xóa sạch bộ đệm các ký tự thừa (vd: xuống dòng)

    switch (lenh) {
    case 'm':
    case 'M': // Đổi chế độ điều khiển
      cheDo = !cheDo;
      Serial.println("\n>> LENH: DOI CHE DO. Hien tai: " +
                     String(cheDo ? "THU CONG" : "TU DONG"));
      break;
    case 's':
    case 'S': // Yêu cầu báo cáo cảm biến ngay lập tức
      Serial.println("\n>> LENH: YEU CAU BAO CAO TRUC TIEP");
      docVaBaoCaoCamBien();
      break;
    case 'r':
    case 'R': // Reset hệ thống về trạng thái an toàn
      Serial.println("\n>> LENH: THIET LAP LAI (RESET)");
      hieuChinhCua.write(0);
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, HIGH);
      capNhatOLED("RESET HE THONG", "Da thiet lap lai", "", "---");
      break;
    case 'h':
    case 'H': // Hiển thị bảng trợ giúp
      hienThiTroGiup();
      break;
    default:
      // Thông báo nếu người dùng nhập sai lệnh
      if (lenh != '\n' && lenh != '\r') {
        Serial.print("\n>> Khong ro lenh: ");
        Serial.println(lenh);
        hienThiTroGiup();
      }
      break;
    }
  }
}

/**
 * In danh sách các lệnh điều khiển ra Serial Monitor
 */
void hienThiTroGiup() {
  Serial.println("\n--- BANG DIEU KHIEN KHO ---");
  Serial.println(" [m] - Doi che do (Tu dong/Thu cong)");
  Serial.println(" [s] - Xem bao cao cam bien ngay");
  Serial.println(" [r] - Reset canh bao & Servo");
  Serial.println(" [h] - Hien thi tro giup");
  Serial.println("---------------------------");
}

/**
 * Hàm quan trọng nhất: Đọc toàn bộ cảm biến và ra quyết định điều khiển
 */
void docVaBaoCaoCamBien() {
  // 1. Đọc dữ liệu từ cảm biến nhiệt độ & độ ẩm
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // 2. Đọc độ sáng từ quang trở (LDR)
  int l = analogRead(LDR_PIN);

  // 3. Đo khoảng cách bằng cảm biến siêu âm
  kickHoatSieuAm();
  long duration =
      pulseIn(ECHO_PIN, HIGH); // Đo thời gian xung phản hồi (echo) quay lại
  int d = duration * 0.034 /
          2; // Công thức vật lý: Distance = (Time * Tốc độ âm thanh) / 2

  // Kiểm tra tính hợp lệ của dữ liệu DHT (phòng trường hợp đứt dây)
  bool loiCBIEN = isnan(t) || isnan(h);
  bool coHanhDong = false; // Biến đánh dấu xem có cần kích hoạt Servo không
  String tieuDeOLED = "QUAN LY KHO";
  String canhBao = "";

  // 4. KIỂM TRA CÁC NGƯỠNG AN TOÀN (LOGIC RA QUYẾT ĐỊNH)
  if (t > NGUONG_NHIET) {
    canhBao += "NGUY HIEM CHAY! ";
    coHanhDong = true;
  }
  if (d > 0 && d < NGUONG_KHOANG_CACH) {
    canhBao += "CO DOT NHAP! ";
    coHanhDong = true;
  }

  // 5. IN DỮ LIỆU RA TRẠM GIÁM SÁT (SERIAL MONITOR)
  Serial.println("\n--- BAO CAO MONITOR: " + String(millis() / 1000) + "s ---");
  if (loiCBIEN)
    Serial.println("[!] LOI: Cam bien DHT khong phan hoi.");
  Serial.print(" Nhiet do: ");
  Serial.print(t);
  Serial.print("C | Do am: ");
  Serial.print(h);
  Serial.println("%");
  Serial.print(" Anh sang: ");
  Serial.print(l);
  Serial.print(" | K.Cach: ");
  Serial.print(d);
  Serial.println("cm");

  // Xử lý báo động trên đèn LED
  if (canhBao != "") {
    Serial.println("[CANH BAO] " + canhBao);
    tieuDeOLED = "!!! CANH BAO !!!";
    digitalWrite(LED_RED, HIGH);  // Bật đèn đỏ cảnh báo
    digitalWrite(LED_GREEN, LOW); // Tắt đèn xanh
  } else {
    Serial.println("[OK] Tinh trang kho: AN TOAN");
    digitalWrite(LED_RED, LOW);    // Tắt đèn đỏ
    digitalWrite(LED_GREEN, HIGH); // Bật đèn xanh an toàn
  }

  // 6. ĐIỀU KHIỂN CƠ CẤU CHẤP HÀNH (SERVO)
  int pos = 0;
  // Nếu đang ở chế độ TỰ ĐỘNG và có cảnh báo (cháy/đột nhập) -> Mở cửa thoát
  // hiểm / Bật quạt
  if (cheDo == 0 && coHanhDong) {
    pos = 90; // Góc quay 90 độ (Trạng thái Mở)
    Serial.println("[HANH DONG] Tu dong mo thiet bi thoat hiem.");
  }
  hieuChinhCua.write(pos); // Xuất lệnh ra chân Servo

  // 7. TỔNG HỢP DỮ LIỆU LÊN MÀN HÌNH OLED
  String dong2 = "T:" + String(t, 1) + "C H:" + String(h, 1) + "%";
  String dong3 = "L:" + String(l) + " D:" + String(d) + "cm";
  String dong4 = "Mode:" + String(cheDo ? "MAN " : "AUTO") +
                 (pos > 0 ? " ACT:ON" : " ACT:-- ");

  capNhatOLED(tieuDeOLED, dong2, dong3, dong4);
}
