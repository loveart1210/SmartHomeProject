import express from "express";
import { pool } from "../db.js";

const router = express.Router();

// ✅ Đăng nhập & trả quyền
router.post("/login", async (req, res) => {
  const { username, password } = req.body;
  if (!username || !password)
    return res.status(400).json({ ok: false, msg: "Thiếu thông tin" });

  try {
    const [rows] = await pool.query(
      "SELECT * FROM users WHERE username=? AND password=? LIMIT 1",
      [username, password]
    );

    if (rows.length === 0)
      return res.status(401).json({ ok: false, msg: "Sai tài khoản hoặc mật khẩu" });

    const u = rows[0];
    return res.json({
      ok: true,
      id: u.id,
      username: u.username,
      role: u.role,
      permission_light: u.permission_light,
      permission_fan: u.permission_fan,
    });
  } catch (err) {
    console.error("[Auth Error]", err);
    return res.status(500).json({ ok: false, msg: "Lỗi máy chủ" });
  }
});

// Admin tạo tài khoản
router.post("/register", async (req, res) => {
  try {
    const { username, password, role } = req.body;
    if (!username || !password || !role) {
      return res.status(400).json({ ok: false, message: "Thiếu dữ liệu" });
    }
    if (!["admin", "user"].includes(role)) {
      return res.status(400).json({ ok: false, message: "Role không hợp lệ" });
    }
    await db.execute(
      "INSERT INTO users (username, password, role) VALUES (?,?,?)",
      [username, password, role]
    );
    res.json({ ok: true });
  } catch (err) {
    console.error(err);
    res.status(500).json({ ok: false, message: "Server error" });
  }
});

export default router;
