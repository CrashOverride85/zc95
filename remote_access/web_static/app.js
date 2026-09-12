(() => {
  "use strict";

  const statusEl = document.getElementById("status");
  const patternPickerEl = document.getElementById("pattern-picker");
  const patternSelectEl = document.getElementById("pattern-select");
  const startBtn = document.getElementById("start-btn");
  const patternRunningEl = document.getElementById("pattern-running");
  const patternNameEl = document.getElementById("pattern-name");
  const triphaseBadgeEl = document.getElementById("triphase-badge");
  const stopBtn = document.getElementById("stop-btn");
  const themeToggleBtn = document.getElementById("theme-toggle");
  const softButtonRowEl = document.getElementById("soft-button-row");
  const softButtonEl = document.getElementById("soft-button");
  const menuItemsEl = document.getElementById("menu-items");
  const channelsEl = document.getElementById("channels");
  const logEl = document.getElementById("log");
  const logToggleBtn = document.getElementById("log-toggle");

  const tmplMinMax = document.getElementById("tmpl-min-max");
  const tmplMultiChoice = document.getElementById("tmpl-multi-choice");
  const tmplChannel = document.getElementById("tmpl-channel");

  let ws = null;
  let reconnectDelayMs = 1000;
  let patterns = [];
  const powerValues = [0, 0, 0, 0]; // Chan1..Chan4, as requested locally via the sliders
  const channelEls = []; // index 0..3 -> {slider, percent, barMax, barActual, barLimit}

  function log(text, isError) {
    const line = document.createElement("div");
    if (isError) line.className = "log-error";
    line.textContent = new Date().toLocaleTimeString() + "  " + text;
    logEl.appendChild(line);
    logEl.scrollTop = logEl.scrollHeight;
  }

  function setStatus(state, text) {
    statusEl.className = "status status-" + state;
    statusEl.textContent = text;
  }

  function send(message) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(message));
    }
  }

  // Other tabs/devices can move these same sliders at any time (see "menu_option_changed"
  // and "power_request" broadcasts below) - this stops an incoming update from yanking a
  // slider out from under a finger that's actively dragging it.
  function bindDragGuard(slider) {
    const setDragging = (dragging) => { slider.dataset.dragging = dragging ? "1" : ""; };
    slider.addEventListener("pointerdown", () => setDragging(true));
    slider.addEventListener("pointerup", () => setDragging(false));
    slider.addEventListener("pointercancel", () => setDragging(false));
  }

  function isDragging(slider) {
    return slider.dataset.dragging === "1";
  }

  function connect() {
    setStatus("connecting", "connecting…");
    ws = new WebSocket("ws://" + location.host + "/ws");

    ws.onopen = () => {
      reconnectDelayMs = 1000;
      setStatus("connected", "connected");
      log("Connected to server");
    };

    ws.onclose = () => {
      setStatus("disconnected", "disconnected");
      log("Disconnected from server, retrying…", true);
      setTimeout(connect, reconnectDelayMs);
      reconnectDelayMs = Math.min(reconnectDelayMs * 2, 10000);
    };

    ws.onerror = () => {
      ws.close();
    };

    ws.onmessage = (event) => {
      handleMessage(JSON.parse(event.data));
    };
  }

  function handleMessage(message) {
    switch (message.type) {
      case "patterns":
        patterns = message.patterns;
        renderPatternList();
        break;
      case "pattern_detail":
        showPatternRunning(message.detail);
        break;
      case "pattern_stopped":
        showPatternPicker();
        break;
      case "power_status":
        updatePowerStatus(message.status);
        break;
      case "power_request":
        updatePowerRequest(message);
        break;
      case "menu_option_changed":
        updateMenuOption(message.menu_id, message.value);
        break;
      case "lua_output":
        log((message.text_type === "Error" ? "[error] " : "[print] ") + message.text, message.text_type === "Error");
        break;
      case "lua_error":
        log("Script stopped due to an error", true);
        break;
      case "error":
        log(message.message, true);
        break;
    }
  }

  // ---- pattern picker ----

  function renderPatternList() {
    patternSelectEl.innerHTML = "";
    for (const pattern of patterns) {
      const option = document.createElement("option");
      option.value = pattern.Id;
      option.textContent = pattern.Name;
      patternSelectEl.appendChild(option);
    }
  }

  startBtn.addEventListener("click", () => {
    const id = parseInt(patternSelectEl.value, 10);
    if (!Number.isNaN(id)) {
      send({ cmd: "start_pattern", id });
    }
  });

  stopBtn.addEventListener("click", () => {
    send({ cmd: "stop_pattern" });
  });

  function showPatternPicker() {
    patternPickerEl.hidden = false;
    patternRunningEl.hidden = true;
  }

  // ---- running pattern: menu items + soft button + channels ----

  const minMaxControls = new Map(); // menuId -> {slider, valueEl, min, max, step}
  const multiChoiceControls = new Map(); // menuId -> {buttons: Map(choiceId -> el)}

  function showPatternRunning(detail) {
    patternPickerEl.hidden = true;
    patternRunningEl.hidden = false;
    patternNameEl.textContent = detail.Name;

    // PatternDetail doesn't carry the "(!)" triphase warning prefix that PatternList's
    // Name does (a ZC95 firmware quirk), so cross-reference the pattern list we already have
    const listEntry = patterns.find((p) => p.Id === detail.Id);
    triphaseBadgeEl.hidden = !(listEntry && listEntry.Name.startsWith("(!)"));

    softButtonRowEl.hidden = !(detail.ButtonA && detail.ButtonA.length > 0);
    softButtonEl.textContent = detail.ButtonA || "Soft button";

    menuItemsEl.innerHTML = "";
    minMaxControls.clear();
    multiChoiceControls.clear();

    for (const item of detail.MenuItems) {
      if (item.Type === "MIN_MAX") {
        buildMinMax(item);
      } else if (item.Type === "MULTI_CHOICE") {
        buildMultiChoice(item);
      }
    }

    buildChannels();
  }

  function buildMinMax(item) {
    const node = tmplMinMax.content.firstElementChild.cloneNode(true);
    const title = item.Title + (item.UoM ? " (" + item.UoM + ")" : "");
    node.querySelector(".menu-title").textContent = title;

    const valueEl = node.querySelector(".menu-value");
    const slider = node.querySelector(".menu-slider");
    slider.min = item.Min;
    slider.max = item.Max;
    slider.step = item.IncrementStep;
    slider.value = item.Default;
    valueEl.textContent = item.Default;

    const control = { slider, valueEl, min: item.Min, max: item.Max, step: item.IncrementStep };
    minMaxControls.set(item.Id, control);
    bindDragGuard(slider);

    slider.addEventListener("input", () => {
      valueEl.textContent = slider.value;
    });
    slider.addEventListener("change", () => {
      send({ cmd: "menu_min_max", menu_id: item.Id, value: parseInt(slider.value, 10) });
    });

    node.querySelector(".btn-dec").addEventListener("click", () => {
      stepMinMax(item.Id, -control.step);
    });
    node.querySelector(".btn-inc").addEventListener("click", () => {
      stepMinMax(item.Id, control.step);
    });

    menuItemsEl.appendChild(node);
  }

  function stepMinMax(menuId, delta) {
    const control = minMaxControls.get(menuId);
    if (!control) return;
    let value = parseInt(control.slider.value, 10) + delta;
    value = Math.max(control.min, Math.min(control.max, value));
    control.slider.value = value;
    control.valueEl.textContent = value;
    send({ cmd: "menu_min_max", menu_id: menuId, value });
  }

  function buildMultiChoice(item) {
    const node = tmplMultiChoice.content.firstElementChild.cloneNode(true);
    node.querySelector(".menu-title").textContent = item.Title;
    const group = node.querySelector(".choice-group");

    const buttons = new Map();
    for (const choice of item.Choices) {
      const btn = document.createElement("button");
      btn.className = "choice-btn";
      btn.type = "button";
      btn.textContent = choice.Name;
      if (choice.Id === item.Default) btn.classList.add("active");
      btn.addEventListener("click", () => {
        setMultiChoiceActive(item.Id, choice.Id);
        send({ cmd: "menu_multi_choice", menu_id: item.Id, choice_id: choice.Id });
      });
      group.appendChild(btn);
      buttons.set(choice.Id, btn);
    }

    multiChoiceControls.set(item.Id, { buttons });
    menuItemsEl.appendChild(node);
  }

  function setMultiChoiceActive(menuId, choiceId) {
    const control = multiChoiceControls.get(menuId);
    if (!control) return;
    for (const [id, btn] of control.buttons) {
      btn.classList.toggle("active", id === choiceId);
    }
  }

  function updateMenuOption(menuId, value) {
    if (minMaxControls.has(menuId)) {
      const control = minMaxControls.get(menuId);
      if (isDragging(control.slider)) return;
      control.slider.value = value;
      control.valueEl.textContent = value;
    } else if (multiChoiceControls.has(menuId)) {
      setMultiChoiceActive(menuId, value);
    }
  }

  // Soft button: held down while pressed, released on pointer up/cancel/leave
  function bindSoftButton() {
    let pressed = false;

    const press = (e) => {
      e.preventDefault();
      if (pressed) return;
      pressed = true;
      softButtonEl.classList.add("pressed");
      send({ cmd: "soft_button", pressed: true });
    };

    const release = (e) => {
      if (!pressed) return;
      pressed = false;
      softButtonEl.classList.remove("pressed");
      send({ cmd: "soft_button", pressed: false });
    };

    softButtonEl.addEventListener("pointerdown", press);
    softButtonEl.addEventListener("pointerup", release);
    softButtonEl.addEventListener("pointercancel", release);
    softButtonEl.addEventListener("pointerleave", release);
    softButtonEl.style.touchAction = "none";
  }
  bindSoftButton();

  // ---- channels (power sliders) ----

  const CHANNEL_STEP = 10; // 1000-basis units, i.e. 1%

  function buildChannels() {
    channelsEl.innerHTML = "";
    channelEls.length = 0;

    for (let i = 0; i < 4; i++) {
      const channelNumber = i + 1;
      const node = tmplChannel.content.firstElementChild.cloneNode(true);
      node.querySelector(".channel-label").textContent = "Channel " + channelNumber;

      const slider = node.querySelector(".channel-slider");
      const percentEl = node.querySelector(".channel-percent");
      const barMax = node.querySelector(".channel-bar-max");
      const barActual = node.querySelector(".channel-bar-actual");
      const barLimit = node.querySelector(".channel-bar-limit");

      slider.value = 0;
      powerValues[i] = 0;
      bindDragGuard(slider);

      slider.addEventListener("input", () => {
        setChannelPower(i, parseInt(slider.value, 10));
      });

      node.querySelector(".btn-dec").addEventListener("click", () => {
        stepChannelPower(i, -CHANNEL_STEP);
      });
      node.querySelector(".btn-inc").addEventListener("click", () => {
        stepChannelPower(i, CHANNEL_STEP);
      });

      channelEls[i] = { slider, percentEl, barMax, barActual, barLimit };
      channelsEl.appendChild(node);
    }
  }

  function setChannelPower(index, value) {
    powerValues[index] = value;
    channelEls[index].slider.value = value;
    channelEls[index].percentEl.textContent = Math.round(value / 10) + "%";
    send({ cmd: "set_power", chan1: powerValues[0], chan2: powerValues[1], chan3: powerValues[2], chan4: powerValues[3] });
  }

  function stepChannelPower(index, delta) {
    const value = Math.max(0, Math.min(1000, powerValues[index] + delta));
    setChannelPower(index, value);
  }

  function updatePowerStatus(status) {
    for (const channel of status.Channels) {
      const el = channelEls[channel.Channel - 1];
      if (!el) continue;
      el.barMax.style.width = (channel.MaxOutputPower / 10) + "%";
      el.barActual.style.width = (channel.OutputPower / 10) + "%";
      el.barLimit.style.left = (channel.PowerLimit / 10) + "%";
    }
  }

  // Requested power level as set by any connected client (this tab or another one) -
  // keeps the sliders themselves, not just the bar graphs above, in sync across tabs/devices.
  function updatePowerRequest(message) {
    const values = [message.chan1, message.chan2, message.chan3, message.chan4];
    for (let i = 0; i < 4; i++) {
      const el = channelEls[i];
      if (!el || isDragging(el.slider)) continue;
      powerValues[i] = values[i];
      el.slider.value = values[i];
      el.percentEl.textContent = Math.round(values[i] / 10) + "%";
    }
  }

  // ---- log panel ----

  logToggleBtn.addEventListener("click", () => {
    logEl.hidden = !logEl.hidden;
    logToggleBtn.textContent = logEl.hidden ? "Show log" : "Hide log";
  });

  // ---- theme toggle (client-side only, persisted in localStorage) ----
  //
  // No stored preference -> no data-theme attribute is set, so the CSS media query follows
  // the browser/OS preference live (and falls back to dark if the browser reports none).
  // Once toggled, the explicit choice is pinned via data-theme and persisted, taking over
  // from the live system preference from then on.

  const THEME_KEY = "zc95-theme";

  function systemPrefersLight() {
    return window.matchMedia && window.matchMedia("(prefers-color-scheme: light)").matches;
  }

  function currentTheme() {
    return document.documentElement.getAttribute("data-theme") || (systemPrefersLight() ? "light" : "dark");
  }

  function updateThemeToggleIcon() {
    const theme = currentTheme();
    themeToggleBtn.textContent = theme === "dark" ? "🌙" : "☀️";
    themeToggleBtn.title = theme === "dark" ? "Switch to light theme" : "Switch to dark theme";
  }

  function initTheme() {
    let stored = null;
    try {
      stored = localStorage.getItem(THEME_KEY);
    } catch (e) {
      // localStorage unavailable (private browsing etc.) - just follow the system preference
    }
    if (stored === "dark" || stored === "light") {
      document.documentElement.setAttribute("data-theme", stored);
    }
    updateThemeToggleIcon();
  }

  themeToggleBtn.addEventListener("click", () => {
    const next = currentTheme() === "dark" ? "light" : "dark";
    document.documentElement.setAttribute("data-theme", next);
    try {
      localStorage.setItem(THEME_KEY, next);
    } catch (e) {
      // ignore - theme just won't persist across reloads
    }
    updateThemeToggleIcon();
  });

  initTheme();

  connect();
})();
