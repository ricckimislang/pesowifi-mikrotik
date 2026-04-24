(() => {
  const API_BASE = ""; // relative URLs — admin portal served from ESP same-origin
  const SEP = "|";
  const ROW = "#";
  let uptimeTimer = null;
  let uptimeSeconds = 0;
  let loggedIn = false;

  const el = {
    loginOverlay: document.getElementById("loginOverlay"),
    loginUser: document.getElementById("loginUser"),
    loginPass: document.getElementById("loginPass"),
    spinner: document.getElementById("spinner"),
    loadingText: document.getElementById("loadingText"),
    toast: document.getElementById("toast"),
    lifeIncome: document.getElementById("lifeIncome"),
    currentSales: document.getElementById("currentSales"),
    customerCount: document.getElementById("customerCount"),
    upTime: document.getElementById("upTime"),
    netStatus: document.getElementById("netStatus"),
    mtStatus: document.getElementById("mtStatus"),
    espMac: document.getElementById("espMac"),
    espIp: document.getElementById("espIp"),
    rateList: document.getElementById("rateList"),
    voucherOutput: document.getElementById("voucherOutput"),
    staticBlock: document.getElementById("staticBlock"),
    pinCoinSlot: document.getElementById("pinCoinSlot"),
    pinCoinSet: document.getElementById("pinCoinSet"),
    pinReadyLed: document.getElementById("pinReadyLed"),
    pinInsertLed: document.getElementById("pinInsertLed"),
    pinInsertBtn: document.getElementById("pinInsertBtn"),
  };

  function showToast(msg) {
    el.toast.textContent = msg;
    el.toast.classList.add("show");
    setTimeout(() => el.toast.classList.remove("show"), 2200);
  }

  function showSpinner(text) {
    el.loadingText.textContent = text || "Loading...";
    el.spinner.classList.add("open");
  }

  function hideSpinner() {
    el.spinner.classList.remove("open");
  }

  function fmtUptime(s) {
    const d = Math.floor(s / 86400);
    const h = Math.floor((s % 86400) / 3600);
    const m = Math.floor((s % 3600) / 60);
    if (d > 0) return `${d}d ${h}h ${m}m`;
    if (h > 0) return `${h}h ${m}m`;
    return `${m}m`;
  }

  async function apiGet(path) {
    const headers = {};
    if (window.adminAuth) headers["Authorization"] = "Basic " + window.adminAuth;
    const r = await fetch(`${API_BASE}${path}`, { headers, cache: "no-store" });
    if (r.status === 401) {
      showLoginOverlay();
      throw new Error("Unauthorized");
    }
    if (!r.ok) throw new Error(`HTTP ${r.status}`);
    return await r.text();
  }

  async function apiPost(path, body) {
    const headers = { "Content-Type": "application/x-www-form-urlencoded" };
    if (window.adminAuth) headers["Authorization"] = "Basic " + window.adminAuth;
    const r = await fetch(`${API_BASE}${path}`, {
      method: "POST",
      headers,
      body,
      cache: "no-store",
    });
    if (r.status === 401) {
      showLoginOverlay();
      throw new Error("Unauthorized");
    }
    if (!r.ok) throw new Error(`HTTP ${r.status}`);
    return await r.text();
  }

  function showLoginOverlay() {
    el.loginOverlay.style.display = "flex";
    loggedIn = false;
    if (uptimeTimer) { clearInterval(uptimeTimer); uptimeTimer = null; }
  }

  function doLogin() {
    const u = el.loginUser.value.trim();
    const p = el.loginPass.value.trim();
    if (!u || !p) { showToast("Enter username and password"); return; }
    showSpinner("Authenticating...");
    
    // Test credentials with any admin endpoint
    const auth = btoa(u + ":" + p);
    fetch(`${API_BASE}/admin/api/dashboard`, {
      headers: { "Authorization": "Basic " + auth },
      cache: "no-store"
    })
    .then(r => {
      if (r.ok) {
        loggedIn = true;
        el.loginOverlay.style.display = "none";
        hideSpinner();
        showToast("Login successful");
        refreshDashboard();
        // Store auth for subsequent requests
        window.adminAuth = auth;
      } else {
        hideSpinner();
        showToast("Login failed");
      }
    })
    .catch(e => { hideSpinner(); showToast("Login failed"); });
  }

  function logout() {
    window.adminAuth = null;
    showLoginOverlay();
    showToast("Logged out");
  }

  window.toggleNav = function() {
    const nav = document.getElementById("topnav");
    nav.classList.toggle("responsive");
  };

  window.openTab = function(evt, name) {
    document.querySelectorAll(".tabcontent").forEach(t => t.classList.remove("active"));
    document.querySelectorAll(".tablinks").forEach(t => t.classList.remove("active"));
    document.getElementById(name).classList.add("active");
    if (evt && evt.currentTarget) evt.currentTarget.classList.add("active");
    if (name === "Dashboard") refreshDashboard();
    if (name === "SystemConfig") loadSystemConfig();
    if (name === "PromoRates") loadRates();
  };

  window.toggleStatic = function() {
    el.staticBlock.style.display = document.getElementById("ipMode").value === "1" ? "block" : "none";
  };

  function populatePins(select) {
    select.innerHTML = "";
    const pins = [
      [16,"D0"],[5,"D1"],[4,"D2"],[0,"D3"],[2,"D4"],
      [14,"D5"],[12,"D6"],[13,"D7"],[15,"D8"],[3,"RX"],[1,"TX"]
    ];
    pins.forEach(([val,label]) => {
      const opt = document.createElement("option");
      opt.value = val;
      opt.textContent = `${label} (GPIO ${val})`;
      select.appendChild(opt);
    });
  }

  [el.pinCoinSlot, el.pinCoinSet, el.pinReadyLed, el.pinInsertLed, el.pinInsertBtn].forEach(populatePins);

  window.refreshDashboard = function() {
    showSpinner("Loading dashboard...");
    apiGet("/admin/api/dashboard?" + Date.now())
      .then(data => {
        hideSpinner();
        const parts = data.split(SEP);
        if (uptimeTimer) clearInterval(uptimeTimer);
        uptimeSeconds = Math.floor(parseInt(parts[0] || "0", 10) / 1000);
        el.lifeIncome.textContent = parts[1] || "0";
        el.currentSales.textContent = parts[2] || "0";
        el.customerCount.textContent = parts[3] || "0";
        el.upTime.textContent = fmtUptime(uptimeSeconds);
        uptimeTimer = setInterval(() => { uptimeSeconds++; el.upTime.textContent = fmtUptime(uptimeSeconds); }, 1000);
        const net = parts[4] === "1";
        const mt = parts[5] === "1";
        el.netStatus.textContent = net ? "Online" : "Offline";
        el.netStatus.className = "value " + (net ? "good" : "danger");
        el.mtStatus.textContent = mt ? "Online" : "Offline";
        el.mtStatus.className = "value " + (mt ? "good" : "danger");
        el.espMac.textContent = parts[6] || "--";
        el.espIp.textContent = parts[7] || "--";
        document.getElementById("aboutVersion").textContent = parts[9] || "1.0.0";
        document.getElementById("fwVersion").value = parts[9] || "1.0.0";
      })
      .catch(e => { hideSpinner(); showToast("Failed to load dashboard"); });
  };

  window.resetStats = function(type) {
    if (!confirm("Reset this statistic?")) return;
    showSpinner("Resetting...");
    apiGet("/admin/api/resetStatistic?type=" + encodeURIComponent(type) + "&" + Date.now())
      .then(() => { hideSpinner(); showToast("Reset done"); refreshDashboard(); })
      .catch(e => { hideSpinner(); showToast("Reset failed"); });
  };

  window.loadSystemConfig = function() {
    showSpinner("Loading config...");
    apiGet("/admin/api/getSystemConfig?" + Date.now())
      .then(data => {
        hideSpinner();
        const p = data.split(SEP);
        const fields = [
          "vendoName","wifiSSID","wifiPW","mtIp","mtUser","mtPw","coinWaitTime",
          "adminUser","adminPw","abuseCount","banMinutes","pinCoinSlot","pinCoinSet",
          "pinReadyLed","pinInsertLed","lcdScreen","pinInsertBtn","checkInternet",
          "voucherPrefix","welcomeMsg","setupDone","voucherLoginOpt","voucherProfile",
          "voucherValidity","ledTrigger","ipMode","localIp","gatewayIp","subnetMask","dnsServer"
        ];
        fields.forEach((id, i) => {
          const el = document.getElementById(id);
          if (el) el.value = p[i] || "";
        });
        if (document.getElementById("mtPwConfirm")) document.getElementById("mtPwConfirm").value = p[5] || "";
        if (document.getElementById("adminPwConfirm")) document.getElementById("adminPwConfirm").value = p[8] || "";
        toggleStatic();
      })
      .catch(e => { hideSpinner(); showToast("Failed to load config"); });
  };

  window.saveSystemConfig = function() {
    const mtPw = document.getElementById("mtPw").value;
    if (mtPw !== document.getElementById("mtPwConfirm").value) { showToast("MikroTik passwords do not match"); return; }
    const adminPw = document.getElementById("adminPw").value;
    if (adminPw !== document.getElementById("adminPwConfirm").value) { showToast("Admin passwords do not match"); return; }

    const pinMap = {};
    ["pinCoinSlot","pinCoinSet","pinReadyLed","pinInsertLed","pinInsertBtn"].forEach(id => {
      const name = id.replace("pin","").replace(/([A-Z])/g," $1").trim();
      pinMap[name] = document.getElementById(id).value;
    });
    for (const a in pinMap) {
      for (const b in pinMap) {
        if (a !== b && pinMap[a] === pinMap[b]) { showToast(`${b} already used by ${a}`); return; }
      }
    }

    const vals = [
      "vendoName","wifiSSID","wifiPW","mtIp","mtUser","mtPw","coinWaitTime",
      "adminUser","adminPw","abuseCount","banMinutes","pinCoinSlot","pinCoinSet",
      "pinReadyLed","pinInsertLed","lcdScreen","pinInsertBtn","checkInternet",
      "voucherPrefix","welcomeMsg","setupDone","voucherLoginOpt","voucherProfile",
      "voucherValidity","ledTrigger","ipMode","localIp","gatewayIp","subnetMask","dnsServer"
    ].map(id => document.getElementById(id)?.value ?? "");

    const postData = vals.map(v => encodeURIComponent(v)).join(SEP);
    showSpinner("Saving config...");
    apiPost("/admin/api/saveSystemConfig", "data=" + postData)
      .then(() => { hideSpinner(); showToast("Config saved. Rebooting..."); })
      .catch(e => { hideSpinner(); showToast("Save failed"); });
  };

  window.exportConfig = function() {
    apiGet("/admin/api/getSystemConfig?" + Date.now())
      .then(data => {
        const blob = new Blob([data], { type: "text/plain" });
        const a = document.createElement("a");
        a.href = URL.createObjectURL(blob);
        a.download = "system.data";
        a.click();
      })
      .catch(() => showToast("Export failed"));
  };

  window.importConfig = function() { document.getElementById("configFile").click(); };
  window.onConfigImport = function(input) {
    const f = input.files[0];
    if (!f) return;
    const r = new FileReader();
    r.onload = e => {
      const p = e.target.result.split(SEP);
      const fields = [
        "vendoName","wifiSSID","wifiPW","mtIp","mtUser","mtPw","coinWaitTime",
        "adminUser","adminPw","abuseCount","banMinutes","pinCoinSlot","pinCoinSet",
        "pinReadyLed","pinInsertLed","lcdScreen","pinInsertBtn","checkInternet",
        "voucherPrefix","welcomeMsg","setupDone","voucherLoginOpt","voucherProfile",
        "voucherValidity","ledTrigger","ipMode","localIp","gatewayIp","subnetMask","dnsServer"
      ];
      fields.forEach((id, i) => { const el = document.getElementById(id); if (el) el.value = p[i] || ""; });
      toggleStatic();
      showToast("Config loaded. Click Save to apply.");
    };
    r.readAsText(f);
    input.value = "";
  };

  let rateCount = 0;
  window.addRate = function(data) {
    data = data || ["","","","","",""];
    const div = document.createElement("div");
    div.className = "rate-row";
    div.id = "rateRow" + rateCount;
    div.innerHTML = `
      <div class="grid">
        <div class="form-group"><label>Rate Name</label><input type="text" id="rName${rateCount}" value="${data[0]}" placeholder="Rate Name"></div>
        <div class="form-group"><label>Price (PHP)</label><input type="number" id="rPrice${rateCount}" value="${data[1]}" placeholder="5"></div>
        <div class="form-group"><label>Minutes</label><input type="number" id="rMin${rateCount}" value="${data[2]}" placeholder="15"></div>
        <div class="form-group"><label>Validity (min)</label><input type="number" id="rVal${rateCount}" value="${data[3]}" placeholder="0"></div>
        <div class="form-group"><label>Data Limit (MB)</label><input type="number" id="rData${rateCount}" value="${data[4]}" placeholder="Optional"></div>
        <div class="form-group"><label>Profile</label><input type="text" id="rProf${rateCount}" value="${data[5]}" placeholder="default"></div>
      </div>
      <button class="btn btn-danger" style="margin-top:8px;" onclick="deleteRate(${rateCount})">Delete</button>
    `;
    el.rateList.appendChild(div);
    rateCount++;
  };

  window.deleteRate = function(idx) {
    const row = document.getElementById("rateRow" + idx);
    if (row) row.remove();
  };

  window.loadRates = function() {
    showSpinner("Loading rates...");
    apiGet("/admin/api/getRates?" + Date.now())
      .then(data => {
        hideSpinner();
        el.rateList.innerHTML = "";
        rateCount = 0;
        if (!data.trim()) return;
        data.split(ROW).forEach(row => {
          if (!row.trim()) return;
          addRate(row.split("|"));
        });
      })
      .catch(e => { hideSpinner(); showToast("Failed to load rates"); });
  };

  window.saveRates = function() {
    const params = [];
    for (let i = 0; i < rateCount; i++) {
      const name = document.getElementById("rName" + i)?.value ?? "";
      const price = document.getElementById("rPrice" + i)?.value ?? "";
      const min = document.getElementById("rMin" + i)?.value ?? "";
      const val = document.getElementById("rVal" + i)?.value ?? "";
      const data = document.getElementById("rData" + i)?.value ?? "";
      const prof = document.getElementById("rProf" + i)?.value ?? "";
      if (name !== null && price !== null && min !== null) {
        params.push([name, price, min, val, data, prof].join("|"));
      }
    }
    const postData = params.map(v => encodeURIComponent(v)).join(SEP);
    showSpinner("Saving rates...");
    apiPost("/admin/api/saveRates", "data=" + postData)
      .then(() => { hideSpinner(); showToast("Rates saved"); })
      .catch(e => { hideSpinner(); showToast("Save failed"); });
  };

  window.exportRates = function() {
    apiGet("/admin/api/getRates?" + Date.now())
      .then(data => {
        const blob = new Blob([data], { type: "text/plain" });
        const a = document.createElement("a");
        a.href = URL.createObjectURL(blob);
        a.download = "rates.data";
        a.click();
      })
      .catch(() => showToast("Export failed"));
  };

  window.importRates = function() { document.getElementById("ratesFile").click(); };
  window.onRatesImport = function(input) {
    const f = input.files[0];
    if (!f) return;
    const r = new FileReader();
    r.onload = e => {
      el.rateList.innerHTML = "";
      rateCount = 0;
      e.target.result.split(ROW).forEach(row => { if (row.trim()) addRate(row.split("|")); });
      showToast("Rates loaded. Click Save to apply.");
    };
    r.readAsText(f);
    input.value = "";
  };

  window.generateVouchers = function() {
    const prefix = document.getElementById("voucherPrefix").value.trim().toUpperCase();
    const amount = parseInt(document.getElementById("voucherAmount").value, 10) || 0;
    const qty = parseInt(document.getElementById("voucherQty").value, 10) || 1;
    const sales = document.getElementById("voucherAddSales").value;
    if (!prefix || prefix.length > 2) { showToast("Prefix must be 1-2 letters"); return; }
    if (amount <= 0) { showToast("Enter valid amount"); return; }
    if (qty < 1 || qty > 15) { showToast("Quantity: 1-15"); return; }
    showSpinner("Generating vouchers...");
    apiPost("/admin/api/generateVouchers", `amt=${amount}&pfx=${prefix}&qty=${qty}&sales=${sales}`)
      .then(data => {
        hideSpinner();
        el.voucherOutput.textContent = data;
        showToast(`Generated ${qty} voucher(s)`);
      })
      .catch(e => { hideSpinner(); showToast("Generation failed"); });
  };

  window.uploadFirmware = function() {
    const fileInput = document.getElementById("binFile");
    const type = document.getElementById("binType").value;
    if (!fileInput.files.length) { showToast("Select a .bin file"); return; }
    const f = fileInput.files[0];
    const sizeMB = f.size / 1024 / 1024;
    if (type === "filesystem" && sizeMB < 0.5) { showToast("Filesystem bin too small (expected >500KB)"); return; }
    if (type === "sketch" && sizeMB > 1) { showToast("Sketch bin too large (expected <1MB)"); return; }
    const fd = new FormData();
    fd.append(type, f);
    showSpinner("Uploading firmware...");
    fetch(`${API_BASE}/admin/updateMainBin`, { method: "POST", body: fd, credentials: "include" })
      .then(r => { if (!r.ok) throw new Error("Upload failed"); return r.text(); })
      .then(() => { hideSpinner(); showToast("Firmware uploaded. Rebooting..."); })
      .catch(e => { hideSpinner(); showToast("Upload failed"); });
  };

  window.logout = logout;
  window.doLogin = doLogin;

  if (el.loginOverlay) el.loginOverlay.style.display = "flex";
})();
