import express from "express";
import { pool } from "../db.js";

const router = express.Router();

// ✅ Kiểm tra RFID hợp lệ
router.get("/check/:uid", async (req, res) => {
  const { uid } = req.params;
  try {
    const [rows] = await pool.query(
      "SELECT * FROM rfid_codes WHERE uid_code=? AND valid=1",
      [uid]
    );

    if (rows.length === 0)
      return res.status(404).json({ ok: false, msg: "Thẻ không hợp lệ" });

    return res.json({
      ok: true,
      uid: rows[0].uid_code,
      owner: rows[0].owner,
    });
  } catch (err) {
    console.error("[RFID Error]", err);
    return res.status(500).json({ ok: false, msg: "Lỗi máy chủ" });
  }
});

export default router;
