// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

const state = {
  devices: [],
  selectedDeviceId: null,
  typeFilter: "",
  statusFilter: "",
  refreshTimer: null,
};

const nodes = {
  connectionStatus: document.querySelector("#connection-status"),
  refreshButton: document.querySelector("#refresh-button"),
  serviceState: document.querySelector("#service-state"),
  deviceCount: document.querySelector("#device-count"),
  attentionCount: document.querySelector("#attention-count"),
  lastUpdate: document.querySelector("#last-update"),
  typeFilter: document.querySelector("#type-filter"),
  statusFilter: document.querySelector("#status-filter"),
  deviceTable: document.querySelector("#device-table"),
  emptyDetail: document.querySelector("#empty-detail"),
  deviceDetail: document.querySelector("#device-detail"),
  detailName: document.querySelector("#detail-name"),
  detailMeta: document.querySelector("#detail-meta"),
  detailStatus: document.querySelector("#detail-status"),
  telemetryList: document.querySelector("#telemetry-list"),
  settingsList: document.querySelector("#settings-list"),
  commandList: document.querySelector("#command-list"),
  toast: document.querySelector("#toast"),
};

function valueToText(value) {
  if (value === null || value === undefined || value === "") {
    return "-";
  }
  if (typeof value === "boolean") {
    return value ? "true" : "false";
  }
  if (typeof value === "object") {
    return JSON.stringify(value);
  }
  return String(value);
}

function escapeHtml(value) {
  return valueToText(value).replace(/[&<>"']/g, (character) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    "\"": "&quot;",
    "'": "&#39;",
  }[character]));
}

function objectToText(value) {
  if (!value || typeof value !== "object") {
    return "-";
  }
  return Object.entries(value)
    .map(([key, item]) => `${escapeHtml(key)}=${escapeHtml(item)}`)
    .join(", ") || "-";
}

function statusClass(status) {
  return `status-pill status-pill--${String(status || "muted").toLowerCase()}`;
}

function showToast(message) {
  nodes.toast.textContent = message;
  nodes.toast.classList.add("is-visible");
  window.setTimeout(() => nodes.toast.classList.remove("is-visible"), 2600);
}

async function fetchJson(url, options) {
  const response = await fetch(url, options);
  const body = await response.json().catch(() => ({}));
  if (!response.ok) {
    throw new Error(body.error || `Request failed: ${response.status}`);
  }
  return body;
}

function setConnectionState(connected) {
  nodes.connectionStatus.textContent = connected ? "Connected" : "Disconnected";
  nodes.connectionStatus.className = connected
    ? "status-pill status-pill--running"
    : "status-pill status-pill--error";
}

function updateFilters() {
  const selectedType = nodes.typeFilter.value;
  const selectedStatus = nodes.statusFilter.value;
  const types = [...new Set(state.devices.map((device) => device.type).filter(Boolean))].sort();
  const statuses = [...new Set(state.devices.map((device) => device.status).filter(Boolean))].sort();

  nodes.typeFilter.innerHTML = `<option value="">All types</option>${types
    .map((type) => `<option value="${escapeHtml(type)}">${escapeHtml(type)}</option>`)
    .join("")}`;
  nodes.statusFilter.innerHTML = `<option value="">All statuses</option>${statuses
    .map((status) => `<option value="${escapeHtml(status)}">${escapeHtml(status)}</option>`)
    .join("")}`;

  nodes.typeFilter.value = types.includes(selectedType) ? selectedType : "";
  nodes.statusFilter.value = statuses.includes(selectedStatus) ? selectedStatus : "";
}

function filteredDevices() {
  return state.devices.filter((device) => {
    if (state.typeFilter && device.type !== state.typeFilter) {
      return false;
    }
    if (state.statusFilter && device.status !== state.statusFilter) {
      return false;
    }
    return true;
  });
}

