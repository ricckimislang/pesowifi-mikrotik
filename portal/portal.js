(() => {
  const ESP_IP = "10.0.0.2";
  const API_BASE = `http://${ESP_IP}`;
  const COIN_TIMEOUT = 60;
  const POLL_INTERVAL = 500;

  const state = {
    mac: "",
    ip: "",
    rates: [],
    sessionSeconds: 0,
    remainingSeconds: 0,
    running: false,
    paused: false,
    lastTick: 0,
    coinWindowOpen: false,
    coinSecondsLeft: 0,
    coinTimerId: null,
    pollTimerId: null,
    locked: false,
    currentCoinAmount: 0,
    currentPulses: 0,
  };

  const el = {
    brandName: document.getElementById("brandName"),
    brandSub: document.getElementById("brandSub"),
    statusDot: document.getElementById("statusDot"),
    statusText: document.getElementById("statusText"),
    timeRemaining: document.getElementById("timeRemaining"),
    insertCoinBtn: document.getElementById("insertCoinBtn"),
    connectBtn: document.getElementById("connectBtn"),
    macLabel: document.getElementById("macLabel"),
    ipLabel: document.getElementById("ipLabel"),
    ratesList: document.getElementById("ratesList"),
    coinModal: document.getElementById("coinModal"),
    coinCountdown: document.getElementById("coinCountdown"),
    coinTimerText: document.getElementById("coinTimerText"),
    coinAmount: document.getElementById("coinAmount"),
    pulseCount: document.getElementById("pulseCount"),
    cancelCoinBtn: document.getElementById("cancelCoinBtn"),
    confirmCoinBtn: document.getElementById("confirmCoinBtn"),
    toast: document.getElementById("toast"),
    connectingOverlay: document.getElementById("connectingOverlay"),
  };
  let resumeVoucher = null;

  function fmtTime(totalSeconds) {
    const s = Math.max(0, Math.floor(totalSeconds));
    const m = Math.floor(s / 60);
    const h = Math.floor(m / 60);
    const mm = String(m % 60).padStart(2, "0");
    const ss = String(s % 60).padStart(2, "0");
    if (h > 0) {
      return `${h}:${mm}:${ss}`;
    }
    return `${mm}:${ss}`;
  }

  function showToast(msg) {
    el.toast.textContent = msg;
    el.toast.classList.add("show");
    setTimeout(() => el.toast.classList.remove("show"), 2200);
  }

  function showConnecting(show = true) {
    if (show) {
      el.connectingOverlay.classList.add("open");
    } else {
      el.connectingOverlay.classList.remove("open");
    }
  }

  function getStorageValue(key) {
    if (window.localStorage) {
      return window.localStorage.getItem(key);
    }
    return null;
  }

  function setStorageValue(key, value) {
    if (window.localStorage) {
      window.localStorage.setItem(key, value);
    }
  }

  function removeStorageValue(key) {
    if (window.localStorage) {
      window.localStorage.removeItem(key);
    }
  }

  function setStatus(kind, text) {
    el.statusText.textContent = text;
    el.statusDot.className = "status-dot";
    if (kind) el.statusDot.classList.add(kind);
  }

  async function apiGet(path) {
    showConnecting(true);
    try {
      const r = await fetch(`${API_BASE}${path}`, { cache: "no-store" });
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      return await r.text();
    } catch (e) {
      showToast("ESP unreachable");
      throw e;
    } finally {
      showConnecting(false);
    }
  }

  async function apiPost(path, body) {
    showConnecting(true);
    try {
      const r = await fetch(`${API_BASE}${path}`, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body,
        cache: "no-store",
      });
      if (!r.ok) {
        if (r.status === 403) {
          showToast("Session expired - please try again");
          unlockSession();
          throw new Error("Session expired");
        }
        throw new Error(`HTTP ${r.status}`);
      }
      return await r.text();
    } catch (e) {
      if (e.message !== "Session expired") {
        showToast("ESP unreachable");
      }
      throw e;
    } finally {
      showConnecting(false);
    }
  }

  function parseRates(data) {
    if (!data) return [];
    return data.split("#").filter(Boolean).map(row => {
      const parts = row.split("|");
      return {
        name: parts[0] || "Rate",
        price: parseInt(parts[1] || "0", 10),
        minutes: parseInt(parts[2] || "0", 10),
        validity: parseInt(parts[3] || "0", 10),
        dataLimit: parts[4] || "",
        profile: parts[5] || "default",
      };
    });
  }

  async function loadRates() {
    try {
      const data = await apiGet("/api/rates");
      state.rates = parseRates(data);
      renderRates();
    } catch (e) {
      state.rates = [
        { name: "Piso Load", price: 5, minutes: 15 },
        { name: "Piso Load", price: 10, minutes: 30 },
        { name: "Piso Load", price: 20, minutes: 75 },
        { name: "Piso Load", price: 50, minutes: 240 },
      ];
      renderRates();
    }
  }

  function renderRates() {
    el.ratesList.innerHTML = "";
    if (!state.rates.length) {
      el.ratesList.innerHTML = `<div class="rate-row"><span class="coin">No rates available</span></div>`;
      return;
    }
    for (const r of state.rates) {
      const div = document.createElement("div");
      div.className = "rate-row";
      div.innerHTML = `
        <span class="coin">PHP ${r.price}.00</span>
        <span class="time">${r.minutes} min${r.minutes > 1 ? "s" : ""}</span>
      `;
      el.ratesList.appendChild(div);
    }
  }

  function initDeviceInfo() {
    const mac = el.macLabel.textContent.trim();
    const ip = el.ipLabel.textContent.trim();
    state.mac = (mac && mac !== "$(mac)") ? mac : "";
    state.ip = (ip && ip !== "$(ip)") ? ip : "";
  }

  function openCoinModal() {
    if (!state.mac) {
      showToast("MAC address not available");
      return;
    }
    if (state.locked) {
      showToast("Coin session already active");
      return;
    }
    state.coinWindowOpen = true;
    state.coinSecondsLeft = COIN_TIMEOUT;
    state.currentCoinAmount = 0;
    state.currentPulses = 0;
    el.coinModal.classList.add("open");
    el.confirmCoinBtn.disabled = true;
    updateCoinDisplay();
    startCoinTimer();
    lockSession();
  }

  function closeCoinModal() {
    state.coinWindowOpen = false;
    stopCoinTimer();
    el.coinModal.classList.remove("open");
    if (state.locked && !state.running) {
      unlockSession();
    }
  }

  function startCoinTimer() {
    stopCoinTimer();
    state.coinTimerId = setInterval(() => {
      state.coinSecondsLeft--;
      if (state.coinSecondsLeft <= 0) {
        stopCoinTimer();
        finalizeCoin();
        return;
      }
      el.coinCountdown.textContent = state.coinSecondsLeft;
      el.coinTimerText.textContent = state.coinSecondsLeft;
    }, 1000);
    state.pollTimerId = setInterval(pollCoinStatus, POLL_INTERVAL);
  }

  function stopCoinTimer() {
    if (state.coinTimerId) { clearInterval(state.coinTimerId); state.coinTimerId = null; }
    if (state.pollTimerId) { clearInterval(state.pollTimerId); state.pollTimerId = null; }
  }

  function updateCoinDisplay() {
    el.coinAmount.textContent = `PHP ${state.currentCoinAmount}.00`;
    el.pulseCount.textContent = `${state.currentPulses} pulse${state.currentPulses !== 1 ? "s" : ""}`;
    el.confirmCoinBtn.disabled = state.currentCoinAmount <= 0;
  }

  async function lockSession() {
    try {
      await apiPost("/api/lock", `mac=${encodeURIComponent(state.mac)}&ip=${encodeURIComponent(state.ip)}`);
      state.locked = true;
      setStatus("", "Insert coin now");
      showToast("Session locked — insert coin");
    } catch (e) {
      showToast("Failed to lock session");
      closeCoinModal();
    }
  }

  async function unlockSession() {
    try {
      await apiGet("/api/unlock");
    } catch (e) {}
    state.locked = false;
    setStatus("", "Waiting for coin");
  }

  async function pollCoinStatus() {
    try {
      const data = await apiGet("/api/coinStatus");
      const parts = data.split("|");
      if (parts.length >= 2) {
        const pulses = parseInt(parts[0], 10) || 0;
        const amount = parseInt(parts[1], 10) || 0;
        if (pulses !== state.currentPulses || amount !== state.currentCoinAmount) {
          state.currentPulses = pulses;
          state.currentCoinAmount = amount;
          updateCoinDisplay();
          if (pulses > 0) {
            state.coinSecondsLeft = COIN_TIMEOUT;
            el.coinCountdown.textContent = state.coinSecondsLeft;
            el.coinTimerText.textContent = state.coinSecondsLeft;
          }
        }
      }
    } catch (e) {}
  }

  async function finalizeCoin() {
    stopCoinTimer();
    if (state.currentCoinAmount <= 0) {
      showToast("No coin inserted");
      unlockSession();
      closeCoinModal();
      return;
    }
    let shouldAutoLogin = false;
    let voucher = "";
    try {
      const data = await apiPost("/api/finalize", `mac=${encodeURIComponent(state.mac)}`);
      const parts = data.split("|");
      const minutes = parseInt(parts[0], 10) || 0;
      const okToken = (parts[1] || "").trim().toLowerCase();
      const ok = okToken === "1" || okToken === "ok" || okToken === "true";
      voucher = (parts[2] || "").trim();

      if (ok && minutes > 0) {
        addTime(minutes * 60);
        showToast(`Added ${minutes} minutes`);
        shouldAutoLogin = !!voucher;
        if (!voucher) showToast("Voucher not received");
      } else if (!ok) {
        showToast("Voucher creation failed");
      } else if (minutes <= 0) {
        showToast("No time added");
      }
    } catch (e) {
      showToast("Failed to finalize coin");
    }
    state.locked = false;
    closeCoinModal();

    if (shouldAutoLogin) {
      const form = document.getElementById("mtLoginForm");
      const user = document.getElementById("mtUser");
      const pass = document.getElementById("mtPass");
      if (!form || !user || !pass) {
        showToast(`Voucher: ${voucher}`);
        return;
      }
      showToast(`Voucher: ${voucher}`);
      user.value = voucher;
      pass.value = "";
      form.submit();
    }
  }

  function addTime(seconds) {
    state.sessionSeconds += seconds;
    state.remainingSeconds += seconds;
    if (!state.running) {
      state.running = true;
      state.paused = false;
      state.lastTick = performance.now();
      setStatus("active", "Session active");
      el.connectBtn.disabled = false;
      tick(performance.now());
    }
    render();
  }

  function tick(now) {
    if (!state.running || state.paused) return;
    if (state.remainingSeconds <= 0) {
      state.remainingSeconds = 0;
      state.running = false;
      el.connectBtn.disabled = true;
      setStatus("expired", "Time ended");
      render();
      return;
    }
    const dt = now - state.lastTick;
    if (dt >= 1000) {
      const steps = Math.floor(dt / 1000);
      state.remainingSeconds = Math.max(0, state.remainingSeconds - steps);
      state.lastTick += steps * 1000;
      render();
    }
    requestAnimationFrame(tick);
  }

  function render() {
    el.timeRemaining.textContent = fmtTime(state.remainingSeconds);
    if (state.remainingSeconds > 0) {
      if (state.paused) {
        el.statusText.textContent = "Paused session";
        el.statusDot.className = "status-dot";
      } else if (state.running) {
        el.statusText.textContent = "Session active";
        el.statusDot.className = "status-dot active";
      } else {
        el.statusText.textContent = "Ready to connect";
        el.statusDot.className = "status-dot";
      }
    } else if (state.sessionSeconds > 0 && state.remainingSeconds === 0) {
      el.statusText.textContent = "Time ended";
      el.statusDot.className = "status-dot expired";
    } else {
      el.statusText.textContent = "Waiting for coin";
      el.statusDot.className = "status-dot";
    }
  }

  function connectInternet() {
    if (resumeVoucher && state.remainingSeconds > 0) {
      const form = document.getElementById("mtLoginForm");
      const user = document.getElementById("mtUser");
      const pass = document.getElementById("mtPass");
      if (form && user && pass) {
        user.value = resumeVoucher;
        pass.value = "";
        showConnecting(true);
        form.submit();
        return;
      }
    }

    if (!state.running || state.remainingSeconds <= 0) {
      showToast("Insert coin first");
      return;
    }
    const loginUrl = document.body.dataset.loginUrl || "";
    if (loginUrl) {
      showConnecting(true);
      window.location.href = loginUrl;
    } else {
      showToast("Connected!");
    }
  }

  function loadResumeSession() {
    const resumedSeconds = parseInt(getStorageValue("resumedSessionSeconds"), 10);
    const resumedVoucher = getStorageValue("resumedSessionVoucher");
    const pausedVoucher = getStorageValue("activeVoucher");
    const pausedSeconds = parseInt(getStorageValue((pausedVoucher || "") + "remainSeconds"), 10);
    const remainingMikrotikTime = parseInt(typeof sessiontime !== 'undefined' ? sessiontime : "", 10);
    const hasMikrotikTime = !isNaN(remainingMikrotikTime) && remainingMikrotikTime > 0;

    let resumeSeconds = 0;
    let resumeCode = null;

    if (resumedSeconds > 0 && resumedVoucher) {
      resumeSeconds = resumedSeconds;
      resumeCode = resumedVoucher;
    } else if (pausedVoucher && pausedSeconds > 0) {
      resumeSeconds = pausedSeconds;
      resumeCode = pausedVoucher;
    } else if (hasMikrotikTime && currentVoucher) {
      resumeSeconds = remainingMikrotikTime;
      resumeCode = currentVoucher;
    }

    if (resumeSeconds > 0 && resumeCode) {
      resumeVoucher = resumeCode;
      state.sessionSeconds = resumeSeconds;
      state.remainingSeconds = resumeSeconds;
      state.running = true;
      state.paused = true;
      el.connectBtn.disabled = false;
      el.connectBtn.textContent = "Resume Internet";
      const user = document.getElementById("mtUser");
      const pass = document.getElementById("mtPass");
      if (user) {
        user.value = resumeVoucher;
      }
      if (pass) {
        pass.value = "";
      }
      showToast("Remaining session detected. Press Connect to resume.");
      render();
    }
  }

  function wireEvents() {
    el.insertCoinBtn.addEventListener("click", openCoinModal);
    el.cancelCoinBtn.addEventListener("click", () => {
      stopCoinTimer();
      unlockSession();
      closeCoinModal();
    });
    el.confirmCoinBtn.addEventListener("click", finalizeCoin);
    el.connectBtn.addEventListener("click", connectInternet);
    el.coinModal.addEventListener("click", (e) => {
      if (e.target === el.coinModal) {
        stopCoinTimer();
        unlockSession();
        closeCoinModal();
      }
    });
    window.addEventListener("keydown", (e) => {
      if (e.key === "Escape" && el.coinModal.classList.contains("open")) {
        stopCoinTimer();
        unlockSession();
        closeCoinModal();
      }
    });
  }

  async function init() {
    initDeviceInfo();
    await loadRates();
    wireEvents();
    setStatus("", "Waiting for coin");
    loadResumeSession();
    render();
  }

  init();
})();
