DROP DATABASE IF EXISTS smarthome;
CREATE DATABASE IF NOT EXISTS smarthome;
USE smarthome;

-- ==============================
-- BẢNG NGƯỜI DÙNG (đăng nhập web)
-- ==============================
CREATE TABLE IF NOT EXISTS users (
  id INT AUTO_INCREMENT PRIMARY KEY,
  username VARCHAR(50) UNIQUE NOT NULL,
  password VARCHAR(255) NOT NULL,
  role ENUM('admin','user') NOT NULL,
  permission_light BOOLEAN DEFAULT 0,   -- có quyền bật/tắt đèn
  permission_fan BOOLEAN DEFAULT 0,     -- có quyền bật/tắt quạt
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- ==============================
-- BẢNG RFID (mã thẻ hợp lệ)
-- ==============================
CREATE TABLE IF NOT EXISTS rfid_codes (
  id INT AUTO_INCREMENT PRIMARY KEY,
  uid_code VARCHAR(20) UNIQUE NOT NULL, -- UID RFID
  owner VARCHAR(50),                    -- tên chủ sở hữu
  valid BOOLEAN DEFAULT 1,              -- 1 = hợp lệ
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 1. ADMIN: có toàn quyền
INSERT IGNORE INTO users (username, password, role, permission_light, permission_fan)
VALUES ('admin', '1234', 'admin', 1, 1);

-- 2. USER 1: chỉ được điều khiển ĐÈN
INSERT IGNORE INTO users (username, password, role, permission_light, permission_fan)
VALUES ('user1', '1234', 'user', 1, 0);

-- 3. USER 2: chỉ được điều khiển QUẠT
INSERT IGNORE INTO users (username, password, role, permission_light, permission_fan)
VALUES ('user2', '1234', 'user', 0, 1);

INSERT IGNORE INTO rfid_codes (uid_code, owner)
VALUES
  ('84D20F05', 'Admin'),
  ('A6C9F703', 'User 1'),
  ('2C0E1905', 'User 2'),
  ('13DF1005', 'Guest');