function renderDevices() {
  const devices = filteredDevices();
  nodes.deviceTable.innerHTML = devices
    .map((device) => `
      <tr data-device-id="${escapeHtml(device.id)}" class="${device.id === state.selectedDeviceId ? "is-selected" : ""}">
        <td>
          <span class="device-name">${escapeHtml(device.name)}</span>
          <span class="device-id">${escapeHtml(device.id)}</span>
        </td>
        <td>${escapeHtml(device.type)}</td>
        <td>${escapeHtml(device.location || "-")}</td>
        <td><span class="${statusClass(device.status)}">${escapeHtml(device.status)}</span></td>
        <td><div class="telemetry-line">${objectToText(device.telemetry)}</div></td>
      </tr>
    `)
    .join("");
}

function renderKeyValues(node, values) {
  node.innerHTML = Object.entries(values || {})
    .map(([key, value]) => `<dt>${escapeHtml(key)}</dt><dd>${escapeHtml(value)}</dd>`)
    .join("") || "<dt>None</dt><dd>-</dd>";
}

function renderDetail() {
  const device = state.devices.find((item) => item.id === state.selectedDeviceId);
  if (!device) {
    nodes.emptyDetail.classList.remove("is-hidden");
    nodes.deviceDetail.classList.add("is-hidden");
    return;
  }

  nodes.emptyDetail.classList.add("is-hidden");
  nodes.deviceDetail.classList.remove("is-hidden");
  nodes.detailName.textContent = device.name;
  nodes.detailMeta.textContent = `${device.id} · ${device.type} · ${device.location || "Unknown location"}`;
  nodes.detailStatus.textContent = device.status;
  nodes.detailStatus.className = statusClass(device.status);
  renderKeyValues(nodes.telemetryList, device.telemetry);
  renderKeyValues(nodes.settingsList, device.settings);

  nodes.commandList.innerHTML = (device.supportedCommands || [])
    .map((command) => `<button class="button" data-command="${escapeHtml(command)}">${escapeHtml(command)}</button>`)
    .join("") || "<p>No commands available.</p>";
}

function updateMetrics(health) {
  nodes.serviceState.textContent = health.service?.running ? "running" : "stopped";
  nodes.deviceCount.textContent = health.deviceCount ?? state.devices.length;
  nodes.attentionCount.textContent = health.attentionCount ?? "0";
  nodes.lastUpdate.textContent = new Date().toLocaleTimeString();
}

async function refreshData() {
  try {
    const [health, devicesPayload] = await Promise.all([
      fetchJson("/api/health"),
      fetchJson("/api/devices"),
    ]);

    state.devices = devicesPayload.devices || [];
    if (!state.selectedDeviceId && state.devices.length > 0) {
      state.selectedDeviceId = state.devices[0].id;
    }

    updateMetrics(health);
    updateFilters();
    renderDevices();
    renderDetail();
    setConnectionState(true);
  } catch (error) {
    setConnectionState(false);
    showToast(error.message);
  }
}

async function sendCommand(command) {
  if (!state.selectedDeviceId) {
    return;
  }

  try {
    await fetchJson(`/api/devices/${encodeURIComponent(state.selectedDeviceId)}/commands`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ command }),
    });
    showToast(`Command accepted: ${command}`);
    await refreshData();
  } catch (error) {
    showToast(error.message);
  }
}

nodes.refreshButton.addEventListener("click", refreshData);
nodes.typeFilter.addEventListener("change", () => {
  state.typeFilter = nodes.typeFilter.value;
  renderDevices();
});
nodes.statusFilter.addEventListener("change", () => {
  state.statusFilter = nodes.statusFilter.value;
  renderDevices();
});
nodes.deviceTable.addEventListener("click", (event) => {
  const row = event.target.closest("tr[data-device-id]");
  if (!row) {
    return;
  }
  state.selectedDeviceId = row.dataset.deviceId;
  renderDevices();
  renderDetail();
});
nodes.commandList.addEventListener("click", (event) => {
  const button = event.target.closest("button[data-command]");
  if (button) {
    sendCommand(button.dataset.command);
  }
});

refreshData();
state.refreshTimer = window.setInterval(refreshData, 3000);
