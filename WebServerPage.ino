// ==========================
// Web server
// ==========================

void setupWebServer() {
  server.on("/", []() {
    float minTemp = getHistoryMin(tempHistory);
    float maxTemp = getHistoryMax(tempHistory);
    float minHum  = getHistoryMin(humHistory);
    float maxHum  = getHistoryMax(humHistory);

    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>CASE &mdash; Environment Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="color-scheme" content="dark">
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg:      #0c0d0f;
      --panel:   #131417;
      --panel-2: #17191d;
      --grid:    #1b1d22;
      --rule:    #24272e;
      --rule-hi: #363a44;
      --ink:     #e8eaed;
      --muted:   #7d828c;
      --red:     #e2504a;   /* signal / channel A - temperature */
      --red-dim: #6b3b34;
      --amber:   #3fa7c4;   /* channel B - humidity (cool cyan) */
      --steel:   #8f9498;   /* channel C - dew point */
      --danger:  #ff5648;
      --warn:    #c8a83a;
      --mono: "JetBrains Mono", ui-monospace, "SF Mono", "Roboto Mono", Menlo, Consolas, monospace;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      font-family: var(--mono);
      color: var(--ink);
      background-color: var(--bg);
      background-image:
        linear-gradient(var(--grid) 1px, transparent 1px),
        linear-gradient(90deg, var(--grid) 1px, transparent 1px);
      background-size: 22px 22px;
      padding: 24px 20px 48px;
      font-variant-numeric: tabular-nums;
      -webkit-font-smoothing: antialiased;
    }

    .container { max-width: 1600px; margin: 0 auto; }

    /* ---- faceplate header ---- */
    .plate {
      border: 1px solid var(--rule-hi);
      background: var(--panel);
      padding: 15px 18px 0;
    }
    .plate-top {
      display: flex; align-items: baseline; justify-content: space-between;
      gap: 14px; flex-wrap: wrap;
    }
    .brand { display: flex; align-items: baseline; gap: 12px; }
    .title {
      font-size: 1.1rem; font-weight: 700; letter-spacing: 0.22em; text-transform: uppercase;
    }
    .title .x { color: var(--red); }
    .expand { color: var(--muted); font-weight: 400; letter-spacing: 0.1em; font-size: 0.74rem; }
    .rec { display: inline-flex; align-items: center; gap: 7px; font-size: 0.68rem; letter-spacing: 0.18em; text-transform: uppercase; color: var(--muted); }
    .rec .led { width: 8px; height: 8px; background: var(--red); border-radius: 50%; animation: pulse 2.4s ease-in-out infinite; }
    @keyframes pulse { 0%,100% { opacity: 1; } 50% { opacity: 0.28; } }
    @media (prefers-reduced-motion: reduce) { .rec .led { animation: none; } }
    .rev { font-size: 0.66rem; letter-spacing: 0.14em; color: var(--muted); text-transform: uppercase; }
    .plate-bar { height: 3px; margin-top: 12px; background: var(--red); }

    /* ---- metadata strip ---- */
    .meta { display: flex; flex-wrap: wrap; border: 1px solid var(--rule); border-top: none; background: var(--panel); }
    .meta-cell {
      flex: 1 1 160px; padding: 8px 12px; border-right: 1px solid var(--rule);
      font-size: 0.73rem; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;
    }
    .meta-cell:last-child { border-right: none; }
    .meta-cell .k { color: var(--red-dim); text-transform: uppercase; letter-spacing: 0.14em; margin-right: 7px; }
    .meta-cell span:last-child { color: var(--ink); }

    /* ---- status strip ---- */
    .status { display: flex; align-items: center; gap: 12px; border: 1px solid var(--rule); border-top: none; background: var(--panel); padding: 11px 14px; }
    .dot { width: 9px; height: 9px; background: var(--red-dim); flex-shrink: 0; }
    .status[data-state="ok"]     .dot { background: var(--red); }
    .status[data-state="warn"]   .dot { background: var(--warn); }
    .status[data-state="danger"] .dot { background: var(--danger); }
    .status[data-state="warn"]   .status-title { color: var(--warn); }
    .status[data-state="danger"] .status-title { color: var(--danger); }
    .status-title { font-size: 0.79rem; font-weight: 700; letter-spacing: 0.09em; text-transform: uppercase; }
    .status-sub { font-size: 0.75rem; color: var(--muted); }

    /* ---- sections ---- */
    .section { margin-top: 28px; }
    .section-title {
      font-size: 0.7rem; letter-spacing: 0.22em; text-transform: uppercase; color: var(--red);
      padding-bottom: 6px; margin-bottom: 14px; border-bottom: 1px solid var(--rule-hi);
      display: flex; justify-content: space-between; align-items: baseline;
    }
    .section-title .hint { letter-spacing: 0.06em; color: var(--muted); }

    /* ---- hero: primary + secondary channels ---- */
    .hero { display: grid; grid-template-columns: 1fr; gap: 14px; }
    @media (min-width: 720px) { .hero { grid-template-columns: 1.9fr 1fr; } }

    .panel {
      background: var(--panel); border: 1px solid var(--rule); padding: 18px;
      position: relative;
    }
    .panel .ch { position: absolute; top: 0; right: 0; font-size: 0.62rem; letter-spacing: 0.16em; color: var(--muted); padding: 5px 8px; border-left: 1px solid var(--rule); border-bottom: 1px solid var(--rule); text-transform: uppercase; }
    .label { font-size: 0.67rem; letter-spacing: 0.14em; text-transform: uppercase; color: var(--muted); }

    .primary { border-left: 3px solid var(--red); display: flex; flex-direction: column; justify-content: center; min-height: 190px; }
    .primary .big { font-size: 3.6rem; font-weight: 600; line-height: 1; margin-top: 10px; letter-spacing: -0.02em; }
    .primary .big .u { font-size: 1.2rem; color: var(--muted); font-weight: 400; margin-left: 4px; }
    .primary .trend { margin-top: 14px; font-size: 0.8rem; letter-spacing: 0.05em; color: var(--muted); }
    .primary .trend .g { color: var(--red); font-weight: 700; }

    .stack { display: grid; grid-template-rows: 1fr 1fr; gap: 14px; }
    .sub-panel { background: var(--panel); border: 1px solid var(--rule); padding: 15px; position: relative; }
    .sub-panel.b { border-left: 3px solid var(--amber); }
    .sub-panel.c { border-left: 3px solid var(--steel); }
    .sub-panel.d { border-left: 3px solid #c8a83a; }
    .sub-panel .mid { font-size: 1.7rem; font-weight: 600; margin-top: 8px; line-height: 1; }
    .sub-panel .mid .u { font-size: 0.82rem; color: var(--muted); font-weight: 400; margin-left: 3px; }
    .sub-panel .trend, .sub-panel .note { font-size: 0.68rem; color: var(--muted); margin-top: 8px; letter-spacing: 0.03em; }
    .sub-panel .trend .g { color: var(--amber); font-weight: 700; }

    /* ---- metric grids ---- */
    .grid-6 { display: grid; grid-template-columns: repeat(auto-fit, minmax(158px, 1fr)); gap: 0; }
    .cell { background: var(--panel); border: 1px solid var(--rule); padding: 14px 15px; margin: 0 -0.5px -0.5px 0; }
    .cell .value { font-size: 1.42rem; font-weight: 600; margin-top: 8px; line-height: 1; }
    .cell .value .u { font-size: 0.76rem; color: var(--muted); font-weight: 400; margin-left: 2px; }
    .cell .sub { font-size: 0.66rem; color: var(--muted); margin-top: 8px; letter-spacing: 0.02em; }
    .cell.danger { border-color: var(--danger); }
    .cell.danger .value, .cell.danger .label { color: var(--danger); }
    .cell.warn { border-color: var(--warn); }
    .cell.warn .value, .cell.warn .label { color: var(--warn); }

    /* ---- charts ---- */
    .charts { display: grid; grid-template-columns: 1fr; gap: 16px; }
    @media (min-width: 780px) { .charts { grid-template-columns: 1fr 1fr; } }
    .chart-card { background: var(--panel); border: 1px solid var(--rule); padding: 14px; }
    .chart-head { display: flex; justify-content: space-between; align-items: baseline; font-size: 0.68rem; letter-spacing: 0.14em; text-transform: uppercase; color: var(--muted); margin-bottom: 10px; }
    .chart-head .ck { display: inline-flex; align-items: center; gap: 6px; }
    .chart-head .sw { width: 20px; height: 2px; display: inline-block; }
    canvas { width: 100%; max-width: 100%; display: block; background: transparent; }

    /* ---- footer ---- */
    .foot { margin-top: 24px; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 10px; }
    .dl { font-family: var(--mono); font-size: 0.72rem; letter-spacing: 0.1em; text-transform: uppercase; color: var(--ink); text-decoration: none; border: 1px solid var(--rule-hi); padding: 9px 14px; background: var(--panel); }
    .dl:hover { background: var(--red); border-color: var(--red); color: #fff; }
    .dl:focus-visible { outline: 2px solid var(--red); outline-offset: 2px; }
    .sig { font-size: 0.66rem; color: var(--muted); letter-spacing: 0.08em; }

    @media (max-width: 560px) {
      body { padding: 16px 12px 36px; background-size: 18px 18px; }
      .primary .big { font-size: 2.8rem; }
      .meta-cell { flex-basis: 50%; border-bottom: 1px solid var(--rule); }
    }
  </style>
</head>
<body>
  <div class="container">

    <div class="plate">
      <div class="plate-top">
        <div class="brand">
          <span class="title">CA<span class="x">S</span>E</span>
          <span class="expand">Compact Ambient Sensing Environment</span>
        </div>
        <div class="rec"><span class="led"></span> REC &middot; LIVE</div>
      </div>
      <div class="plate-bar"></div>
    </div>

    <div class="meta">
      <div class="meta-cell"><span class="k">TIME</span><span id="timeValue">%TIME%</span></div>
      <div class="meta-cell"><span class="k">UP</span><span id="uptimeValue">%UPTIME%</span></div>
      <div class="meta-cell"><span class="k">IP</span><span id="ipValue">%IP%</span></div>
      <div class="meta-cell"><span class="k">UID</span><span id="uidValue">%UID%</span></div>
    </div>

    <div class="status" id="statusStrip" data-state="ok">
      <div class="dot" id="healthDot"></div>
      <div>
        <span class="status-title" id="healthTitle">System healthy</span>
        <span class="status-sub" id="healthSub">Sensor OK &middot; uploads running &middot; WiFi connected</span>
      </div>
    </div>

    <!-- LIVE CONDITIONS -->
    <div class="section">
      <div class="section-title"><span>Live conditions</span><span class="hint">refresh 3 s</span></div>
      <div class="hero">
        <div class="panel primary">
          <span class="ch">CH A</span>
          <div class="label">Temperature</div>
          <div class="big" id="tempValue">%TEMP%<span class="u">&deg;C</span></div>
          <div class="trend">Trend <span class="g" id="tempTrendValue">%TEMP_TREND%</span></div>
        </div>
        <div class="stack">
          <div class="sub-panel b">
            <span class="ch">CH B</span>
            <div class="label">Humidity</div>
            <div class="mid" id="humValue">%HUM%<span class="u">%</span></div>
            <div class="trend">Trend <span class="g" id="humTrendValue">%HUM_TREND%</span></div>
          </div>
          <div class="sub-panel c">
            <span class="ch">CH C</span>
            <div class="label">Dew point</div>
            <div class="mid" id="dewPointValue">%DEW_POINT%<span class="u">&deg;C</span></div>
            <div class="note">Derived from T + RH</div>
          </div>
          <div class="sub-panel d">
            <span class="ch">CH D</span>
            <div class="label">Pressure</div>
            <div class="mid" id="pressureValue">%PRESSURE%<span class="u">hPa</span></div>
            <div class="trend">Trend <span class="g" id="presTrendValue">%PRES_TREND%</span></div>
          </div>
        </div>
      </div>
    </div>

    <!-- HISTORY SUMMARY -->
    <div class="section">
      <div class="section-title"><span>History summary</span><span class="hint">stored buffer</span></div>
      <div class="grid-6">
        <div class="cell"><div class="label">Min temp</div><div class="value" id="minTempValue">%MIN_TEMP%<span class="u">&deg;C</span></div></div>
        <div class="cell"><div class="label">Max temp</div><div class="value" id="maxTempValue">%MAX_TEMP%<span class="u">&deg;C</span></div></div>
        <div class="cell"><div class="label">Avg temp</div><div class="value" id="avgTempValue">%AVG_TEMP%<span class="u">&deg;C</span></div></div>
        <div class="cell"><div class="label">Min RH</div><div class="value" id="minHumValue">%MIN_HUM%<span class="u">%</span></div></div>
        <div class="cell"><div class="label">Max RH</div><div class="value" id="maxHumValue">%MAX_HUM%<span class="u">%</span></div></div>
        <div class="cell"><div class="label">Avg RH</div><div class="value" id="avgHumValue">%AVG_HUM%<span class="u">%</span></div></div>
      </div>
    </div>

    <!-- SYSTEM HEALTH -->
    <div class="section">
      <div class="section-title"><span>System health</span><span class="hint">counters</span></div>
      <div class="grid-6">
        <div class="cell" id="sensorErrorsCard"><div class="label">Sensor errors</div><div class="value" id="sensorErrorsValue">%SENSOR_ERRORS%</div><div class="sub">Failed reads</div></div>
        <div class="cell"><div class="label">Uploads OK</div><div class="value" id="successfulUploadsValue">%SUCCESSFUL_UPLOADS%</div><div class="sub">Firebase</div></div>
        <div class="cell" id="failedUploadsCard"><div class="label">Uploads failed</div><div class="value" id="failedUploadsValue">%FAILED_UPLOADS%</div><div class="sub">Firebase</div></div>
        <div class="cell" id="wifiReconnectsCard"><div class="label">WiFi reconnects</div><div class="value" id="wifiReconnectsValue">%WIFI_RECONNECTS%</div><div class="sub">Since boot</div></div>
      </div>
    </div>

    <!-- HISTORY TRACES -->
    <div class="section">
      <div class="section-title"><span>History traces</span><span class="hint">old &rarr; new</span></div>
      <div class="charts">
        <div class="chart-card">
          <div class="chart-head"><span>Temperature</span><span class="ck"><span class="sw" style="background:#e2504a"></span>&deg;C</span></div>
          <canvas id="tempGraph" width="760" height="220"></canvas>
        </div>
        <div class="chart-card">
          <div class="chart-head"><span>Humidity</span><span class="ck"><span class="sw" style="background:#3fa7c4"></span>%RH</span></div>
          <canvas id="humGraph" width="760" height="220"></canvas>
        </div>
        <div class="chart-card">
          <div class="chart-head"><span>Pressure</span><span class="ck"><span class="sw" style="background:#c8a83a"></span>hPa</span></div>
          <canvas id="presGraph" width="760" height="220"></canvas>
        </div>
      </div>
    </div>

    <div class="foot">
      <a class="dl" href="/download">&darr; Download CSV log</a>
      <span class="sig">CASE &middot; environment monitor &middot; self-hosted</span>
    </div>
  </div>

<script>
function safeParse(s){ try { return JSON.parse(s); } catch(e){ return []; } }

var UP = String.fromCharCode(0x25B2), DN = String.fromCharCode(0x25BC), EQ = String.fromCharCode(0x2500);

var FRAME = "#363a44", GRIDC = "#1b1d22", MUTED = "#7d828c";
var C_TEMP = "#e2504a", C_HUM = "#3fa7c4", C_PRES = "#c8a83a";

var tempData = safeParse('%TEMP_HISTORY%');
var humData  = safeParse('%HUM_HISTORY%');
var presData = safeParse('%PRES_HISTORY%');

function fmt(value, unit) {
  if (value === null || value === undefined || Number.isNaN(value)) return "N/A";
  return Number(value).toFixed(1) + unit;
}

function trendGlyph(t) {
  var s = (t || "").toLowerCase();
  if (s.indexOf("ris") === 0 || s.indexOf("up") === 0)    return UP + " " + t;
  if (s.indexOf("fall") === 0 || s.indexOf("down") === 0)  return DN + " " + t;
  return EQ + " " + (t || "Steady");
}

function updateHealthBanner(data) {
  var strip = document.getElementById("statusStrip");
  var title = document.getElementById("healthTitle");
  var sub   = document.getElementById("healthSub");
  var se = data.sensorErrors ?? 0, fu = data.failedUploads ?? 0, wr = data.wifiReconnects ?? 0;

  if (se > 0) { strip.dataset.state = "danger"; title.textContent = "Sensor warning"; sub.textContent = se + " failed sensor read(s)"; }
  else if (fu > 0) { strip.dataset.state = "danger"; title.textContent = "Upload warning"; sub.textContent = fu + " Firebase upload failure(s)"; }
  else if (wr > 5) { strip.dataset.state = "warn"; title.textContent = "Connection unstable"; sub.textContent = wr + " WiFi reconnect attempts"; }
  else { strip.dataset.state = "ok"; title.textContent = "System healthy"; sub.textContent = "Sensor OK " + String.fromCharCode(0xB7) + " uploads running " + String.fromCharCode(0xB7) + " WiFi connected"; }
}

function applyCardStates(data) {
  var se = data.sensorErrors ?? 0, fu = data.failedUploads ?? 0, wr = data.wifiReconnects ?? 0;
  document.getElementById("sensorErrorsCard").className   = "cell" + (se > 0 ? " danger" : "");
  document.getElementById("failedUploadsCard").className  = "cell" + (fu > 0 ? " danger" : "");
  document.getElementById("wifiReconnectsCard").className = "cell" + (wr > 5 ? " warn" : "");
}

function drawGraph(canvasId, data, unit, lineColor) {
  var canvas = document.getElementById(canvasId);
  var ctx = canvas.getContext("2d");
  var W = canvas.width, H = canvas.height;
  ctx.clearRect(0, 0, W, H);
  var left = 50, top = 16, width = W - 74, height = H - 52;

  if (!data || data.length < 2) {
    ctx.fillStyle = MUTED; ctx.font = "13px 'JetBrains Mono', ui-monospace, monospace";
    ctx.fillText("Not enough data yet", left, top + 24); return;
  }

  var minVal = Math.min.apply(null, data), maxVal = Math.max.apply(null, data);
  if (minVal === maxVal) { minVal -= 1; maxVal += 1; }
  var pad = (maxVal - minVal) * 0.12; minVal -= pad; maxVal += pad;

  ctx.strokeStyle = GRIDC; ctx.lineWidth = 1;
  ctx.font = "11px 'JetBrains Mono', ui-monospace, monospace"; ctx.fillStyle = MUTED; ctx.textBaseline = "middle";
  for (var i = 0; i <= 4; i++) {
    var y = top + (height / 4) * i;
    ctx.beginPath(); ctx.moveTo(left, y + 0.5); ctx.lineTo(left + width, y + 0.5); ctx.stroke();
    var v = maxVal - ((maxVal - minVal) / 4) * i;
    ctx.fillText(v.toFixed(1), 6, y);
  }

  ctx.strokeStyle = FRAME; ctx.lineWidth = 1; ctx.strokeRect(left + 0.5, top + 0.5, width, height);

  ctx.textBaseline = "alphabetic"; ctx.fillStyle = MUTED;
  ctx.fillText("old", left, H - 8);
  ctx.fillText("new", left + width - 22, H - 8);

  ctx.beginPath();
  for (var j = 0; j < data.length; j++) {
    var x = left + (j * width) / (data.length - 1);
    var yy = top + height - ((data[j] - minVal) / (maxVal - minVal)) * height;
    if (j === 0) ctx.moveTo(x, yy); else ctx.lineTo(x, yy);
  }
  ctx.strokeStyle = lineColor; ctx.lineWidth = 1.75; ctx.stroke();

  var lx = left + width;
  var ly = top + height - ((data[data.length - 1] - minVal) / (maxVal - minVal)) * height;
  ctx.fillStyle = lineColor; ctx.fillRect(lx - 2.5, ly - 2.5, 5, 5);
}

function render(data) {
  document.getElementById("tempValue").innerHTML     = fmt(data.temperature, "") + '<span class="u">&deg;C</span>';
  document.getElementById("humValue").innerHTML      = fmt(data.humidity, "") + '<span class="u">%</span>';
  document.getElementById("dewPointValue").innerHTML = fmt(data.dewPoint, "") + '<span class="u">&deg;C</span>';
  document.getElementById("pressureValue").innerHTML = fmt(data.pressure, "") + '<span class="u">hPa</span>';
  document.getElementById("minTempValue").innerHTML  = fmt(data.minTemp, "") + '<span class="u">&deg;C</span>';
  document.getElementById("maxTempValue").innerHTML  = fmt(data.maxTemp, "") + '<span class="u">&deg;C</span>';
  document.getElementById("avgTempValue").innerHTML  = fmt(data.avgTemperature, "") + '<span class="u">&deg;C</span>';
  document.getElementById("minHumValue").innerHTML   = fmt(data.minHum, "") + '<span class="u">%</span>';
  document.getElementById("maxHumValue").innerHTML   = fmt(data.maxHum, "") + '<span class="u">%</span>';
  document.getElementById("avgHumValue").innerHTML   = fmt(data.avgHumidity, "") + '<span class="u">%</span>';

  document.getElementById("tempTrendValue").textContent = trendGlyph(data.tempTrend);
  document.getElementById("humTrendValue").textContent  = trendGlyph(data.humTrend);
  document.getElementById("presTrendValue").textContent = trendGlyph(data.presTrend);
  document.getElementById("uptimeValue").textContent = data.uptime || "N/A";
  document.getElementById("timeValue").textContent   = data.time || "--:--:--";
  if (data.ip)  document.getElementById("ipValue").textContent  = data.ip;
  if (data.uid) document.getElementById("uidValue").textContent = data.uid;
  document.getElementById("sensorErrorsValue").textContent      = data.sensorErrors ?? 0;
  document.getElementById("successfulUploadsValue").textContent = data.successfulUploads ?? 0;
  document.getElementById("failedUploadsValue").textContent     = data.failedUploads ?? 0;
  document.getElementById("wifiReconnectsValue").textContent    = data.wifiReconnects ?? 0;

  tempData = data.tempHistory || tempData || [];
  humData  = data.humHistory  || humData  || [];
  presData = data.presHistory || presData || [];
  drawGraph("tempGraph", tempData, " C", C_TEMP);
  drawGraph("humGraph",  humData,  " %", C_HUM);
  drawGraph("presGraph", presData, " hPa", C_PRES);
  updateHealthBanner(data);
  applyCardStates(data);
}

function refreshData() {
  fetch("/data").then(function (r) { return r.json(); }).then(render)
    .catch(function () { if (!tempData.length) loadDemo(); });
}

function loadDemo() {
  var t = [], h = [], bt = 21.0, bh = 47;
  for (var i = 0; i < 60; i++) {
    bt += (Math.random() - 0.5) * 0.25; bh += (Math.random() - 0.5) * 0.7;
    t.push(+bt.toFixed(1)); h.push(+bh.toFixed(1));
  }
  render({
    temperature: t[59], humidity: h[59], dewPoint: 10.1,
    avgTemperature: 21.3, avgHumidity: 47.4,
    minTemp: Math.min.apply(null,t), maxTemp: Math.max.apply(null,t),
    minHum: Math.min.apply(null,h), maxHum: Math.max.apply(null,h),
    tempTrend: "Rising", humTrend: "Steady",
    time: "21:42:07", uptime: "3d 04:12:55", ip: "192.168.1.54", uid: "kQ9...demo",
    sensorErrors: 0, successfulUploads: 1284, failedUploads: 0, wifiReconnects: 2,
    tempHistory: t, humHistory: h
  });
}

drawGraph("tempGraph", tempData, " C", C_TEMP);
drawGraph("humGraph",  humData,  " %", C_HUM);
drawGraph("presGraph", presData, " hPa", C_PRES);
if (!tempData.length) loadDemo();
refreshData();
setInterval(refreshData, 3000);
</script>
</body>
</html>
)rawliteral";

    html.replace("%TEMP%", isnan(temperature) ? String("N/A") : String(temperature, 1));
    html.replace("%HUM%", isnan(humidity) ? String("N/A") : String(humidity, 1));
    html.replace("%PRESSURE%", isnan(pressure) ? String("N/A") : String(pressure, 1));
    html.replace("%TIME%", String(timeStr));
    html.replace("%IP%", getLocalIPString());
    html.replace("%UID%", firebaseLocalId);
    html.replace("%UPTIME%", formatUptime(millis()));

    html.replace("%MIN_TEMP%", isnan(minTemp) ? String("N/A") : String(minTemp, 1));
    html.replace("%MAX_TEMP%", isnan(maxTemp) ? String("N/A") : String(maxTemp, 1));
    html.replace("%MIN_HUM%", isnan(minHum) ? String("N/A") : String(minHum, 1));
    html.replace("%MAX_HUM%", isnan(maxHum) ? String("N/A") : String(maxHum, 1));

    html.replace("%AVG_TEMP%", isnan(avgTemperature) ? String("N/A") : String(avgTemperature, 1));
    html.replace("%AVG_HUM%", isnan(avgHumidity) ? String("N/A") : String(avgHumidity, 1));
    html.replace("%DEW_POINT%", isnan(dewPointC) ? String("N/A") : String(dewPointC, 1));
    html.replace("%TEMP_TREND%", tempTrend);
    html.replace("%HUM_TREND%", humTrend);
    html.replace("%PRES_TREND%", presTrend);

    html.replace("%SENSOR_ERRORS%", String(sensorErrorCount));
    html.replace("%SUCCESSFUL_UPLOADS%", String(successfulUploads));
    html.replace("%FAILED_UPLOADS%", String(failedUploads));
    html.replace("%WIFI_RECONNECTS%", String(wifiReconnectCount));

    html.replace("%TEMP_HISTORY%", buildHistoryJSON(tempHistory));
    html.replace("%HUM_HISTORY%", buildHistoryJSON(humHistory));
    html.replace("%PRES_HISTORY%", buildHistoryJSON(presHistory));

    server.send(200, "text/html", html);
  });

  server.on("/download", []() {
    String csv = getCSVLog();
    server.sendHeader("Content-Disposition", "attachment; filename=weather_log.csv");
    server.send(200, "text/csv", csv);
  });

  server.on("/data", []() {
    float minTemp = getHistoryMin(tempHistory);
    float maxTemp = getHistoryMax(tempHistory);
    float minHum  = getHistoryMin(humHistory);
    float maxHum  = getHistoryMax(humHistory);
    float minPres = getHistoryMin(presHistory);
    float maxPres = getHistoryMax(presHistory);

    String json = "{";
    json += "\"temperature\":" + (isnan(temperature) ? String("null") : String(temperature, 1)) + ",";
    json += "\"humidity\":" + (isnan(humidity) ? String("null") : String(humidity, 1)) + ",";
    json += "\"pressure\":" + (isnan(pressure) ? String("null") : String(pressure, 1)) + ",";
    json += "\"avgTemperature\":" + (isnan(avgTemperature) ? String("null") : String(avgTemperature, 1)) + ",";
    json += "\"avgHumidity\":" + (isnan(avgHumidity) ? String("null") : String(avgHumidity, 1)) + ",";
    json += "\"avgPressure\":" + (isnan(avgPressure) ? String("null") : String(avgPressure, 1)) + ",";
    json += "\"dewPoint\":" + (isnan(dewPointC) ? String("null") : String(dewPointC, 1)) + ",";
    json += "\"tempTrend\":\"" + tempTrend + "\",";
    json += "\"humTrend\":\"" + humTrend + "\",";
    json += "\"presTrend\":\"" + presTrend + "\",";
    json += "\"time\":\"" + String(timeStr) + "\",";
    json += "\"ip\":\"" + getLocalIPString() + "\",";
    json += "\"uid\":\"" + firebaseLocalId + "\",";
    json += "\"uptime\":\"" + formatUptime(millis()) + "\",";
    json += "\"sensorErrors\":" + String(sensorErrorCount) + ",";
    json += "\"successfulUploads\":" + String(successfulUploads) + ",";
    json += "\"failedUploads\":" + String(failedUploads) + ",";
    json += "\"wifiReconnects\":" + String(wifiReconnectCount) + ",";
    json += "\"minTemp\":" + (isnan(minTemp) ? String("null") : String(minTemp, 1)) + ",";
    json += "\"maxTemp\":" + (isnan(maxTemp) ? String("null") : String(maxTemp, 1)) + ",";
    json += "\"minHum\":" + (isnan(minHum) ? String("null") : String(minHum, 1)) + ",";
    json += "\"maxHum\":" + (isnan(maxHum) ? String("null") : String(maxHum, 1)) + ",";
    json += "\"minPres\":" + (isnan(minPres) ? String("null") : String(minPres, 1)) + ",";
    json += "\"maxPres\":" + (isnan(maxPres) ? String("null") : String(maxPres, 1)) + ",";
    json += "\"tempHistory\":" + buildHistoryJSON(tempHistory) + ",";
    json += "\"humHistory\":" + buildHistoryJSON(humHistory) + ",";
    json += "\"presHistory\":" + buildHistoryJSON(presHistory);
    json += "}";

    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Web server started");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Open: http://");
    Serial.println(WiFi.localIP());
  }
}
