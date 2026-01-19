import express from "express";
import cors from "cors";
import multer from "multer";
import bcrypt from "bcryptjs";
import Database from "better-sqlite3";
import crypto from "crypto";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const PORT = process.env.PORT || 3000;
const DB_PATH = path.join(__dirname, "local.db");
const UPLOAD_DIR = path.join(__dirname, "uploads");

fs.mkdirSync(UPLOAD_DIR, { recursive: true });

const db = new Database(DB_PATH);

db.pragma("journal_mode = WAL");

db.exec(`
  create table if not exists users (
    id integer primary key autoincrement,
    email text not null unique,
    password_hash text not null,
    created_at text not null
  );

  create table if not exists sessions (
    token text primary key,
    user_id integer not null,
    created_at text not null,
    foreign key (user_id) references users(id)
  );

  create table if not exists files (
    id integer primary key autoincrement,
    filename text not null,
    original_name text not null,
    mime_type text not null,
    size integer not null,
    created_at text not null,
    user_id integer not null,
    foreign key (user_id) references users(id)
  );
`);

const app = express();
app.use(cors({ origin: true, credentials: true }));
app.use(express.json({ limit: "10mb" }));
app.use("/uploads", express.static(UPLOAD_DIR, { maxAge: "1h" }));

const storage = multer.diskStorage({
  destination: (_req, _file, cb) => cb(null, UPLOAD_DIR),
  filename: (_req, file, cb) => {
    const safeName = file.originalname.toLowerCase().replace(/[^a-z0-9._-]+/g, "_");
    const token = crypto.randomUUID ? crypto.randomUUID() : `${Date.now()}`;
    cb(null, `${token}-${safeName}`);
  }
});

const upload = multer({
  storage,
  limits: {
    fileSize: 1024 * 1024 * 200
  }
});

function createToken() {
  return crypto.randomUUID ? crypto.randomUUID() : crypto.randomBytes(16).toString("hex");
}

function getAuthToken(req) {
  const header = req.headers.authorization || "";
  if (header.startsWith("Bearer ")) {
    return header.slice(7);
  }
  return "";
}

function requireAuth(req, res, next) {
  const token = getAuthToken(req);
  if (!token) {
    return res.status(401).json({ error: "unauthorized" });
  }

  const session = db
    .prepare("select token, user_id from sessions where token = ?")
    .get(token);

  if (!session) {
    return res.status(401).json({ error: "unauthorized" });
  }

  const user = db.prepare("select id, email from users where id = ?").get(session.user_id);
  if (!user) {
    return res.status(401).json({ error: "unauthorized" });
  }

  req.user = user;
  req.token = token;
  return next();
}

app.post("/api/auth/signup", (req, res) => {
  const email = String(req.body?.email || "").trim().toLowerCase();
  const password = String(req.body?.password || "");

  if (!email || !password) {
    return res.status(400).json({ error: "email \u0438 \u043f\u0430\u0440\u043e\u043b\u044c \u043e\u0431\u044f\u0437\u0430\u0442\u0435\u043b\u044c\u043d\u044b" });
  }

  const existing = db.prepare("select id from users where email = ?").get(email);
  if (existing) {
    return res.status(409).json({ error: "\u043f\u043e\u043b\u044c\u0437\u043e\u0432\u0430\u0442\u0435\u043b\u044c \u0443\u0436\u0435 \u0441\u0443\u0449\u0435\u0441\u0442\u0432\u0443\u0435\u0442" });
  }

  const passwordHash = bcrypt.hashSync(password, 10);
  const createdAt = new Date().toISOString();
  const info = db
    .prepare("insert into users (email, password_hash, created_at) values (?, ?, ?)")
    .run(email, passwordHash, createdAt);

  const token = createToken();
  db.prepare("insert into sessions (token, user_id, created_at) values (?, ?, ?)").run(token, info.lastInsertRowid, createdAt);

  return res.json({ token, user: { id: info.lastInsertRowid, email } });
});

app.post("/api/auth/signin", (req, res) => {
  const email = String(req.body?.email || "").trim().toLowerCase();
  const password = String(req.body?.password || "");

  if (!email || !password) {
    return res.status(400).json({ error: "email \u0438 \u043f\u0430\u0440\u043e\u043b\u044c \u043e\u0431\u044f\u0437\u0430\u0442\u0435\u043b\u044c\u043d\u044b" });
  }

  const user = db.prepare("select id, email, password_hash from users where email = ?").get(email);
  if (!user) {
    return res.status(401).json({ error: "\u043d\u0435\u0432\u0435\u0440\u043d\u044b\u0435 \u0434\u0430\u043d\u043d\u044b\u0435" });
  }

  const ok = bcrypt.compareSync(password, user.password_hash);
  if (!ok) {
    return res.status(401).json({ error: "\u043d\u0435\u0432\u0435\u0440\u043d\u044b\u0435 \u0434\u0430\u043d\u043d\u044b\u0435" });
  }

  const token = createToken();
  db.prepare("insert into sessions (token, user_id, created_at) values (?, ?, ?)").run(token, user.id, new Date().toISOString());

  return res.json({ token, user: { id: user.id, email: user.email } });
});

app.post("/api/auth/signout", requireAuth, (req, res) => {
  db.prepare("delete from sessions where token = ?").run(req.token);
  return res.json({ ok: true });
});

app.get("/api/auth/me", requireAuth, (req, res) => {
  return res.json({ user: req.user });
});

app.post("/api/upload", requireAuth, upload.single("file"), (req, res) => {
  if (!req.file) {
    return res.status(400).json({ error: "\u0444\u0430\u0439\u043b \u043d\u0435 \u043f\u043e\u043b\u0443\u0447\u0435\u043d" });
  }

  const createdAt = new Date().toISOString();
  const stmt = db.prepare(
    "insert into files (filename, original_name, mime_type, size, created_at, user_id) values (?, ?, ?, ?, ?, ?)"
  );
  const info = stmt.run(
    req.file.filename,
    req.file.originalname,
    req.file.mimetype,
    req.file.size,
    createdAt,
    req.user.id
  );

  const baseUrl = `${req.protocol}://${req.get("host")}`;
  const url = `${baseUrl}/uploads/${req.file.filename}`;

  return res.json({
    file: {
      id: info.lastInsertRowid,
      name: req.file.filename,
      original_name: req.file.originalname,
      mime_type: req.file.mimetype,
      size: req.file.size,
      created_at: createdAt,
      url
    }
  });
});

app.get("/api/files", requireAuth, (req, res) => {
  const rows = db
    .prepare("select filename, original_name, mime_type, size, created_at from files order by created_at desc limit 200")
    .all();

  const baseUrl = `${req.protocol}://${req.get("host")}`;
  const files = rows.map((row) => ({
    name: row.filename,
    original_name: row.original_name,
    mime_type: row.mime_type,
    size: row.size,
    created_at: row.created_at,
    url: `${baseUrl}/uploads/${row.filename}`
  }));

  return res.json({ files });
});

app.get("/api/health", (_req, res) => {
  res.json({ ok: true });
});

app.listen(PORT, () => {
  console.log(`local server on http://127.0.0.1:${PORT}`);
});
