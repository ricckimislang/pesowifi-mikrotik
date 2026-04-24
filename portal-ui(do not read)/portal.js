(() => {
  const $ = (sel) => document.querySelector(sel);

  const CONFIG = {
    brandName: "WAYPAY",
    brandSubtitle: "Insert coin to start",
    brandLogoText: "WP",
    // Coin value → minutes mapping
    rates: [
      { coin: 5, minutes: 15 },
      { coin: 10, minutes: 30 },
      { coin: 20, minutes: 75 },
      { coin: 50, minutes: 240 },
    ],
    // Default coin value for simulation
    defaultCoin: 5,
    tickMs: 1000,
    coinWindowSeconds: 60,
  };

  const state = {
    totalSeconds: 0,
    remainingSeconds: 0,
    running: false,
    paused: false,
    totalPesos: 0,
    tickHandle: null,
    modalCoins: 0,
    modalSeconds: 0,
    coinWindowRemaining: 0,
    coinWindowActive: false,
    coinWindowHandle: null,
  };

  const el = {
    brandName: $("#brandName"),
    brandSub: $("#brandSub"),
    statusText: $("#statusText"),
    brandLogo: $("#brandLogo"),

    timeRemaining: $("#timeRemaining"),
    usedTotal: $("#usedTotal"),
    progressBar: $("#progressBar"),

    insertCoinBtn: $("#insertCoinBtn"),
    pauseResumeBtn: $("#pauseResumeBtn"),
    resetBtn: $("#resetBtn"),

    coinModal: $("#coinModal"),
    timerProgressBar: $("#timerProgressBar"),
    timerText: $("#timerText"),
    coinAmount: $("#coinAmount"),
    coinTime: $("#coinTime"),
    insertCoinBtnModal: $("#insertCoinBtnModal"),
    cancelCoinBtn: $("#cancelCoinBtn"),
    confirmCoinBtn: $("#confirmCoinBtn"),

    macAddress: $("#macAddress"),
    ipAddress: $("#ipAddress"),
  };

  function clamp(n, min, max) {
    return Math.max(min, Math.min(max, n));
  }

  function fmtTime(seconds) {
    const s = Math.max(0, Math.floor(seconds));
    const m = Math.floor(s / 60);
    const r = s % 60;
    const mm = String(m).padStart(2, "0");
    const ss = String(r).padStart(2, "0");
    return `${mm}:${ss}`;
  }

  function fmtTimeWithMinutes(minutes) {
    if (minutes >= 60) {
      const hours = Math.floor(minutes / 60);
      const mins = minutes % 60;
      if (mins === 0) {
        return `${hours} hour${hours > 1 ? 's' : ''}`;
      }
      return `${hours} hour${hours > 1 ? 's' : ''} ${mins} minute${mins > 1 ? 's' : ''}`;
    }
    return `${minutes} minute${minutes > 1 ? 's' : ''}`;
  }

  function setStatus(kind, text) {
    el.statusText.textContent = text;
    el.statusText.style.color = kind === "danger" ? "#e53e3e" : "";
  }

  function startIfNeeded() {
    if (state.running) return;
    state.running = true;
    state.paused = false;
    el.pauseResumeBtn.disabled = false;
    startTicking();
  }

  function openModal() {
    state.modalCoins = 0;
    state.modalSeconds = 0;
    state.coinWindowRemaining = CONFIG.coinWindowSeconds;
    state.coinWindowActive = true;
    el.coinModal.classList.add("open");
    el.confirmCoinBtn.disabled = true;
    updateModalDisplay();
    startCoinWindowTimer();
  }

  function closeModal() {
    el.coinModal.classList.remove("open");
    state.coinWindowActive = false;
    if (state.coinWindowHandle) {
      clearInterval(state.coinWindowHandle);
      state.coinWindowHandle = null;
    }
  }

  function updateModalDisplay() {
    el.coinAmount.textContent = `₱${state.modalCoins.toFixed(2)}`;
    const minutes = Math.floor(state.modalSeconds / 60);
    el.coinTime.textContent = fmtTimeWithMinutes(minutes);
    const progress = (state.coinWindowRemaining / CONFIG.coinWindowSeconds) * 100;
    el.timerProgressBar.style.width = `${progress}%`;
    el.timerText.textContent = `${state.coinWindowRemaining}s`;
  }

  function addModalCoin(pesos) {
    state.modalCoins += pesos;
    const rate = CONFIG.rates.find(r => r.coin === pesos);
    if (rate) {
      state.modalSeconds += rate.minutes * 60;
    } else {
      state.modalSeconds += pesos * CONFIG.defaultCoin * 3;
    }
    // Reset timer on coin insertion
    state.coinWindowRemaining = CONFIG.coinWindowSeconds;
    el.confirmCoinBtn.disabled = false;
    updateModalDisplay();
  }

  function confirmCoins() {
    if (state.modalCoins > 0) {
      state.totalSeconds += state.modalSeconds;
      state.remainingSeconds += state.modalSeconds;
      state.totalPesos += state.modalCoins;
      startIfNeeded();
    }
    closeModal();
    render();
  }

  function startCoinWindowTimer() {
    if (state.coinWindowHandle) {
      clearInterval(state.coinWindowHandle);
    }
    state.coinWindowHandle = setInterval(() => {
      state.coinWindowRemaining--;
      const progress = (state.coinWindowRemaining / CONFIG.coinWindowSeconds) * 100;
      el.timerProgressBar.style.width = `${progress}%`;
      el.timerText.textContent = `${state.coinWindowRemaining}s`;
      if (state.coinWindowRemaining <= 0) {
        clearInterval(state.coinWindowHandle);
        state.coinWindowHandle = null;
        if (state.modalCoins > 0) {
          confirmCoins();
        } else {
          closeModal();
        }
      }
    }, 1000);
  }

  function reset() {
    state.totalSeconds = 0;
    state.remainingSeconds = 0;
    state.running = false;
    state.paused = false;
    state.totalPesos = 0;
    state.modalCoins = 0;
    state.modalSeconds = 0;
    stopTicking();
    el.pauseResumeBtn.disabled = true;
    el.pauseResumeBtn.textContent = "Pause";
    render();
  }

  function togglePauseResume() {
    if (!state.running) return;
    state.paused = !state.paused;
    el.pauseResumeBtn.textContent = state.paused ? "Resume" : "Pause";
    render();
    if (!state.paused) {
      startTicking();
    }
  }

  function startTicking() {
    if (state.tickHandle) return;
    state.tickHandle = window.setInterval(onTick, CONFIG.tickMs);
  }

  function stopTicking() {
    if (!state.tickHandle) return;
    window.clearInterval(state.tickHandle);
    state.tickHandle = null;
  }

  function getTopStatus() {
    if (state.coinWindowActive) {
      return { kind: "warn", text: `Insert coin (${state.coinWindowRemaining}s)` };
    }
    if (state.running && state.remainingSeconds > 0) {
      return { kind: state.paused ? "warn" : "ok", text: state.paused ? "Paused" : "Session active" };
    }
    if (state.totalSeconds > 0 && state.remainingSeconds === 0) {
      return { kind: "danger", text: "Time ended" };
    }
    return { kind: "warn", text: "Waiting for coin" };
  }

  function onTick() {
    if (state.running && !state.paused) {
      if (state.remainingSeconds <= 0) {
        state.remainingSeconds = 0;
        state.running = false;
        el.pauseResumeBtn.disabled = true;
        render();
        return;
      }
      state.remainingSeconds = Math.max(0, state.remainingSeconds - 1);
    }
    render();
  }

  function render() {
    el.brandName.textContent = CONFIG.brandName;
    el.brandSub.textContent = CONFIG.brandSubtitle;
    if (el.brandLogo) el.brandLogo.textContent = CONFIG.brandLogoText;

    el.timeRemaining.textContent = fmtTime(state.remainingSeconds);

    const used = Math.max(0, state.totalSeconds - state.remainingSeconds);
    el.usedTotal.textContent = `${fmtTime(used)} / ${fmtTime(state.totalSeconds)}`;

    const pct = state.totalSeconds > 0 ? (used / state.totalSeconds) * 100 : 0;
    el.progressBar.style.width = `${clamp(pct, 0, 100).toFixed(2)}%`;

    const top = getTopStatus();
    setStatus(top.kind, top.text);
  }

  function wireEvents() {
    el.insertCoinBtn.addEventListener("click", () => openModal());
    el.pauseResumeBtn.addEventListener("click", () => togglePauseResume());
    el.resetBtn.addEventListener("click", () => {
      reset();
    });

    el.insertCoinBtnModal.addEventListener("click", () => addModalCoin(CONFIG.defaultCoin));
    el.cancelCoinBtn.addEventListener("click", () => closeModal());
    el.confirmCoinBtn.addEventListener("click", () => confirmCoins());

    window.addEventListener("keydown", (e) => {
      if (e.key === "Escape" && el.coinModal.classList.contains("open")) {
        closeModal();
      }
    });
  }

  function init() {
    wireEvents();
    reset();
    startTicking();
    fetchDeviceInfo();
  }

  async function fetchDeviceInfo() {
    try {
      const response = await fetch('https://api.ipify.org?format=json');
      const data = await response.json();
      if (el.ipAddress) el.ipAddress.textContent = data.ip;
    } catch (err) {
      if (el.ipAddress) el.ipAddress.textContent = "Unable to fetch";
    }
    // MAC address would be provided by backend/router in production
    if (el.macAddress) el.macAddress.textContent = "AA:BB:CC:DD:EE:FF";
  }

  init();
})();
