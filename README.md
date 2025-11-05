# SmartHomeProject

Kiến trúc đầy đủ: ESP32 (thiết bị) + Backend Node.js (đăng nhập, quản lý user, RFID) + MySQL + Frontend (login/dashboard).

## Cách chạy

### 1) Database
- Cài MySQL, chạy `database/init_db.sql`
- Mặc định có user: `admin / 1234` (role admin)

### 2) Backend
```bash
cd backend
cp .env.example .env   # sửa DB_USER/DB_PASS nếu cần
npm install
npm run start
```

### 3) Frontend
- Mở `frontend/login.html` bằng VSCode Live Server hoặc bất kỳ web server tĩnh nào.
- Đăng nhập `admin / 1234` → chuyển sang `dashboard.html`.

### 4) ESP32
- Mở `esp32/smart_home_esp32_code.ino` trong Arduino IDE
- Sửa `esp32/credentials.h` (WIFI_SSID, WIFI_PASS, BACKEND_URL)
- Nạp code và đảm bảo ESP32 cùng mạng với Backend.

## Ghi chú
- Đây là bản khung đơn giản để chạy nhanh. Bạn có thể bổ sung:
  - Bcrypt/JWT cho bảo mật
  - Phân quyền chặt chẽ trên Backend
  - Ghi log lịch sử thao tác
