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

const server = Bun.serve({
  port: PORT,
  async fetch(req) {
    if (req.method === "POST") {
      const body = await req.json().catch(() => ({}));

      const entry = {
        timestamp: new Date().toISOString(),
        ...body,
      };

      const logs = loadLogs();
      logs.push(entry);
      saveLogs(logs);

      console.log(`[LOG] ${entry.timestamp}`, body);

      return new Response(JSON.stringify({ status: "ok" }), {
        headers: { "Content-Type": "application/json" },
      });
    }

    // GET — return all logs
    const logs = loadLogs();
    return new Response(JSON.stringify(logs, null, 2), {
      headers: { "Content-Type": "application/json" },
    });
  },
});

console.log(`ESP32 log server running on http://localhost:${PORT}`);
