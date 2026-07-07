// Servidor web local: serve uma página ao vivo em "/" e os dados em JSON em
// "/data". A página tem um JS que busca /data a cada 2s e atualiza os números
// sem recarregar. Nada sai pra internet — é só na rede local.

#pragma once

extern "C"
{
#include "esp_http_server.h"
#include "esp_log.h"
}

#include <atomic>
#include <cstdio>

namespace web
{

// Última leitura, compartilhada entre o loop do sensor e o servidor.
// atomic<float> pra ler/escrever sem trava entre as tarefas.
struct Reading
{
  std::atomic<float> temperature{0.0f};
  std::atomic<float> humidity{0.0f};
  std::atomic<uint32_t> uptime_s{0};
  std::atomic<bool> valid{false};
};

inline Reading g_reading;

// Página HTML (com CSS e JS embutidos). Mantida como raw string.
inline const char *INDEX_HTML = R"HTML(<!doctype html>
<html lang="pt-br">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>DHT22 — Temperatura e Umidade</title>
<style>
  :root { color-scheme: light dark; }
  body { font-family: system-ui, sans-serif; margin: 0; padding: 2rem;
         display: flex; flex-direction: column; align-items: center;
         gap: 1.5rem; background: #0f1115; color: #e8eaed; }
  h1 { font-size: 1.2rem; font-weight: 600; opacity: .8; margin: 0; }
  .cards { display: flex; gap: 1.5rem; flex-wrap: wrap; justify-content: center; }
  .card { background: #1b1e26; border-radius: 16px; padding: 1.5rem 2rem;
          min-width: 160px; text-align: center;
          box-shadow: 0 2px 12px rgba(0,0,0,.3); }
  .label { font-size: .85rem; opacity: .6; text-transform: uppercase;
           letter-spacing: .05em; }
  .value { font-size: 3rem; font-weight: 700; margin-top: .3rem; }
  .value.temp { color: #ff7a59; }
  .value.umid { color: #4aa3ff; }
  .unit { font-size: 1.2rem; opacity: .6; }
  .meta { font-size: .8rem; opacity: .5; }
  .stale { opacity: .35; }
</style>
</head>
<body>
  <h1>Sensor DHT22</h1>
  <div class="cards">
    <div class="card">
      <div class="label">Temperatura</div>
      <div class="value temp"><span id="t">--</span><span class="unit">&deg;C</span></div>
    </div>
    <div class="card">
      <div class="label">Umidade</div>
      <div class="value umid"><span id="h">--</span><span class="unit">%</span></div>
    </div>
  </div>
  <div class="meta">atualizado h&aacute; <span id="ago">--</span>s &middot; uptime <span id="up">--</span>s</div>
<script>
let lastOk = 0;
async function tick() {
  try {
    const r = await fetch('/data', {cache: 'no-store'});
    const d = await r.json();
    document.getElementById('t').textContent = d.valid ? d.temperature.toFixed(1) : '--';
    document.getElementById('h').textContent = d.valid ? d.humidity.toFixed(1) : '--';
    document.getElementById('up').textContent = d.uptime_s;
    lastOk = Date.now();
    document.body.classList.remove('stale');
  } catch (e) {
    document.body.classList.add('stale');
  }
  const ago = lastOk ? Math.round((Date.now() - lastOk) / 1000) : '--';
  document.getElementById('ago').textContent = ago;
}
tick();
setInterval(tick, 2000);
</script>
</body>
</html>)HTML";

inline esp_err_t index_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_sendstr(req, INDEX_HTML);
}

inline esp_err_t data_handler(httpd_req_t *req)
{
  char json[128];
  snprintf(json, sizeof(json),
           "{\"temperature\":%.1f,\"humidity\":%.1f,\"uptime_s\":%lu,\"valid\":%s}",
           g_reading.temperature.load(), g_reading.humidity.load(),
           (unsigned long)g_reading.uptime_s.load(),
           g_reading.valid.load() ? "true" : "false");
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_sendstr(req, json);
}

inline httpd_handle_t start()
{
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = 8080; // porta alternativa; acesse http://<IP>:8080
  httpd_handle_t server = nullptr;
  if (httpd_start(&server, &cfg) != ESP_OK)
  {
    ESP_LOGE("web", "falha ao iniciar o servidor HTTP");
    return nullptr;
  }

  httpd_uri_t index_uri = {"/", HTTP_GET, index_handler, nullptr};
  httpd_uri_t data_uri = {"/data", HTTP_GET, data_handler, nullptr};
  httpd_register_uri_handler(server, &index_uri);
  httpd_register_uri_handler(server, &data_uri);
  ESP_LOGI("web", "servidor no ar — abra http://<IP-do-esp>:%d no navegador",
           cfg.server_port);
  return server;
}

} // namespace web
