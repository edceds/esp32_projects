import { readFileSync, writeFileSync, existsSync } from "fs";

const LOG_FILE = "./logs.json";
const PORT = 3100;

function loadLogs(): any[] {
  if (existsSync(LOG_FILE)) {
    return JSON.parse(readFileSync(LOG_FILE, "utf-8"));
  }
  return [];
}

function saveLogs(logs: any[]) {
  writeFileSync(LOG_FILE, JSON.stringify(logs, null, 2));
}

// Latest state reported by ESP32
let deviceStatus: Record<string, any> = {};
let lastUpdateTime: string | null = null;

const server = Bun.serve({
  port: PORT,
  async fetch(req) {
    const url = new URL(req.url);

    if (req.method === "POST") {
      const body = await req.json().catch(() => ({}));

      const entry = {
        timestamp: new Date().toISOString(),
        ...body,
      };

      // Save to logs
      const logs = loadLogs();
      logs.push(entry);
      saveLogs(logs);

      // Update current status
      lastUpdateTime = entry.timestamp;
      deviceStatus = { ...deviceStatus, ...body };

      console.log(`[LOG] ${entry.timestamp}`, body);

      return new Response(JSON.stringify({ status: "ok" }), {
        headers: { "Content-Type": "application/json" },
      });
    }

    if (url.pathname === "/status") {
      const status = {
        last_update: lastUpdateTime,
        ...deviceStatus,
      };

      return new Response(JSON.stringify(status, null, 2), {
        headers: { "Content-Type": "application/json" },
      });
    }

    return new Response("Not found", { status: 404 });
  },
});

console.log(`ESP32 status server running on http://localhost:${PORT}`);
