#define CONFIG_LOG_DEFAULT_LEVEL 0

#include "HomeSpan.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============= CẤU HÌNH PHẦN CỨNG =============
#define I2C_SDA        5     // GPIO 5 - SDA OLED
#define I2C_SCL        6     // GPIO 6 - SCL OLED
#define FAN_PWM_PIN    7     // GPIO 7 - Chân xuất PWM điều khiển Quạt

// ============= CẤU HÌNH OLED 0.42" (72x40) =============
#define SCREEN_WIDTH  128    // Dùng dải RAM đầy đủ của SSD1306
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// Tọa độ Offset thực tế để đẩy chữ vào ĐÚNG GIỮA KÍNH
#define OFFSET_X      28     // Dịch ngang sang phải 28px
#define OFFSET_Y      30     // Dịch dọc xuống dưới 12px

#define PWM_FREQ       1000  // Tần số PWM 1000 Hz
#define PWM_RESOLUTION 8     // Độ phân giải 8-bit (0-255)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int currentFanSpeed = 50;
boolean fanActive = false;

void updateOLEDDisplay();

// ============= Fan Service Class =============
struct DimmableFan : Service::Fan {
  SpanCharacteristic *active;
  SpanCharacteristic *rotationSpeed;

  DimmableFan() : Service::Fan() {
    pinMode(FAN_PWM_PIN, OUTPUT);
    
    active = new Characteristic::Active(0);
    rotationSpeed = (new Characteristic::RotationSpeed(50))->setRange(0, 100, 25);

    ledcAttach(FAN_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(FAN_PWM_PIN, 0);
  }

  boolean update() override {
    fanActive = active->getNewVal();
    currentFanSpeed = rotationSpeed->getNewVal<int>();

    if (fanActive && currentFanSpeed > 0) {
      int pwmVal = map(currentFanSpeed, 1, 100, 1, 255);
      ledcAttach(FAN_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
      ledcWrite(FAN_PWM_PIN, pwmVal);
    } else {
      ledcWrite(FAN_PWM_PIN, 0);
    }

    updateOLEDDisplay();
    return true;
  }
};

// ============= Setup =============
void setup() {
  Serial.begin(115200);

  // Khởi tạo OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setRotation(0); // Nếu số bị ngược chiều bạn chỉ cần đổi thành (2)
    updateOLEDDisplay(); 
  }

  // Khởi tạo HomeSpan
  homeSpan.begin(Category::Fans, "HomeSpan Fan");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Manufacturer("HomeSpan");
      new Characteristic::Model("ESP32-C3 Fan");

    new DimmableFan();

  delay(500);
  updateOLEDDisplay();
}

// ============= Main Loop =============
void loop() {
  homeSpan.poll(); 
}

// ============= Căn chỉnh vị trí số =============
void updateOLEDDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3); // Kích thước chữ chuẩn rõ nét

  int displayValue = (fanActive) ? currentFanSpeed : 0;

  // Tính toán vị trí căn giữa dựa trên số chữ số + Offset phần cứng
  int x = OFFSET_X;
  int y = OFFSET_Y;

  if (displayValue < 10) {
    x += 28; // Căn giữa cho số 1 chữ số (0, 1..9)
  } else if (displayValue < 100) {
    x += 18; // Căn giữa cho số 2 chữ số (25, 50, 75)
  } else {
    x += 8;  // Căn giữa cho số 3 chữ số (100)
  }

  display.setCursor(x, y);
  display.print(displayValue);
  display.display();
}