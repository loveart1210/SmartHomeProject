import express from "express";
import bodyParser from "body-parser";
import { pool } from "./db.js";
import cors from "cors";
import dotenv from "dotenv";
import authRoutes from "./routes/auth.js";
import rfidRoutes from "./routes/rfid.js";
import userRoutes from "./routes/user.js";

dotenv.config();
const app = express();

app.use(cors());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

app.get("/", (_, res) => res.json({ ok: true, service: "SmartHome Backend" }));

// Routes
app.use("/api/auth", authRoutes);
app.use("/api/rfid", rfidRoutes);
app.use("/api/user", userRoutes);

// ================= RFID CHECK =================
app.get("/api/rfid/:uid", async (req, res) => {
  const { uid } = req.params;
  try {
    const [rows] = await pool.query(
      "SELECT * FROM rfid_codes WHERE uid_code = ? AND valid = 1",
      [uid]
    );
    if (rows.length > 0) {
      res.json({ authorized: true, owner: rows[0].owner });
    } else {
      res.json({ authorized: false });
    }
  } catch (err) {
    console.error("RFID check error:", err);
    res.status(500).json({ authorized: false, error: "Server error" });
  }
});


const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`✅ Backend running on port ${PORT}`);
});
