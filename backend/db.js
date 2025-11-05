import mysql from "mysql2/promise";

export const pool = mysql.createPool({
  host: "localhost",
  user: "root",
  password: "135790",
  database: "smarthome",
  waitForConnections: true,
  connectionLimit: 10,
});
