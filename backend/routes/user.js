import express from "express";
import { pool } from "../db.js";

const router = express.Router();

// ✅ Lấy danh sách người dùng (Admin-only)
router.get("/", async (_, res) => {
  try {
    const [rows] = await pool.query("SELECT id, username, role, permission_light, permission_fan FROM users");
    res.json(rows);
  } catch (err) {
    console.error("[User Error]", err);
    res.status(500).json({ ok: false, msg: "Lỗi máy chủ" });
  }
});

export default router;
